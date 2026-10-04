#pragma once

#include "algo/md.h"
#include "file.h"
#include "hash.h"
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

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

struct hashmonke_hash_ctrl
{
    /* Controls */
    _Atomic bool cancel;

    /* Stats / observation */
    _Atomic size_t bytes_hashed;
    _Atomic size_t bytes_total;
    _Atomic size_t ms_elapsed;
    const char *file_path;
};

/** Hash a file at the given path.
 * @return a hash code representing an error, match/mismatch, or interruption.
 * @param ctrl optional control and stats block. If cancel is set, hashing aborts early.
 */
enum hashmonke_hash_code hashmonke_hasher_hash(struct hashmonke_hasher *hasher, const char *file_path, bool text_mode,
                                               const struct hashmonke_hash *expected,
                                               struct hashmonke_hash_ctrl *ctrl);