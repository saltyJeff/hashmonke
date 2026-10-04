#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "runner.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
static uint32_t get_logical_cores(void)
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return (sysinfo.dwNumberOfProcessors > 0) ? (uint32_t)sysinfo.dwNumberOfProcessors : 4u;
}
static void runner_sleep_ms(uint32_t ms)
{
    Sleep(ms);
}
#else
#include <unistd.h>
static uint32_t get_logical_cores(void)
{
    long c = sysconf(_SC_NPROCESSORS_ONLN);
    return (c > 0) ? (uint32_t)c : 4u;
}
static void runner_sleep_ms(uint32_t ms)
{
    struct timespec duration;
    duration.tv_sec = ms / 1000;
    duration.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&duration, NULL);
}
#endif

static double runner_monotonic_seconds(void)
{
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
#endif
}

#define WINDOW_DURATION_SEC 1.0
#define SLEEP_SLICE_MS 50

struct hashmonke_runner
{
    struct hashmonke_file *file;
    hashmonke_runner_cb cb;
    pthread_mutex_t cb_lock;

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

    pthread_t coordinator_thread;
    pthread_t *worker_threads;
    uint32_t max_workers;
    uint32_t total_workers_spawned;
    bool thread_warmup;
    pthread_mutex_t thread_mgmt_lock;
    pthread_mutex_t join_lock;
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

static void *worker_func(void *arg)
{
    struct hashmonke_runner *runner = (struct hashmonke_runner *)arg;
    struct hashmonke_hasher *hasher = hashmonke_hasher_create();

    if (!hasher)
    {
        atomic_store(&runner->fatal_error, true);
        atomic_store(&runner->interrupted, true);
        atomic_fetch_sub(&runner->active_workers, 1);
        return NULL;
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

            if (runner->cb)
            {
                pthread_mutex_lock(&runner->cb_lock);
                runner->cb(entry->file_path ? entry->file_path : "<malformed>", HASHMONKE_HASH_MALFORMED);
                pthread_mutex_unlock(&runner->cb_lock);
            }
            continue;
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

        struct hashmonke_hash_ctrl ctrl;
        memset(&ctrl, 0, sizeof(ctrl));
        atomic_store(&ctrl.cancel, atomic_load(&runner->interrupted));

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

        if (hcode == HASHMONKE_HASH_INTERNAL_ERR)
        {
            atomic_store(&runner->fatal_error, true);
            atomic_store(&runner->interrupted, true);
            if (runner->cb)
            {
                pthread_mutex_lock(&runner->cb_lock);
                runner->cb(entry->file_path, hcode);
                pthread_mutex_unlock(&runner->cb_lock);
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
            pthread_mutex_lock(&runner->cb_lock);
            runner->cb(entry->file_path, hcode);
            pthread_mutex_unlock(&runner->cb_lock);
        }
    }

    if (hasher)
    {
        hashmonke_hasher_free(hasher);
    }
    atomic_fetch_sub(&runner->active_workers, 1);
    return NULL;
}

static bool spawn_worker(struct hashmonke_runner *runner)
{
    pthread_mutex_lock(&runner->thread_mgmt_lock);
    if (runner->total_workers_spawned >= runner->max_workers)
    {
        pthread_mutex_unlock(&runner->thread_mgmt_lock);
        return false;
    }

    atomic_fetch_add(&runner->active_workers, 1);
    pthread_t thread;
    if (pthread_create(&thread, NULL, worker_func, runner) != 0)
    {
        atomic_fetch_sub(&runner->active_workers, 1);
        atomic_store(&runner->fatal_error, true);
        atomic_store(&runner->interrupted, true);
        pthread_mutex_unlock(&runner->thread_mgmt_lock);
        return false;
    }

    runner->worker_threads[runner->total_workers_spawned++] = thread;
    pthread_mutex_unlock(&runner->thread_mgmt_lock);
    return true;
}

enum tuner_state
{
    STATE_BASELINE_W1,
    STATE_BASELINE_W2,
    STATE_SETTLING,
    STATE_MEASURE_W1,
    STATE_MEASURE_W2,
    STATE_LOCKED
};

static void *coordinator_func(void *arg)
{
    struct hashmonke_runner *runner = (struct hashmonke_runner *)arg;

    // Start with the requested worker count.
    uint32_t initial_workers = atomic_load(&runner->target_workers);
    for (uint32_t i = 0; i < initial_workers; ++i)
        spawn_worker(runner);

    enum tuner_state state = runner->thread_warmup ? STATE_BASELINE_W1 : STATE_LOCKED;
    double baseline_rate = 0.0;
    double b1_rate = 0.0;
    double m1_rate = 0.0;

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

        if (atomic_load(&runner->manifest_eof) || atomic_load(&runner->interrupted))
        {
            continue;
        }

        switch (state)
        {
        case STATE_BASELINE_W1:
            b1_rate = current_rate;
            state = STATE_BASELINE_W2;
            break;

        case STATE_BASELINE_W2: {
            double b2_rate = current_rate;
            baseline_rate = (b1_rate + b2_rate) / 2.0;

            if (atomic_load(&runner->active_workers) < runner->max_workers && !atomic_load(&runner->manifest_eof))
            {
                atomic_fetch_add(&runner->target_workers, 1);
                spawn_worker(runner);
                state = STATE_SETTLING;
            }
            else
            {
                state = STATE_LOCKED;
            }
            break;
        }

        case STATE_SETTLING:
            // Discard settling window
            state = STATE_MEASURE_W1;
            break;

        case STATE_MEASURE_W1:
            m1_rate = current_rate;
            state = STATE_MEASURE_W2;
            break;

        case STATE_MEASURE_W2: {
            double m2_rate = current_rate;
            double new_rate = (m1_rate + m2_rate) / 2.0;

            bool both_dropped_5pct = (baseline_rate > 0.0) && (m1_rate < baseline_rate * 0.95) &&
                                     (m2_rate < baseline_rate * 0.95);
            bool improvement_under_5pct = (baseline_rate > 0.0) && (new_rate < baseline_rate * 1.05);

            if (both_dropped_5pct || improvement_under_5pct)
            {
                // Backtrack 1 worker and lock
                uint32_t current_target = atomic_load(&runner->target_workers);
                if (current_target > 1)
                {
                    atomic_store(&runner->target_workers, current_target - 1);
                    atomic_fetch_add(&runner->retirement_requests, 1);
                }
                state = STATE_LOCKED;
            }
            else
            {
                // Improvement is >= 5%: keep worker and check if we can add more
                baseline_rate = new_rate;
                if (atomic_load(&runner->active_workers) < runner->max_workers && !atomic_load(&runner->manifest_eof))
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
    pthread_mutex_lock(&runner->thread_mgmt_lock);
    for (uint32_t i = 0; i < runner->total_workers_spawned; ++i)
    {
        pthread_join(runner->worker_threads[i], NULL);
    }
    runner->total_workers_spawned = 0;
    pthread_mutex_unlock(&runner->thread_mgmt_lock);

    atomic_store(&runner->is_finished, true);
    return NULL;
}

struct hashmonke_runner *hashmonke_runner_run_with_options(
    struct hashmonke_file *file, hashmonke_runner_cb cb, uint32_t starting_workers, bool thread_warmup)
{
    if (!file)
        return NULL;

    struct hashmonke_runner *runner = (struct hashmonke_runner *)calloc(1, sizeof(struct hashmonke_runner));
    if (!runner)
        return NULL;

    runner->file = file;
    runner->cb = cb;
    runner->thread_warmup = thread_warmup;
    atomic_store(&runner->min_hash_workers, UINT32_MAX);
    if (pthread_mutex_init(&runner->cb_lock, NULL) != 0)
    {
        free(runner);
        return NULL;
    }
    if (pthread_mutex_init(&runner->thread_mgmt_lock, NULL) != 0)
    {
        pthread_mutex_destroy(&runner->cb_lock);
        free(runner);
        return NULL;
    }
    if (pthread_mutex_init(&runner->join_lock, NULL) != 0)
    {
        pthread_mutex_destroy(&runner->thread_mgmt_lock);
        pthread_mutex_destroy(&runner->cb_lock);
        free(runner);
        return NULL;
    }

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

    runner->worker_threads = (pthread_t *)malloc(sizeof(pthread_t) * runner->max_workers);
    if (!runner->worker_threads)
    {
        pthread_mutex_destroy(&runner->join_lock);
        pthread_mutex_destroy(&runner->thread_mgmt_lock);
        pthread_mutex_destroy(&runner->cb_lock);
        free(runner);
        return NULL;
    }

    if (pthread_create(&runner->coordinator_thread, NULL, coordinator_func, runner) != 0)
    {
        free(runner->worker_threads);
        pthread_mutex_destroy(&runner->join_lock);
        pthread_mutex_destroy(&runner->thread_mgmt_lock);
        pthread_mutex_destroy(&runner->cb_lock);
        free(runner);
        return NULL;
    }

    return runner;
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
    pthread_mutex_lock(&runner->join_lock);
    if (!runner->coordinator_joined &&
        pthread_join(runner->coordinator_thread, NULL) == 0)
        runner->coordinator_joined = true;
    pthread_mutex_unlock(&runner->join_lock);
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

    pthread_mutex_destroy(&runner->cb_lock);
    pthread_mutex_destroy(&runner->thread_mgmt_lock);
    pthread_mutex_destroy(&runner->join_lock);
    free(runner->worker_threads);
    free(runner);
}
