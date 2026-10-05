#include "runner.h"
#include "time_util.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define WINDOW_DURATION_SEC 1.0
#define EVENT_POLL_TIMEOUT_MS 30
#define EVENT_QUEUE_CAP 1024

struct hashmonke_runner;

struct worker
{
    struct hashmonke_runner *runner;
    HANDLE thread;
    HANDLE work_event;
    struct hashmonke_file_entry *entry;
    size_t entry_idx;
    double start_time;
    struct hashmonke_hash_ctrl ctrl;
    bool busy;
    bool terminate;
};

struct event_msg
{
    enum hashmonke_runner_event_type type;
    size_t entry_idx;
    uint64_t bytes_chunk;
    uint64_t bytes_hashed;
    uint64_t bytes_total;
    enum hashmonke_hash_code status;
    struct worker *worker;
};

enum tuner_state
{
    STATE_SETTLING,
    STATE_EVALUATE,
    STATE_LOCKED
};

struct hashmonke_runner
{
    struct hashmonke_file *file;
    hashmonke_runner_cb cb;
    hashmonke_runner_event_cb event_cb;
    struct hashmonke_runner_event_context *event_ctx;
    bool thread_warmup;
    double start_time;

    uint32_t max_workers;
    uint32_t spawned_workers;
    uint32_t active_workers;
    uint32_t target_workers;
    size_t next_entry_idx;

    // Hill climber state
    enum tuner_state hc_state;
    int hc_direction;
    uint32_t hc_reversals;
    double hc_prev_rate;
    double hc_best_rate;
    uint32_t hc_best_workers;

    bool interrupted;
    HANDLE notify_event;
    HANDLE loop_thread;

    struct worker *workers;

    // Ring-buffer event queue from workers
    CRITICAL_SECTION queue_lock;
    struct event_msg queue[EVENT_QUEUE_CAP];
    uint32_t queue_head;
    uint32_t queue_tail;
    uint32_t queue_count;

    // Published stats for external readers
    CRITICAL_SECTION stats_lock;
    struct hashmonke_runner_stats stats;
    struct hashmonke_runner_stats published_stats;
};


static uint32_t get_default_max_workers(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uint32_t cores = si.dwNumberOfProcessors > 0 ? (uint32_t)si.dwNumberOfProcessors : 4u;
    return cores > 16 ? 16 : cores;
}

static void update_throughput(struct hashmonke_runner_stats *st, double mb_s)
{
    if (mb_s <= 0.0)
        return;
    st->current_throughput_mb_s = mb_s;
    if (st->min_throughput_mb_s <= 0.0 || mb_s < st->min_throughput_mb_s)
        st->min_throughput_mb_s = mb_s;
    if (mb_s > st->max_throughput_mb_s)
        st->max_throughput_mb_s = mb_s;
}

static void emit_event(struct hashmonke_runner *runner, enum hashmonke_runner_event_type type,
                       const struct hashmonke_file_entry *entry, size_t entry_idx,
                       enum hashmonke_hash_code status, uint64_t bytes, uint64_t total, double mb_s)
{
    if (!runner->event_cb)
        return;

    const char *path = entry && entry->file_path ? entry->file_path : "<malformed>";
    struct hashmonke_runner_event ev = {
        .type = type,
        .line_number = entry ? entry->line_number : 0,
        .entry_index = entry_idx,
        .file_path = path,
        .display_path = entry && entry->display_path ? entry->display_path : path,
        .status = status,
        .bytes_processed = bytes,
        .file_size = total,
        .throughput_mb_s = mb_s,
    };
    runner->event_cb(&ev, runner->event_ctx);
}

static void push_event(struct hashmonke_runner *runner, const struct event_msg *msg)
{
    EnterCriticalSection(&runner->queue_lock);
    if (runner->queue_count < EVENT_QUEUE_CAP)
    {
        runner->queue[runner->queue_tail] = *msg;
        runner->queue_tail = (runner->queue_tail + 1) % EVENT_QUEUE_CAP;
        runner->queue_count++;
    }
    LeaveCriticalSection(&runner->queue_lock);

    SetEvent(runner->notify_event);
}

static void worker_progress_callback(const struct hashmonke_hasher_progress_event *pe,
                                     struct hashmonke_hasher_progress_context *ctx)
{
    struct worker *w = (struct worker *)ctx;
    if (!w || !w->runner)
        return;

    struct event_msg msg = {
        .type = HASHMONKE_RUNNER_EVENT_PROGRESS,
        .entry_idx = w->entry_idx,
        .bytes_chunk = pe->bytes_chunk,
        .bytes_hashed = pe->bytes_hashed,
        .bytes_total = pe->bytes_total,
        .worker = w,
    };
    push_event(w->runner, &msg);
}

static DWORD WINAPI worker_thread_proc(LPVOID arg)
{
    struct worker *w = (struct worker *)arg;
    struct hashmonke_hasher *hasher = hashmonke_hasher_create();
    if (!hasher)
        return 1;

    while (true)
    {
        WaitForSingleObject(w->work_event, INFINITE);
        if (w->terminate)
            break;

        uint8_t expected_buf[sizeof(struct hashmonke_hash) + 20];
        struct hashmonke_hash *expected = (struct hashmonke_hash *)expected_buf;
        expected->algo = w->entry->algo;
        int sz = hashmonke_hash_size(expected);
        if (sz > 0 && w->entry->hash)
            memcpy(expected->value, w->entry->hash, (size_t)sz);

        memset(&w->ctrl, 0, sizeof(w->ctrl));
        atomic_store(&w->ctrl.cancel, w->runner->interrupted);
        w->ctrl.progress_cb = worker_progress_callback;
        w->ctrl.progress_context = (struct hashmonke_hasher_progress_context *)w;

        enum hashmonke_hash_code code = hashmonke_hasher_hash(
            hasher, w->entry->file_path, w->entry->text_mode, expected, &w->ctrl);

        struct event_msg msg = {
            .type = HASHMONKE_RUNNER_EVENT_COMPLETE,
            .entry_idx = w->entry_idx,
            .bytes_hashed = atomic_load(&w->ctrl.bytes_hashed),
            .bytes_total = atomic_load(&w->ctrl.bytes_total),
            .status = code,
            .worker = w,
        };
        push_event(w->runner, &msg);
    }

    hashmonke_hasher_free(hasher);
    return 0;
}

static struct worker *spawn_worker(struct hashmonke_runner *runner)
{
    if (runner->spawned_workers >= runner->max_workers)
        return NULL;

    struct worker *w = &runner->workers[runner->spawned_workers];
    memset(w, 0, sizeof(*w));
    w->runner = runner;
    w->work_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!w->work_event)
        return NULL;

    w->thread = CreateThread(NULL, 0, worker_thread_proc, w, 0, NULL);
    if (!w->thread)
    {
        CloseHandle(w->work_event);
        return NULL;
    }

    runner->spawned_workers++;
    return w;
}

static void hill_climber_step(struct hashmonke_runner *r, double rate)
{
    if (r->hc_state == STATE_SETTLING)
    {
        r->hc_state = STATE_EVALUATE;
        return;
    }
    if (r->hc_state != STATE_EVALUATE)
        return;

    if (rate > r->hc_best_rate)
    {
        r->hc_best_rate = rate;
        r->hc_best_workers = r->active_workers;
    }

    if (r->hc_prev_rate <= 0.0)
    {
        r->hc_prev_rate = rate;
        if (r->target_workers < r->max_workers)
        {
            r->target_workers++;
            r->hc_direction = 1;
            r->hc_state = STATE_SETTLING;
        }
        else
            r->hc_state = STATE_LOCKED;
        return;
    }

    bool up = (r->hc_direction > 0);
    bool ok = up ? (rate >= r->hc_prev_rate * 1.05) : (rate >= r->hc_prev_rate * 0.98);
    r->hc_prev_rate = rate;

    if (ok)
    {
        uint32_t next = r->target_workers + (up ? 1 : -1);
        if (next >= 1 && next <= r->max_workers)
        {
            r->target_workers = next;
            r->hc_state = STATE_SETTLING;
        }
        else
            r->hc_state = STATE_LOCKED;
    }
    else if (++r->hc_reversals >= 3)
    {
        r->target_workers = r->hc_best_workers;
        r->hc_state = STATE_LOCKED;
    }
    else
    {
        r->hc_direction = -r->hc_direction;
        uint32_t next = r->target_workers + r->hc_direction;
        r->target_workers = (next < 1) ? 1 : (next > r->max_workers ? r->max_workers : next);
        r->hc_state = STATE_SETTLING;
    }
}

static void process_events(struct hashmonke_runner *runner)
{
    struct event_msg batch[64];
    while (true)
    {
        uint32_t count = 0;
        EnterCriticalSection(&runner->queue_lock);
        while (runner->queue_count > 0 && count < 64)
        {
            batch[count++] = runner->queue[runner->queue_head];
            runner->queue_head = (runner->queue_head + 1) % EVENT_QUEUE_CAP;
            runner->queue_count--;
        }
        LeaveCriticalSection(&runner->queue_lock);

        if (count == 0)
            break;

        for (uint32_t i = 0; i < count; ++i)
        {
            struct event_msg *m = &batch[i];
            struct hashmonke_file_entry *entry = &runner->file->entries[m->entry_idx];
            double elapsed = hashmonke_monotonic_seconds() - m->worker->start_time;
            double mb_s = (elapsed > 0.0) ? ((double)m->bytes_hashed / (1024.0 * 1024.0)) / elapsed : 0.0;

            if (m->type == HASHMONKE_RUNNER_EVENT_PROGRESS)
            {
                runner->stats.total_bytes_hashed += m->bytes_chunk;
                emit_event(runner, HASHMONKE_RUNNER_EVENT_PROGRESS, entry, m->entry_idx, 0, m->bytes_hashed, m->bytes_total, mb_s);
            }
            else
            {
                m->worker->busy = false;
                runner->active_workers--;
                runner->stats.total_files_processed++;

                if (m->status == HASHMONKE_HASH_MATCHES)
                    runner->stats.files_matched++;
                else if (m->status == HASHMONKE_HASH_IO_ERR)
                    runner->stats.files_missing++;
                else if (m->status == HASHMONKE_HASH_MALFORMED)
                    runner->stats.files_malformed++;
                else
                    runner->stats.files_failed++;

                emit_event(runner, HASHMONKE_RUNNER_EVENT_COMPLETE, entry, m->entry_idx, m->status, m->bytes_hashed, m->bytes_total, mb_s);
                if (runner->cb)
                    runner->cb(entry->file_path ? entry->file_path : "<unknown>", m->status);
            }
        }
    }
}

static void dispatch_workers(struct hashmonke_runner *runner)
{
    while (runner->active_workers < runner->target_workers &&
           runner->next_entry_idx < runner->file->num_entries &&
           !runner->interrupted)
    {
        size_t idx = runner->next_entry_idx++;
        struct hashmonke_file_entry *entry = &runner->file->entries[idx];
        if (entry->code != HASHMONKE_ENTRY_OK)
        {
            runner->stats.files_malformed++;
            runner->stats.total_files_processed++;
            emit_event(runner, HASHMONKE_RUNNER_EVENT_START, entry, idx, HASHMONKE_HASH_MALFORMED, 0, 0, 0.0);
            emit_event(runner, HASHMONKE_RUNNER_EVENT_COMPLETE, entry, idx, HASHMONKE_HASH_MALFORMED, 0, 0, 0.0);
            if (runner->cb)
                runner->cb(entry->file_path ? entry->file_path : "<malformed>", HASHMONKE_HASH_MALFORMED);
            continue;
        }

        struct worker *w = NULL;
        for (uint32_t i = 0; i < runner->spawned_workers; ++i)
        {
            if (!runner->workers[i].busy)
            {
                w = &runner->workers[i];
                break;
            }
        }
        if (!w && runner->spawned_workers < runner->max_workers)
            w = spawn_worker(runner);

        if (w)
        {
            w->busy = true;
            w->entry = entry;
            w->entry_idx = idx;
            w->start_time = hashmonke_monotonic_seconds();
            runner->active_workers++;
            emit_event(runner, HASHMONKE_RUNNER_EVENT_START, entry, idx, 0, 0, 0, 0.0);
            SetEvent(w->work_event);
        }
        else
        {
            runner->next_entry_idx--;
            break;
        }
    }
}

static DWORD WINAPI loop_thread_proc(LPVOID arg)
{
    struct hashmonke_runner *r = (struct hashmonke_runner *)arg;
    double win_t = hashmonke_monotonic_seconds(), last_sample_t = win_t;
    uint64_t win_b = 0, last_sample_b = 0;

    while (!r->interrupted)
    {
        WaitForSingleObject(r->notify_event, EVENT_POLL_TIMEOUT_MS);
        ResetEvent(r->notify_event);

        process_events(r);

        r->stats.active_workers = r->active_workers;
        if (r->active_workers > 0)
        {
            if (r->active_workers > r->stats.max_hash_workers)
                r->stats.max_hash_workers = r->active_workers;
            if (r->stats.min_hash_workers == 0 || r->active_workers < r->stats.min_hash_workers)
                r->stats.min_hash_workers = r->active_workers;
        }

        double now = hashmonke_monotonic_seconds();
        if (now - last_sample_t >= 0.1)
        {
            uint64_t sb = (r->stats.total_bytes_hashed >= last_sample_b) ? (r->stats.total_bytes_hashed - last_sample_b) : 0;
            update_throughput(&r->stats, ((double)sb / (1024.0 * 1024.0)) / (now - last_sample_t));
            last_sample_t = now;
            last_sample_b = r->stats.total_bytes_hashed;
        }

        if (now - win_t >= WINDOW_DURATION_SEC)
        {
            uint64_t wb = (r->stats.total_bytes_hashed >= win_b) ? (r->stats.total_bytes_hashed - win_b) : 0;
            double rate = (now > win_t) ? (double)wb / (now - win_t) : 0.0;
            update_throughput(&r->stats, rate / (1024.0 * 1024.0));
            hill_climber_step(r, rate);
            win_t = now;
            win_b = r->stats.total_bytes_hashed;
        }

        dispatch_workers(r);

        if (r->active_workers == 0 && r->next_entry_idx >= r->file->num_entries)
            break;

        EnterCriticalSection(&r->stats_lock);
        r->published_stats = r->stats;
        LeaveCriticalSection(&r->stats_lock);
    }

    double elapsed = hashmonke_monotonic_seconds() - r->start_time;
    update_throughput(&r->stats, ((double)r->stats.total_bytes_hashed / (1024.0 * 1024.0)) / (elapsed > 0 ? elapsed : 0.001));
    r->stats.is_finished = true;
    r->stats.active_workers = 0;

    EnterCriticalSection(&r->stats_lock);
    r->published_stats = r->stats;
    LeaveCriticalSection(&r->stats_lock);
    return 0;
}

struct hashmonke_runner *hashmonke_runner_run_with_events(
    struct hashmonke_file *file, hashmonke_runner_cb cb,
    hashmonke_runner_event_cb event_cb,
    struct hashmonke_runner_event_context *event_ctx,
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
    runner->start_time = hashmonke_monotonic_seconds();

    runner->max_workers = get_default_max_workers();
    if (starting_workers > runner->max_workers)
        runner->max_workers = starting_workers;
    if (runner->max_workers < 1)
        runner->max_workers = 1;
    if (runner->max_workers > 128)
        runner->max_workers = 128;

    if (starting_workers < 1)
        starting_workers = 1;
    if (starting_workers > runner->max_workers)
        starting_workers = runner->max_workers;

    runner->target_workers = starting_workers;
    runner->hc_state = thread_warmup ? STATE_SETTLING : STATE_LOCKED;
    runner->hc_direction = 1;
    runner->hc_best_workers = starting_workers;

    runner->workers = (struct worker *)calloc(runner->max_workers, sizeof(struct worker));
    if (!runner->workers)
    {
        free(runner);
        return NULL;
    }

    InitializeCriticalSection(&runner->queue_lock);
    InitializeCriticalSection(&runner->stats_lock);
    runner->notify_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!runner->notify_event)
    {
        DeleteCriticalSection(&runner->stats_lock);
        DeleteCriticalSection(&runner->queue_lock);
        free(runner->workers);
        free(runner);
        return NULL;
    }

    for (uint32_t i = 0; i < starting_workers; ++i)
    {
        if (!spawn_worker(runner))
            break;
    }

    runner->loop_thread = CreateThread(NULL, 0, loop_thread_proc, runner, 0, NULL);
    if (!runner->loop_thread)
    {
        hashmonke_runner_free(runner);
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
    if (runner && runner->loop_thread)
        WaitForSingleObject(runner->loop_thread, INFINITE);
}

void hashmonke_runner_interrupt(struct hashmonke_runner *runner)
{
    if (!runner)
        return;
    runner->interrupted = true;
    for (uint32_t i = 0; i < runner->spawned_workers; ++i)
        atomic_store(&runner->workers[i].ctrl.cancel, true);
    SetEvent(runner->notify_event);
}

struct hashmonke_runner_stats hashmonke_runner_get_stats(struct hashmonke_runner *runner)
{
    struct hashmonke_runner_stats s;
    memset(&s, 0, sizeof(s));
    if (!runner)
        return s;
    EnterCriticalSection(&runner->stats_lock);
    s = runner->published_stats;
    LeaveCriticalSection(&runner->stats_lock);
    return s;
}

void hashmonke_runner_free(struct hashmonke_runner *runner)
{
    if (!runner)
        return;
    hashmonke_runner_interrupt(runner);
    hashmonke_runner_wait(runner);

    for (uint32_t i = 0; i < runner->spawned_workers; ++i)
    {
        runner->workers[i].terminate = true;
        SetEvent(runner->workers[i].work_event);
    }
    for (uint32_t i = 0; i < runner->spawned_workers; ++i)
    {
        WaitForSingleObject(runner->workers[i].thread, INFINITE);
        CloseHandle(runner->workers[i].thread);
        CloseHandle(runner->workers[i].work_event);
    }
    free(runner->workers);

    CloseHandle(runner->loop_thread);
    CloseHandle(runner->notify_event);
    DeleteCriticalSection(&runner->queue_lock);
    DeleteCriticalSection(&runner->stats_lock);
    free(runner);
}
