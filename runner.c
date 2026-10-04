#include "runner.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static uint32_t get_logical_cores(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors > 0 ? (uint32_t)si.dwNumberOfProcessors : 4u;
}

static void runner_sleep_ms(uint32_t ms)
{
    Sleep(ms);
}

static double runner_monotonic_seconds(void)
{
    static LARGE_INTEGER freq;
    static int init = 0;
    if (!init)
    {
        QueryPerformanceFrequency(&freq);
        init = 1;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / (double)freq.QuadPart;
}

#define WINDOW_DURATION_SEC 1.0
#define SLEEP_SLICE_MS 50

struct hashmonke_runner
{
    struct hashmonke_file *file;
    hashmonke_runner_cb cb;
    CRITICAL_SECTION cb_lock;

    hashmonke_runner_event_cb event_cb;
    struct hashmonke_runner_event_context *event_ctx;
    CRITICAL_SECTION event_lock;

    _Atomic size_t next_entry_idx;
    _Atomic uint64_t total_bytes_hashed;
    _Atomic uint32_t active_workers;
    _Atomic uint32_t current_hash_workers;
    _Atomic uint32_t min_hash_workers;
    _Atomic uint32_t max_hash_workers;
    _Atomic uint32_t target_workers;
    _Atomic uint32_t files_matched;
    _Atomic uint32_t files_failed;
    _Atomic uint32_t files_missing;
    _Atomic uint32_t files_malformed;
    _Atomic uint32_t total_files_processed;
    _Atomic bool interrupted;
    _Atomic bool manifest_eof;
    _Atomic bool fatal_error;
    _Atomic bool is_finished;
    _Atomic uint32_t retirement_requests;

    _Atomic double current_throughput_mb_s;
    _Atomic double min_throughput_mb_s;
    _Atomic double max_throughput_mb_s;

    HANDLE coordinator_thread;
    HANDLE *worker_threads;
    uint32_t max_workers;
    uint32_t total_workers_spawned;
    bool thread_warmup;
    CRITICAL_SECTION thread_mgmt_lock;
    CRITICAL_SECTION join_lock;
    bool coordinator_joined;
};

static void record_hash_worker_count(struct hashmonke_runner *runner, uint32_t count)
{
    uint32_t observed = atomic_load(&runner->max_hash_workers);
    while (count > observed &&
           !atomic_compare_exchange_weak(&runner->max_hash_workers, &observed, count))
    {
    }

    observed = atomic_load(&runner->min_hash_workers);
    while (count < observed &&
           !atomic_compare_exchange_weak(&runner->min_hash_workers, &observed, count))
    {
    }
}

struct runner_hasher_worker_ctx
{
    struct hashmonke_runner *runner;
    size_t line_number;
    size_t entry_index;
    const char *file_path;
    const char *display_path;
    double start_time;
};

static void runner_hasher_progress_callback(
    const struct hashmonke_hasher_progress_event *pe,
    struct hashmonke_hasher_progress_context *ctx)
{
    struct runner_hasher_worker_ctx *wctx = (struct runner_hasher_worker_ctx *)ctx;
    if (!wctx || !wctx->runner || !wctx->runner->event_cb)
        return;

    double now = runner_monotonic_seconds();
    double elapsed = now - wctx->start_time;
    double mb_s = 0.0;
    if (elapsed > 0.0)
        mb_s = ((double)pe->bytes_hashed / (1024.0 * 1024.0)) / elapsed;

    struct hashmonke_runner_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = HASHMONKE_RUNNER_EVENT_PROGRESS;
    ev.line_number = wctx->line_number;
    ev.entry_index = wctx->entry_index;
    ev.file_path = wctx->file_path;
    ev.display_path = wctx->display_path;
    ev.bytes_processed = pe->bytes_hashed;
    ev.file_size = pe->bytes_total;
    ev.throughput_mb_s = mb_s;

    EnterCriticalSection(&wctx->runner->event_lock);
    wctx->runner->event_cb(&ev, wctx->runner->event_ctx);
    LeaveCriticalSection(&wctx->runner->event_lock);
}

static DWORD WINAPI worker_func(LPVOID arg)
{
    struct hashmonke_runner *runner = (struct hashmonke_runner *)arg;
    struct hashmonke_hasher *hasher = hashmonke_hasher_create();

    if (!hasher)
    {
        atomic_store(&runner->fatal_error, true);
        atomic_store(&runner->interrupted, true);
        atomic_fetch_sub(&runner->active_workers, 1);
        return 0;
    }

    while (!atomic_load(&runner->interrupted))
    {
        // Claim one retirement request so each target reduction retires one worker.
        uint32_t requests = atomic_load(&runner->retirement_requests);
        while (requests > 0 &&
               !atomic_compare_exchange_weak(&runner->retirement_requests, &requests, requests - 1))
        {
        }
        if (requests > 0)
            break;

        size_t entry_idx = atomic_fetch_add(&runner->next_entry_idx, 1);
        if (entry_idx >= runner->file->num_entries)
        {
            atomic_store(&runner->manifest_eof, true);
            break;
        }

        struct hashmonke_file_entry *entry = &runner->file->entries[entry_idx];

        if (entry->code != HASHMONKE_ENTRY_OK)
        {
            atomic_fetch_add(&runner->files_malformed, 1);
            atomic_fetch_add(&runner->total_files_processed, 1);

            const char *path = entry->file_path ? entry->file_path : "<malformed>";
            const char *disp = entry->display_path ? entry->display_path : "<malformed>";

            if (runner->event_cb)
            {
                struct hashmonke_runner_event ev;
                memset(&ev, 0, sizeof(ev));
                ev.type = HASHMONKE_RUNNER_EVENT_START;
                ev.line_number = entry->line_number;
                ev.entry_index = entry_idx;
                ev.file_path = path;
                ev.display_path = disp;
                ev.status = HASHMONKE_HASH_MALFORMED;
                EnterCriticalSection(&runner->event_lock);
                runner->event_cb(&ev, runner->event_ctx);
                LeaveCriticalSection(&runner->event_lock);

                ev.type = HASHMONKE_RUNNER_EVENT_COMPLETE;
                EnterCriticalSection(&runner->event_lock);
                runner->event_cb(&ev, runner->event_ctx);
                LeaveCriticalSection(&runner->event_lock);
            }

            if (runner->cb)
            {
                EnterCriticalSection(&runner->cb_lock);
                runner->cb(path, HASHMONKE_HASH_MALFORMED);
                LeaveCriticalSection(&runner->cb_lock);
            }
            continue;
        }

        const char *path = entry->file_path;
        const char *disp = entry->display_path ? entry->display_path : entry->file_path;
        double file_start_time = runner_monotonic_seconds();

        if (runner->event_cb)
        {
            struct hashmonke_runner_event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = HASHMONKE_RUNNER_EVENT_START;
            ev.line_number = entry->line_number;
            ev.entry_index = entry_idx;
            ev.file_path = path;
            ev.display_path = disp;
            EnterCriticalSection(&runner->event_lock);
            runner->event_cb(&ev, runner->event_ctx);
            LeaveCriticalSection(&runner->event_lock);
        }

        // HASHMONKE_ENTRY_OK: Execute verification
        uint8_t expected_buf[sizeof(struct hashmonke_hash) + 20];
        struct hashmonke_hash *expected = (struct hashmonke_hash *)expected_buf;
        expected->algo = entry->algo;
        int sz = hashmonke_hash_size(expected);
        if (sz > 0 && entry->hash)
        {
            memcpy(expected->value, entry->hash, (size_t)sz);
        }

        struct runner_hasher_worker_ctx wctx;
        wctx.runner = runner;
        wctx.line_number = entry->line_number;
        wctx.entry_index = entry_idx;
        wctx.file_path = path;
        wctx.display_path = disp;
        wctx.start_time = file_start_time;

        struct hashmonke_hash_ctrl ctrl;
        memset(&ctrl, 0, sizeof(ctrl));
        atomic_store(&ctrl.cancel, atomic_load(&runner->interrupted));
        if (runner->event_cb)
        {
            ctrl.progress_cb = runner_hasher_progress_callback;
            ctrl.progress_context = (struct hashmonke_hasher_progress_context *)&wctx;
        }

        uint32_t hashing_workers = atomic_fetch_add(&runner->current_hash_workers, 1) + 1;
        record_hash_worker_count(runner, hashing_workers);

        enum hashmonke_hash_code hcode = hashmonke_hasher_hash(
            hasher, entry->file_path, entry->text_mode, expected, &ctrl);

        uint32_t remaining_hash_workers = atomic_fetch_sub(&runner->current_hash_workers, 1) - 1;
        if (remaining_hash_workers > 0)
            record_hash_worker_count(runner, remaining_hash_workers);

        atomic_fetch_add(&runner->total_bytes_hashed, atomic_load(&ctrl.bytes_hashed));

        if (hcode == HASHMONKE_HASH_INTERRUPTED)
        {
            break;
        }

        atomic_fetch_add(&runner->total_files_processed, 1);

        if (runner->event_cb)
        {
            double now = runner_monotonic_seconds();
            double elapsed = now - file_start_time;
            uint64_t b_done = atomic_load(&ctrl.bytes_hashed);
            double mb_s = (elapsed > 0.0) ? ((double)b_done / (1024.0 * 1024.0)) / elapsed : 0.0;

            struct hashmonke_runner_event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = HASHMONKE_RUNNER_EVENT_COMPLETE;
            ev.line_number = entry->line_number;
            ev.entry_index = entry_idx;
            ev.file_path = path;
            ev.display_path = disp;
            ev.status = hcode;
            ev.bytes_processed = b_done;
            ev.file_size = atomic_load(&ctrl.bytes_total);
            ev.throughput_mb_s = mb_s;

            EnterCriticalSection(&runner->event_lock);
            runner->event_cb(&ev, runner->event_ctx);
            LeaveCriticalSection(&runner->event_lock);
        }

        if (hcode == HASHMONKE_HASH_INTERNAL_ERR)
        {
            atomic_store(&runner->fatal_error, true);
            atomic_store(&runner->interrupted, true);
            if (runner->cb)
            {
                EnterCriticalSection(&runner->cb_lock);
                runner->cb(entry->file_path, hcode);
                LeaveCriticalSection(&runner->cb_lock);
            }
            break;
        }

        if (hcode == HASHMONKE_HASH_MATCHES)
        {
            atomic_fetch_add(&runner->files_matched, 1);
        }
        else if (hcode == HASHMONKE_HASH_MISMATCH)
        {
            atomic_fetch_add(&runner->files_failed, 1);
        }
        else
        {
            atomic_fetch_add(&runner->files_missing, 1);
        }

        if (runner->cb)
        {
            EnterCriticalSection(&runner->cb_lock);
            runner->cb(entry->file_path, hcode);
            LeaveCriticalSection(&runner->cb_lock);
        }
    }

    if (hasher)
    {
        hashmonke_hasher_free(hasher);
    }
    atomic_fetch_sub(&runner->active_workers, 1);
    return 0;
}

static bool spawn_worker(struct hashmonke_runner *runner)
{
    EnterCriticalSection(&runner->thread_mgmt_lock);
    if (runner->total_workers_spawned >= runner->max_workers)
    {
        LeaveCriticalSection(&runner->thread_mgmt_lock);
        return false;
    }

    atomic_fetch_add(&runner->active_workers, 1);
    HANDLE thread = CreateThread(NULL, 0, worker_func, runner, 0, NULL);
    if (!thread)
    {
        atomic_fetch_sub(&runner->active_workers, 1);
        atomic_store(&runner->fatal_error, true);
        atomic_store(&runner->interrupted, true);
        LeaveCriticalSection(&runner->thread_mgmt_lock);
        return false;
    }

    runner->worker_threads[runner->total_workers_spawned++] = thread;
    LeaveCriticalSection(&runner->thread_mgmt_lock);
    return true;
}

enum tuner_state
{
    STATE_SETTLING,
    STATE_EVALUATE,
    STATE_LOCKED
};

static DWORD WINAPI coordinator_func(LPVOID arg)
{
    struct hashmonke_runner *runner = (struct hashmonke_runner *)arg;

    // Start with the requested worker count.
    uint32_t initial_workers = atomic_load(&runner->target_workers);
    for (uint32_t i = 0; i < initial_workers; ++i)
        spawn_worker(runner);

    enum tuner_state state = runner->thread_warmup ? STATE_SETTLING : STATE_LOCKED;
    int direction = +1;          // +1 = increasing workers, -1 = decreasing workers
    uint32_t reversals = 0;       // Direction changes; capped at 3
    double prev_rate = 0.0;
    double best_rate = 0.0;
    uint32_t best_workers = initial_workers;

    while (!atomic_load(&runner->interrupted))
    {
        if (atomic_load(&runner->manifest_eof) && atomic_load(&runner->active_workers) == 0)
        {
            break;
        }

        // Measure a window
        uint64_t bytes_start = atomic_load(&runner->total_bytes_hashed);
        double window_start = runner_monotonic_seconds();
        while (runner_monotonic_seconds() - window_start < WINDOW_DURATION_SEC)
        {
            if (atomic_load(&runner->interrupted))
                break;
            if (atomic_load(&runner->manifest_eof) && atomic_load(&runner->active_workers) == 0)
                break;
            runner_sleep_ms(SLEEP_SLICE_MS);
        }

        double elapsed_sec = runner_monotonic_seconds() - window_start;
        if (elapsed_sec <= 0.0)
            elapsed_sec = 0.001;
        uint64_t bytes_end = atomic_load(&runner->total_bytes_hashed);
        uint64_t window_bytes = (bytes_end >= bytes_start) ? (bytes_end - bytes_start) : 0;
        double current_rate = (elapsed_sec > 0.0) ? ((double)window_bytes / elapsed_sec) : 0.0;
        double throughput_mb_s = current_rate / (1024.0 * 1024.0);
        atomic_store(&runner->current_throughput_mb_s, throughput_mb_s);
        if (throughput_mb_s > 0.0)
        {
            double cur_min = atomic_load(&runner->min_throughput_mb_s);
            while ((cur_min <= 0.0 || throughput_mb_s < cur_min) &&
                   !atomic_compare_exchange_weak(&runner->min_throughput_mb_s, &cur_min, throughput_mb_s))
            {
            }
            double cur_max = atomic_load(&runner->max_throughput_mb_s);
            while (throughput_mb_s > cur_max &&
                   !atomic_compare_exchange_weak(&runner->max_throughput_mb_s, &cur_max, throughput_mb_s))
            {
            }
        }

        if (atomic_load(&runner->manifest_eof) || atomic_load(&runner->interrupted))
        {
            continue;
        }

        switch (state)
        {
        case STATE_SETTLING:
            // Discard transitional window after thread count changes; transition to measurement
            state = STATE_EVALUATE;
            break;

        case STATE_EVALUATE: {
            uint32_t current_w = atomic_load(&runner->active_workers);

            // Track all-time best throughput and worker count
            if (current_rate > best_rate)
            {
                best_rate = current_rate;
                best_workers = current_w;
            }

            if (prev_rate <= 0.0)
            {
                // First valid measurement: establish baseline and try scaling up
                prev_rate = current_rate;
                if (current_w < runner->max_workers && !atomic_load(&runner->manifest_eof))
                {
                    atomic_fetch_add(&runner->target_workers, 1);
                    spawn_worker(runner);
                    direction = +1;
                    state = STATE_SETTLING;
                }
                else
                {
                    state = STATE_LOCKED;
                }
                break;
            }

            // Decide whether to continue or reverse direction
            if (direction == +1)
            {
                // We scaled up: did throughput improve by at least 5%?
                if (current_rate >= prev_rate * 1.05)
                {
                    // Improved: continue climbing if headroom exists
                    prev_rate = current_rate;
                    if (current_w < runner->max_workers && !atomic_load(&runner->manifest_eof))
                    {
                        atomic_fetch_add(&runner->target_workers, 1);
                        spawn_worker(runner);
                        state = STATE_SETTLING;
                    }
                    else
                    {
                        state = STATE_LOCKED;
                    }
                }
                else
                {
                    // Plateaued or regressed: reverse direction
                    reversals++;
                    direction = -1;
                    prev_rate = current_rate;

                    if (reversals >= 3)
                    {
                        // Oscillation limit reached: converge to best observed worker count
                        if (current_w > best_workers)
                        {
                            atomic_store(&runner->target_workers, best_workers);
                            atomic_fetch_add(&runner->retirement_requests, current_w - best_workers);
                        }
                        else if (current_w < best_workers)
                        {
                            uint32_t to_add = best_workers - current_w;
                            atomic_store(&runner->target_workers, best_workers);
                            for (uint32_t i = 0; i < to_add; ++i)
                                spawn_worker(runner);
                        }
                        state = STATE_LOCKED;
                    }
                    else
                    {
                        // Step down 1 worker
                        uint32_t cur_target = atomic_load(&runner->target_workers);
                        if (cur_target > 1)
                        {
                            atomic_store(&runner->target_workers, cur_target - 1);
                            atomic_fetch_add(&runner->retirement_requests, 1);
                            state = STATE_SETTLING;
                        }
                        else
                        {
                            state = STATE_LOCKED;
                        }
                    }
                }
            }
            else // direction == -1
            {
                // We scaled down: did throughput hold within 2% or improve?
                if (current_rate >= prev_rate * 0.98)
                {
                    // Throughput held or improved with fewer threads: continue reducing
                    prev_rate = current_rate;
                    uint32_t cur_target = atomic_load(&runner->target_workers);
                    if (cur_target > 1)
                    {
                        atomic_store(&runner->target_workers, cur_target - 1);
                        atomic_fetch_add(&runner->retirement_requests, 1);
                        state = STATE_SETTLING;
                    }
                    else
                    {
                        state = STATE_LOCKED;
                    }
                }
                else
                {
                    // Throughput dropped noticeably: reversing direction back up
                    reversals++;
                    direction = +1;
                    prev_rate = current_rate;

                    if (reversals >= 3)
                    {
                        // Oscillation limit reached: converge to best observed worker count
                        if (current_w > best_workers)
                        {
                            atomic_store(&runner->target_workers, best_workers);
                            atomic_fetch_add(&runner->retirement_requests, current_w - best_workers);
                        }
                        else if (current_w < best_workers)
                        {
                            uint32_t to_add = best_workers - current_w;
                            atomic_store(&runner->target_workers, best_workers);
                            for (uint32_t i = 0; i < to_add; ++i)
                                spawn_worker(runner);
                        }
                        state = STATE_LOCKED;
                    }
                    else
                    {
                        // Step up 1 worker
                        if (current_w < runner->max_workers && !atomic_load(&runner->manifest_eof))
                        {
                            atomic_fetch_add(&runner->target_workers, 1);
                            spawn_worker(runner);
                            state = STATE_SETTLING;
                        }
                        else
                        {
                            state = STATE_LOCKED;
                        }
                    }
                }
            }
            break;
        }

        case STATE_LOCKED:
            // Worker count locked for remainder of run
            break;
        }
    }

    // Wait for all active workers to finish
    while (atomic_load(&runner->active_workers) > 0)
    {
        runner_sleep_ms(10);
    }

    // Join all spawned workers
    EnterCriticalSection(&runner->thread_mgmt_lock);
    for (uint32_t i = 0; i < runner->total_workers_spawned; ++i)
    {
        WaitForSingleObject(runner->worker_threads[i], INFINITE);
        CloseHandle(runner->worker_threads[i]);
    }
    runner->total_workers_spawned = 0;
    LeaveCriticalSection(&runner->thread_mgmt_lock);

    atomic_store(&runner->is_finished, true);
    return 0;
}

struct hashmonke_runner *hashmonke_runner_run_with_events(
    struct hashmonke_file *file, hashmonke_runner_cb cb,
    hashmonke_runner_event_cb event_cb, struct hashmonke_runner_event_context *event_ctx,
    uint32_t starting_workers, bool thread_warmup)
{
    if (!file)
        return NULL;

    struct hashmonke_runner *runner = (struct hashmonke_runner *)calloc(1, sizeof(struct hashmonke_runner));
    if (!runner)
        return NULL;

    runner->file = file;
    runner->cb = cb;
    runner->event_cb = event_cb;
    runner->event_ctx = event_ctx;
    runner->thread_warmup = thread_warmup;
    atomic_store(&runner->min_hash_workers, UINT32_MAX);

    InitializeCriticalSection(&runner->cb_lock);
    InitializeCriticalSection(&runner->event_lock);
    InitializeCriticalSection(&runner->thread_mgmt_lock);
    InitializeCriticalSection(&runner->join_lock);

    runner->max_workers = get_logical_cores();
    if (runner->max_workers < 1)
        runner->max_workers = 1;
    if (runner->max_workers > 128)
        runner->max_workers = 128;

    if (starting_workers < 1)
        starting_workers = 1;
    if (starting_workers > runner->max_workers)
        starting_workers = runner->max_workers;
    atomic_store(&runner->target_workers, starting_workers);

    runner->worker_threads = (HANDLE *)malloc(sizeof(HANDLE) * runner->max_workers);
    if (!runner->worker_threads)
    {
        DeleteCriticalSection(&runner->join_lock);
        DeleteCriticalSection(&runner->thread_mgmt_lock);
        DeleteCriticalSection(&runner->event_lock);
        DeleteCriticalSection(&runner->cb_lock);
        free(runner);
        return NULL;
    }

    runner->coordinator_thread = CreateThread(NULL, 0, coordinator_func, runner, 0, NULL);
    if (!runner->coordinator_thread)
    {
        free(runner->worker_threads);
        DeleteCriticalSection(&runner->join_lock);
        DeleteCriticalSection(&runner->thread_mgmt_lock);
        DeleteCriticalSection(&runner->event_lock);
        DeleteCriticalSection(&runner->cb_lock);
        free(runner);
        return NULL;
    }

    return runner;
}

struct hashmonke_runner *hashmonke_runner_run_with_options(
    struct hashmonke_file *file, hashmonke_runner_cb cb, uint32_t starting_workers, bool thread_warmup)
{
    return hashmonke_runner_run_with_events(file, cb, NULL, NULL, starting_workers, thread_warmup);
}

struct hashmonke_runner *hashmonke_runner_run_with_starting_workers(
    struct hashmonke_file *file, hashmonke_runner_cb cb, uint32_t starting_workers)
{
    return hashmonke_runner_run_with_options(file, cb, starting_workers, true);
}

struct hashmonke_runner *hashmonke_runner_run(struct hashmonke_file *file, hashmonke_runner_cb cb)
{
    return hashmonke_runner_run_with_starting_workers(file, cb, 1);
}

void hashmonke_runner_wait(struct hashmonke_runner *runner)
{
    if (!runner)
        return;
    EnterCriticalSection(&runner->join_lock);
    if (!runner->coordinator_joined && runner->coordinator_thread)
    {
        WaitForSingleObject(runner->coordinator_thread, INFINITE);
        CloseHandle(runner->coordinator_thread);
        runner->coordinator_thread = NULL;
        runner->coordinator_joined = true;
    }
    LeaveCriticalSection(&runner->join_lock);
}

void hashmonke_runner_interrupt(struct hashmonke_runner *runner)
{
    if (!runner)
        return;
    atomic_store(&runner->interrupted, true);
}

struct hashmonke_runner_stats hashmonke_runner_get_stats(struct hashmonke_runner *runner)
{
    struct hashmonke_runner_stats stats;
    memset(&stats, 0, sizeof(stats));
    if (!runner)
        return stats;

    stats.total_bytes_hashed = atomic_load(&runner->total_bytes_hashed);
    stats.active_workers = atomic_load(&runner->active_workers);
    stats.min_hash_workers = atomic_load(&runner->min_hash_workers);
    stats.max_hash_workers = atomic_load(&runner->max_hash_workers);
    stats.files_matched = atomic_load(&runner->files_matched);
    stats.files_failed = atomic_load(&runner->files_failed);
    stats.files_missing = atomic_load(&runner->files_missing);
    stats.files_malformed = atomic_load(&runner->files_malformed);
    stats.total_files_processed = atomic_load(&runner->total_files_processed);
    stats.current_throughput_mb_s = atomic_load(&runner->current_throughput_mb_s);
    stats.min_throughput_mb_s = atomic_load(&runner->min_throughput_mb_s);
    stats.max_throughput_mb_s = atomic_load(&runner->max_throughput_mb_s);
    stats.is_finished = atomic_load(&runner->is_finished);
    stats.has_error = atomic_load(&runner->fatal_error);
    return stats;
}

void hashmonke_runner_free(struct hashmonke_runner *runner)
{
    if (!runner)
        return;
    hashmonke_runner_interrupt(runner);
    hashmonke_runner_wait(runner);

    DeleteCriticalSection(&runner->cb_lock);
    DeleteCriticalSection(&runner->event_lock);
    DeleteCriticalSection(&runner->thread_mgmt_lock);
    DeleteCriticalSection(&runner->join_lock);
    free(runner->worker_threads);
    free(runner);
}
