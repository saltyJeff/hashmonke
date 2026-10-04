#pragma once

#include "algo/md.h"
#include "file.h"
#include "hash.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifndef __cplusplus
#include <stdatomic.h>
#endif

enum hashmonke_hash_code
{
    HASHMONKE_HASH_MATCHES,
    HASHMONKE_HASH_MISMATCH,
    HASHMONKE_HASH_IO_ERR,
    HASHMONKE_HASH_MALFORMED,
    HASHMONKE_HASH_INTERNAL_ERR,
    HASHMONKE_HASH_INTERRUPTED,
};

struct hashmonke_hasher;

/** instantiates a reusable hash thread pool. */
struct hashmonke_hasher *hashmonke_hasher_create(void);
void hashmonke_hasher_free(struct hashmonke_hasher *hasher);

struct hashmonke_hasher_progress_event
{
    const char *file_path;
    uint64_t bytes_hashed;
    uint64_t bytes_total;
};

struct hashmonke_hasher_progress_context;

typedef void (*hashmonke_hasher_progress_cb)(const struct hashmonke_hasher_progress_event *event,
                                             struct hashmonke_hasher_progress_context *context);

struct hashmonke_hash_ctrl
{
    /* Controls */
#ifdef __cplusplus
    volatile bool cancel;

    /* Stats / observation */
    volatile uint64_t bytes_hashed;
    volatile uint64_t bytes_total;
    volatile uint64_t ms_elapsed;
#else
    _Atomic bool cancel;

    /* Stats / observation */
    _Atomic uint64_t bytes_hashed;
    _Atomic uint64_t bytes_total;
    _Atomic uint64_t ms_elapsed;
#endif
    const char *file_path;

    /* Progress reporting */
    hashmonke_hasher_progress_cb progress_cb;
    struct hashmonke_hasher_progress_context *progress_context;
};

/** Hash a file at the given path.
 * @return a hash code representing an error, match/mismatch, or interruption.
 * @param ctrl optional control and stats block. If cancel is set, hashing aborts early.
 */
enum hashmonke_hash_code hashmonke_hasher_hash(struct hashmonke_hasher *hasher, const char *file_path, bool text_mode,
                                               const struct hashmonke_hash *expected,
                                               struct hashmonke_hash_ctrl *ctrl);