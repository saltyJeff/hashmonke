#pragma once

#include "file.h"
#include "hash.h"
#include "hasher.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct hashmonke_runner_stats
{
    uint64_t total_bytes_hashed;
    uint32_t active_workers;
    uint32_t min_hash_workers;
    uint32_t max_hash_workers;
    uint32_t files_matched;
    uint32_t files_failed;
    uint32_t files_missing;
    uint32_t files_malformed;
    uint32_t total_files_processed;
    double current_throughput_mb_s;
    bool is_finished;
    bool has_error;
};

struct hashmonke_runner;

typedef void (*hashmonke_runner_cb)(const char *file_abs_path, enum hashmonke_hash_code code);

struct hashmonke_runner *hashmonke_runner_run(struct hashmonke_file *file, hashmonke_runner_cb cb);
struct hashmonke_runner *hashmonke_runner_run_with_starting_workers(struct hashmonke_file *file, hashmonke_runner_cb cb,
                                                                    uint32_t starting_workers);
struct hashmonke_runner *hashmonke_runner_run_with_options(struct hashmonke_file *file, hashmonke_runner_cb cb,
                                                           uint32_t starting_workers, bool thread_warmup);
void hashmonke_runner_wait(struct hashmonke_runner *runner);
void hashmonke_runner_interrupt(struct hashmonke_runner *runner);
struct hashmonke_runner_stats hashmonke_runner_get_stats(struct hashmonke_runner *runner);
void hashmonke_runner_free(struct hashmonke_runner *runner);
