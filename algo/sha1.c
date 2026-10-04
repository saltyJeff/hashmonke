#include "md_internal.h"
#include <stdlib.h>
#include <string.h>

#define SHA_CBLOCK 64
#define SHA_DIGEST_LENGTH 20

typedef struct sha_state_st {
    uint32_t h[5];
    uint32_t Nl, Nh;
    uint8_t data[SHA_CBLOCK];
    unsigned int num;
} SHA_CTX;

// --- Assembly hook or C fallback ---
#if !defined(SHA1_NO_ASM) && (defined(__x86_64__) || defined(_M_X64))
#define SHA1_ASM

void HASHMONKE_ASM_ABI sha1_block_asm_data_order(uint32_t *state, const uint8_t *data, size_t num);
#define sha1_block_data_order sha1_block_asm_data_order

#else // C Fallback

#define Xupdate(a, ix, ia, ib, ic, id)                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        (a) = ((ia) ^ (ib) ^ (ic) ^ (id));                                                                             \
        (ix) = (a) = CRYPTO_rotl_u32((a), 1);                                                                          \
    } while (0)

#define K_00_19 0x5a827999UL
#define K_20_39 0x6ed9eba1UL
#define K_40_59 0x8f1bbcdcUL
#define K_60_79 0xca62c1d6UL

#define F_00_19(b, c, d) ((((c) ^ (d)) & (b)) ^ (d))
#define F_20_39(b, c, d) ((b) ^ (c) ^ (d))
#define F_40_59(b, c, d) (((b) & (c)) | (((b) | (c)) & (d)))
#define F_60_79(b, c, d) F_20_39(b, c, d)

#define BODY_00_15(i, a, b, c, d, e, f, xi)                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        (f) = (xi) + (e) + K_00_19 + CRYPTO_rotl_u32((a), 5) + F_00_19((b), (c), (d));                                 \
        (b) = CRYPTO_rotl_u32((b), 30);                                                                                \
    } while (0)

#define BODY_16_19(i, a, b, c, d, e, f, xi, xa, xb, xc, xd)                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        Xupdate(f, xi, xa, xb, xc, xd);                                                                                \
        (f) += (e) + K_00_19 + CRYPTO_rotl_u32((a), 5) + F_00_19((b), (c), (d));                                       \
        (b) = CRYPTO_rotl_u32((b), 30);                                                                                \
    } while (0)

#define BODY_20_31(i, a, b, c, d, e, f, xi, xa, xb, xc, xd)                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        Xupdate(f, xi, xa, xb, xc, xd);                                                                                \
        (f) += (e) + K_20_39 + CRYPTO_rotl_u32((a), 5) + F_20_39((b), (c), (d));                                       \
        (b) = CRYPTO_rotl_u32((b), 30);                                                                                \
    } while (0)

#define BODY_32_39(i, a, b, c, d, e, f, xa, xb, xc, xd)                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        Xupdate(f, xa, xa, xb, xc, xd);                                                                                \
        (f) += (e) + K_20_39 + CRYPTO_rotl_u32((a), 5) + F_20_39((b), (c), (d));                                       \
        (b) = CRYPTO_rotl_u32((b), 30);                                                                                \
    } while (0)

#define BODY_40_59(i, a, b, c, d, e, f, xa, xb, xc, xd)                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        Xupdate(f, xa, xa, xb, xc, xd);                                                                                \
        (f) += (e) + K_40_59 + CRYPTO_rotl_u32((a), 5) + F_40_59((b), (c), (d));                                       \
        (b) = CRYPTO_rotl_u32((b), 30);                                                                                \
    } while (0)

#define BODY_60_79(i, a, b, c, d, e, f, xa, xb, xc, xd)                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        Xupdate(f, xa, xa, xb, xc, xd);                                                                                \
        (f) = (xa) + (e) + K_60_79 + CRYPTO_rotl_u32((a), 5) + F_60_79((b), (c), (d));                                 \
        (b) = CRYPTO_rotl_u32((b), 30);                                                                                \
    } while (0)

#define X(i) XX##i

static void HASHMONKE_ASM_ABI sha1_block_data_order(uint32_t *state, const uint8_t *data, size_t num)
{
    uint32_t A, B, C, D, E, T;
    uint32_t XX0, XX1, XX2, XX3, XX4, XX5, XX6, XX7, XX8, XX9, XX10, XX11, XX12, XX13, XX14, XX15;

    A = state[0];
    B = state[1];
    C = state[2];
    D = state[3];
    E = state[4];

    for (;;)
    {
        X(0) = CRYPTO_load_u32_be(data);
        data += 4;
        X(1) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(0, A, B, C, D, E, T, X(0));
        X(2) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(1, T, A, B, C, D, E, X(1));
        X(3) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(2, E, T, A, B, C, D, X(2));
        X(4) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(3, D, E, T, A, B, C, X(3));
        X(5) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(4, C, D, E, T, A, B, X(4));
        X(6) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(5, B, C, D, E, T, A, X(5));
        X(7) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(6, A, B, C, D, E, T, X(6));
        X(8) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(7, T, A, B, C, D, E, X(7));
        X(9) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(8, E, T, A, B, C, D, X(8));
        X(10) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(9, D, E, T, A, B, C, X(9));
        X(11) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(10, C, D, E, T, A, B, X(10));
        X(12) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(11, B, C, D, E, T, A, X(11));
        X(13) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(12, A, B, C, D, E, T, X(12));
        X(14) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(13, T, A, B, C, D, E, X(13));
        X(15) = CRYPTO_load_u32_be(data);
        data += 4;
        BODY_00_15(14, E, T, A, B, C, D, X(14));
        BODY_00_15(15, D, E, T, A, B, C, X(15));

        BODY_16_19(16, C, D, E, T, A, B, X(0), X(0), X(2), X(8), X(13));
        BODY_16_19(17, B, C, D, E, T, A, X(1), X(1), X(3), X(9), X(14));
        BODY_16_19(18, A, B, C, D, E, T, X(2), X(2), X(4), X(10), X(15));
        BODY_16_19(19, T, A, B, C, D, E, X(3), X(3), X(5), X(11), X(0));

        BODY_20_31(20, E, T, A, B, C, D, X(4), X(4), X(6), X(12), X(1));
        BODY_20_31(21, D, E, T, A, B, C, X(5), X(5), X(7), X(13), X(2));
        BODY_20_31(22, C, D, E, T, A, B, X(6), X(6), X(8), X(14), X(3));
        BODY_20_31(23, B, C, D, E, T, A, X(7), X(7), X(9), X(15), X(4));
        BODY_20_31(24, A, B, C, D, E, T, X(8), X(8), X(10), X(0), X(5));
        BODY_20_31(25, T, A, B, C, D, E, X(9), X(9), X(11), X(1), X(6));
        BODY_20_31(26, E, T, A, B, C, D, X(10), X(10), X(12), X(2), X(7));
        BODY_20_31(27, D, E, T, A, B, C, X(11), X(11), X(13), X(3), X(8));
        BODY_20_31(28, C, D, E, T, A, B, X(12), X(12), X(14), X(4), X(9));
        BODY_20_31(29, B, C, D, E, T, A, X(13), X(13), X(15), X(5), X(10));
        BODY_20_31(30, A, B, C, D, E, T, X(14), X(14), X(0), X(6), X(11));
        BODY_20_31(31, T, A, B, C, D, E, X(15), X(15), X(1), X(7), X(12));

        BODY_32_39(32, E, T, A, B, C, D, X(0), X(2), X(8), X(13));
        BODY_32_39(33, D, E, T, A, B, C, X(1), X(3), X(9), X(14));
        BODY_32_39(34, C, D, E, T, A, B, X(2), X(4), X(10), X(15));
        BODY_32_39(35, B, C, D, E, T, A, X(3), X(5), X(11), X(0));
        BODY_32_39(36, A, B, C, D, E, T, X(4), X(6), X(12), X(1));
        BODY_32_39(37, T, A, B, C, D, E, X(5), X(7), X(13), X(2));
        BODY_32_39(38, E, T, A, B, C, D, X(6), X(8), X(14), X(3));
        BODY_32_39(39, D, E, T, A, B, C, X(7), X(9), X(15), X(4));

        BODY_40_59(40, C, D, E, T, A, B, X(8), X(10), X(0), X(5));
        BODY_40_59(41, B, C, D, E, T, A, X(9), X(11), X(1), X(6));
        BODY_40_59(42, A, B, C, D, E, T, X(10), X(12), X(2), X(7));
        BODY_40_59(43, T, A, B, C, D, E, X(11), X(13), X(3), X(8));
        BODY_40_59(44, E, T, A, B, C, D, X(12), X(14), X(4), X(9));
        BODY_40_59(45, D, E, T, A, B, C, X(13), X(15), X(5), X(10));
        BODY_40_59(46, C, D, E, T, A, B, X(14), X(0), X(6), X(11));
        BODY_40_59(47, B, C, D, E, T, A, X(15), X(1), X(7), X(12));
        BODY_40_59(48, A, B, C, D, E, T, X(0), X(2), X(8), X(13));
        BODY_40_59(49, T, A, B, C, D, E, X(1), X(3), X(9), X(14));
        BODY_40_59(50, E, T, A, B, C, D, X(2), X(4), X(10), X(15));
        BODY_40_59(51, D, E, T, A, B, C, X(3), X(5), X(11), X(0));
        BODY_40_59(52, C, D, E, T, A, B, X(4), X(6), X(12), X(1));
        BODY_40_59(53, B, C, D, E, T, A, X(5), X(7), X(13), X(2));
        BODY_40_59(54, A, B, C, D, E, T, X(6), X(8), X(14), X(3));
        BODY_40_59(55, T, A, B, C, D, E, X(7), X(9), X(15), X(4));
        BODY_40_59(56, E, T, A, B, C, D, X(8), X(10), X(0), X(5));
        BODY_40_59(57, D, E, T, A, B, C, X(9), X(11), X(1), X(6));
        BODY_40_59(58, C, D, E, T, A, B, X(10), X(12), X(2), X(7));
        BODY_40_59(59, B, C, D, E, T, A, X(11), X(13), X(3), X(8));

        BODY_60_79(60, A, B, C, D, E, T, X(12), X(14), X(4), X(9));
        BODY_60_79(61, T, A, B, C, D, E, X(13), X(15), X(5), X(10));
        BODY_60_79(62, E, T, A, B, C, D, X(14), X(0), X(6), X(11));
        BODY_60_79(63, D, E, T, A, B, C, X(15), X(1), X(7), X(12));
        BODY_60_79(64, C, D, E, T, A, B, X(0), X(2), X(8), X(13));
        BODY_60_79(65, B, C, D, E, T, A, X(1), X(3), X(9), X(14));
        BODY_60_79(66, A, B, C, D, E, T, X(2), X(4), X(10), X(15));
        BODY_60_79(67, T, A, B, C, D, E, X(3), X(5), X(11), X(0));
        BODY_60_79(68, E, T, A, B, C, D, X(4), X(6), X(12), X(1));
        BODY_60_79(69, D, E, T, A, B, C, X(5), X(7), X(13), X(2));
        BODY_60_79(70, C, D, E, T, A, B, X(6), X(8), X(14), X(3));
        BODY_60_79(71, B, C, D, E, T, A, X(7), X(9), X(15), X(4));
        BODY_60_79(72, A, B, C, D, E, T, X(8), X(10), X(0), X(5));
        BODY_60_79(73, T, A, B, C, D, E, X(9), X(11), X(1), X(6));
        BODY_60_79(74, E, T, A, B, C, D, X(10), X(12), X(2), X(7));
        BODY_60_79(75, D, E, T, A, B, C, X(11), X(13), X(3), X(8));
        BODY_60_79(76, C, D, E, T, A, B, X(12), X(14), X(4), X(9));
        BODY_60_79(77, B, C, D, E, T, A, X(13), X(15), X(5), X(10));
        BODY_60_79(78, A, B, C, D, E, T, X(14), X(0), X(6), X(11));
        BODY_60_79(79, T, A, B, C, D, E, X(15), X(1), X(7), X(12));

        state[0] = (state[0] + E) & 0xffffffffL;
        state[1] = (state[1] + T) & 0xffffffffL;
        state[2] = (state[2] + A) & 0xffffffffL;
        state[3] = (state[3] + B) & 0xffffffffL;
        state[4] = (state[4] + C) & 0xffffffffL;

        if (--num == 0)
        {
            break;
        }

        A = state[0];
        B = state[1];
        C = state[2];
        D = state[3];
        E = state[4];
    }
}

#undef X
#undef Xupdate
#undef K_00_19
#undef K_20_39
#undef K_40_59
#undef K_60_79
#undef F_00_19
#undef F_20_39
#undef F_40_59
#undef F_60_79
#undef BODY_00_15
#undef BODY_16_19
#undef BODY_20_31
#undef BODY_32_39
#undef BODY_40_59
#undef BODY_60_79
#endif

// --- hashmonke_md implementation ---

static void sha1_update(struct hashmonke_md *md, const char *data, size_t len)
{
    SHA_CTX *c = (SHA_CTX *)md->ctx;
    crypto_md32_update(sha1_block_data_order, c->h, c->data, SHA_CBLOCK, &c->num, &c->Nh, &c->Nl, (const uint8_t *)data, len);
}

static const uint8_t *sha1_final(struct hashmonke_md *md)
{
    SHA_CTX *c = (SHA_CTX *)md->ctx;
    crypto_md32_final(sha1_block_data_order, c->h, c->data, SHA_CBLOCK, &c->num, c->Nh, c->Nl, /*is_big_endian=*/1);

    uint8_t *out = (uint8_t *)malloc(SHA_DIGEST_LENGTH);
    if (out) {
        CRYPTO_store_u32_be(out, c->h[0]);
        CRYPTO_store_u32_be(out + 4, c->h[1]);
        CRYPTO_store_u32_be(out + 8, c->h[2]);
        CRYPTO_store_u32_be(out + 12, c->h[3]);
        CRYPTO_store_u32_be(out + 16, c->h[4]);
    }

    free(c);
    free(md);
    return out;
}

struct hashmonke_md *hashmonke_md_sha1()
{
    struct hashmonke_md *md = (struct hashmonke_md *)malloc(sizeof(struct hashmonke_md));
    SHA_CTX *c = (SHA_CTX *)malloc(sizeof(SHA_CTX));

    if (!md || !c) {
        if (md) free(md);
        if (c) free(c);
        return NULL;
    }

    memset(c, 0, sizeof(SHA_CTX));
    c->h[0] = 0x67452301UL;
    c->h[1] = 0xefcdab89UL;
    c->h[2] = 0x98badcfeUL;
    c->h[3] = 0x10325476UL;
    c->h[4] = 0xc3d2e1f0UL;

    md->ctx = c;
    md->update = sha1_update;
    md->final = sha1_final;
    md->digest_size = SHA_DIGEST_LENGTH;

    return md;
}