#pragma once
#include "md.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef void (*hashmonke_md_update_func_t)(struct hashmonke_md *md, const char *data, size_t len);
typedef const uint8_t *(*hashmonke_md_final_func_t)(struct hashmonke_md *md);

struct hashmonke_md
{
    hashmonke_md_update_func_t update;
    hashmonke_md_final_func_t final;
    size_t digest_size;
    void *ctx;
};

static inline uint32_t CRYPTO_rotl_u32(uint32_t val, int shift) {
    return (val << shift) | (val >> (32 - shift));
}

static inline uint32_t CRYPTO_load_u32_le(const void *ptr) {
    uint32_t ret;
    memcpy(&ret, ptr, sizeof(ret));
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __builtin_bswap32(ret);
#else
    return ret;
#endif
}

static inline void CRYPTO_store_u32_le(void *ptr, uint32_t val) {
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    val = __builtin_bswap32(val);
#endif
    memcpy(ptr, &val, sizeof(val));
}

static inline uint32_t CRYPTO_load_u32_be(const void *ptr) {
    uint32_t ret;
    memcpy(&ret, ptr, sizeof(ret));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap32(ret);
#else
    return ret;
#endif
}

static inline void CRYPTO_store_u32_be(void *ptr, uint32_t val) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    val = __builtin_bswap32(val);
#endif
    memcpy(ptr, &val, sizeof(val));
}

#if defined(__x86_64__) || defined(_M_X64)
#if defined(__MINGW32__)
#define HASHMONKE_ASM_ABI __attribute__((sysv_abi))
#endif
#endif

#ifndef HASHMONKE_ASM_ABI
#define HASHMONKE_ASM_ABI
#endif

typedef void (HASHMONKE_ASM_ABI *crypto_md32_block_func)(uint32_t *state, const uint8_t *data, size_t num_blocks);

static inline void crypto_md32_update(crypto_md32_block_func block_func,
                                      uint32_t *h, uint8_t *data,
                                      size_t block_size, unsigned *num,
                                      uint32_t *Nh, uint32_t *Nl,
                                      const uint8_t *in, size_t len) {
    if (len == 0) {
        return;
    }

    uint32_t l = *Nl + (((uint32_t)len) << 3);
    if (l < *Nl) {
        (*Nh)++;
    }
    *Nh += (uint32_t)(len >> 29);
    *Nl = l;

    size_t n = *num;
    if (n != 0) {
        if (len >= block_size || len + n >= block_size) {
            memcpy(data + n, in, block_size - n);
            block_func(h, data, 1);
            n = block_size - n;
            in += n;
            len -= n;
            *num = 0;
            memset(data, 0, block_size);
        } else {
            memcpy(data + n, in, len);
            *num += (unsigned)len;
            return;
        }
    }

    n = len / block_size;
    if (n > 0) {
        block_func(h, in, n);
        n *= block_size;
        in += n;
        len -= n;
    }

    if (len != 0) {
        *num = (unsigned)len;
        memcpy(data, in, len);
    }
}

static inline void crypto_md32_final(crypto_md32_block_func block_func,
                                     uint32_t *h, uint8_t *data,
                                     size_t block_size, unsigned *num,
                                     uint32_t Nh, uint32_t Nl,
                                     int is_big_endian) {
    size_t n = *num;
    data[n] = 0x80;
    n++;

    if (n > block_size - 8) {
        memset(data + n, 0, block_size - n);
        n = 0;
        block_func(h, data, 1);
    }
    memset(data + n, 0, block_size - 8 - n);

    if (is_big_endian) {
        CRYPTO_store_u32_be(data + block_size - 8, Nh);
        CRYPTO_store_u32_be(data + block_size - 4, Nl);
    } else {
        CRYPTO_store_u32_le(data + block_size - 8, Nl);
        CRYPTO_store_u32_le(data + block_size - 4, Nh);
    }
    block_func(h, data, 1);
    *num = 0;
    memset(data, 0, block_size);
}
