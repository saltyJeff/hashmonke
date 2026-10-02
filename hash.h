#pragma once

#include "algo/md.h"
#include "file.h"
#include <stddef.h>
#include <stdint.h>


enum hashmonke_hash_code
{
    HASHMONKE_HASH_MATCHES,
    HASHMONKE_HASH_MISMATCH,
    HASHMONKE_HASH_IO_ERR,
    HASHMONKE_HASH_MALFORMED,
    HASHMONKE_HASH_INTERNAL_ERR,
};

struct hashmonke_hasher;
typedef void (*hashmonke_hash_progress_cb)(size_t bytes, void *user_data);

struct hashmonke_hasher *hashmonke_hasher_create(void);
void hashmonke_hasher_free(struct hashmonke_hasher *hasher);

/** Hash an already-open descriptor from its current position. Ownership of
 * fd and the heap-allocated expected hash is transferred to this call; both
 * are released before return, including error returns. Progress callbacks run
 * on the compute thread after each chunk and are used only until return. */
enum hashmonke_hash_code hashmonke_hasher_hash(
    struct hashmonke_hasher *hasher, int fd, char *hash,
    enum hashmonke_algo algo, hashmonke_hash_progress_cb progress, void *user_data);

size_t hashmonke_hasher_get_last_bytes(struct hashmonke_hasher *hasher);
