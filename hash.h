#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum hashmonke_algo
{
    HASHMONKE_ALGO_MD5,
    HASHMONKE_ALGO_SHA1,
    HASHMONKE_ALGO_SFV
};

struct hashmonke_hash
{
    enum hashmonke_algo algo;
    uint8_t value[];
};

static inline int hashmonke_hash_size(const struct hashmonke_hash *hash)
{
    static const int HASHMONKE_ALGO_SFV_SIZE = 4;
    static const int HASHMONKE_ALGO_MD5_SIZE = 16;
    static const int HASHMONKE_ALGO_SHA1_SIZE = 20;
    switch (hash->algo)
    {
    case HASHMONKE_ALGO_MD5: return HASHMONKE_ALGO_MD5_SIZE;
    case HASHMONKE_ALGO_SHA1: return HASHMONKE_ALGO_SHA1_SIZE;
    case HASHMONKE_ALGO_SFV: return HASHMONKE_ALGO_SFV_SIZE;
    default: return -1;
    }
}

static inline bool hashmonke_hash_matches(const struct hashmonke_hash *hash, const uint8_t *other, size_t other_len)
{
    int size = hashmonke_hash_size(hash);
    if (size < 0 || (size_t)size != other_len)
        return false;
    return memcmp(hash->value, other, size) == 0;
}