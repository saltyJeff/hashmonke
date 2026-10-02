#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "hash.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32
#include <io.h>
#endif

#define HASHMONKE_BUFFER_SIZE (1 * 1024 * 1024)

struct hashmonke_hasher
{
    char *buf[2];
    size_t last_bytes;
};

struct slot
{
    char *data;
    size_t len;
    bool is_eof;
    bool is_err;
};

struct pipeline_ctx
{
    int fd;
    struct hashmonke_md *md;
    struct slot slots[2];
    int write_idx;
    int read_idx;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    bool io_error;
    size_t total_bytes;
    hashmonke_hash_progress_cb progress;
    void *progress_user_data;
};

static void *compute_worker(void *arg)
{
    struct pipeline_ctx *ctx = (struct pipeline_ctx *)arg;
    while (true)
    {
        pthread_mutex_lock(&ctx->lock);
        while (ctx->count == 0)
        {
            pthread_cond_wait(&ctx->not_empty, &ctx->lock);
        }
        int slot_idx = ctx->read_idx;
        size_t len = ctx->slots[slot_idx].len;
        bool is_eof = ctx->slots[slot_idx].is_eof;
        bool is_err = ctx->slots[slot_idx].is_err;
        char *data = ctx->slots[slot_idx].data;
        pthread_mutex_unlock(&ctx->lock);

        if (len > 0)
        {
            hashmonke_md_update_func(ctx->md, data, len);
            if (ctx->progress)
                ctx->progress(len, ctx->progress_user_data);
        }

        if (is_err)
            break;

        pthread_mutex_lock(&ctx->lock);
        ctx->read_idx = (ctx->read_idx + 1) % 2;
        ctx->count--;
        pthread_cond_signal(&ctx->not_full);
        pthread_mutex_unlock(&ctx->lock);

        if (is_eof)
        {
            break;
        }
    }
    return NULL;
}

struct hashmonke_hasher *hashmonke_hasher_create(void)
{
    struct hashmonke_hasher *hasher = (struct hashmonke_hasher *)calloc(1, sizeof(struct hashmonke_hasher));
    if (!hasher)
        return NULL;

    hasher->buf[0] = (char *)malloc(HASHMONKE_BUFFER_SIZE);
    hasher->buf[1] = (char *)malloc(HASHMONKE_BUFFER_SIZE);

    if (!hasher->buf[0] || !hasher->buf[1])
    {
        hashmonke_hasher_free(hasher);
        return NULL;
    }
    return hasher;
}

void hashmonke_hasher_free(struct hashmonke_hasher *hasher)
{
    if (!hasher)
        return;
    if (hasher->buf[0])
        free(hasher->buf[0]);
    if (hasher->buf[1])
        free(hasher->buf[1]);
    free(hasher);
}

size_t hashmonke_hasher_get_last_bytes(struct hashmonke_hasher *hasher)
{
    return hasher ? hasher->last_bytes : 0;
}

static struct hashmonke_md *create_md(enum hashmonke_algo algo)
{
    switch (algo)
    {
    case HASHMONKE_ALGO_MD5:
        return hashmonke_md_md5();
    case HASHMONKE_ALGO_SHA1:
        return hashmonke_md_sha1();
    case HASHMONKE_ALGO_CRC32:
        return hashmonke_md_crc32();
    default:
        return NULL;
    }
}

static enum hashmonke_hash_code hash_fd_core(
    struct hashmonke_hasher *hasher, int fd, const char *hash,
    enum hashmonke_algo algo, hashmonke_hash_progress_cb progress, void *user_data)
{
    if (!hasher)
    {
        if (fd >= 0)
            close(fd);
        return HASHMONKE_HASH_INTERNAL_ERR;
    }

    hasher->last_bytes = 0;
    if (fd < 0)
        return HASHMONKE_HASH_IO_ERR;

#if defined(POSIX_FADV_SEQUENTIAL)
    // Advisory only: unsupported filesystems/streams must still be hashable.
    (void)posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
#endif

    struct hashmonke_md *md = create_md(algo);
    if (!md)
    {
        close(fd);
        return HASHMONKE_HASH_INTERNAL_ERR;
    }

    struct pipeline_ctx ctx;
    ctx.fd = fd;
    ctx.md = md;
    ctx.slots[0].data = hasher->buf[0];
    ctx.slots[1].data = hasher->buf[1];
    ctx.write_idx = 0;
    ctx.read_idx = 0;
    ctx.count = 0;
    ctx.total_bytes = 0;
    ctx.io_error = false;
    ctx.progress = progress;
    ctx.progress_user_data = user_data;
    if (pthread_mutex_init(&ctx.lock, NULL) != 0)
    {
        close(fd);
        const char *digest = hashmonke_md_final_func(md);
        free((void *)digest);
        return HASHMONKE_HASH_INTERNAL_ERR;
    }
    if (pthread_cond_init(&ctx.not_full, NULL) != 0)
    {
        close(fd);
        pthread_mutex_destroy(&ctx.lock);
        const char *digest = hashmonke_md_final_func(md);
        free((void *)digest);
        return HASHMONKE_HASH_INTERNAL_ERR;
    }
    if (pthread_cond_init(&ctx.not_empty, NULL) != 0)
    {
        close(fd);
        pthread_cond_destroy(&ctx.not_full);
        pthread_mutex_destroy(&ctx.lock);
        const char *digest = hashmonke_md_final_func(md);
        free((void *)digest);
        return HASHMONKE_HASH_INTERNAL_ERR;
    }

    pthread_t worker;
    if (pthread_create(&worker, NULL, compute_worker, &ctx) != 0)
    {
        close(fd);
        const char *d = hashmonke_md_final_func(md);
        free((void *)d);
        pthread_mutex_destroy(&ctx.lock);
        pthread_cond_destroy(&ctx.not_full);
        pthread_cond_destroy(&ctx.not_empty);
        return HASHMONKE_HASH_INTERNAL_ERR;
    }

    while (true)
    {
        pthread_mutex_lock(&ctx.lock);
        while (ctx.count == 2)
        {
            pthread_cond_wait(&ctx.not_full, &ctx.lock);
        }
        int slot_idx = ctx.write_idx;
        pthread_mutex_unlock(&ctx.lock);

        size_t n = 0;
        bool is_eof = false;
        bool is_err = false;

        // Read directly into the pipeline buffer; short reads are not EOF.
        while (n < HASHMONKE_BUFFER_SIZE)
        {
            ssize_t bytes = read(ctx.fd, ctx.slots[slot_idx].data + n,
                                 HASHMONKE_BUFFER_SIZE - n);
            if (bytes > 0)
            {
                n += (size_t)bytes;
            }
            else if (bytes == 0)
            {
                is_eof = true;
                break;
            }
            else if (errno != EINTR)
            {
                is_err = true;
                ctx.io_error = true;
                break;
            }
        }

        pthread_mutex_lock(&ctx.lock);
        ctx.slots[slot_idx].len = n;
        ctx.slots[slot_idx].is_eof = is_eof;
        ctx.slots[slot_idx].is_err = is_err;
        ctx.total_bytes += n;
        ctx.write_idx = (ctx.write_idx + 1) % 2;
        ctx.count++;
        pthread_cond_signal(&ctx.not_empty);
        pthread_mutex_unlock(&ctx.lock);

        if (is_eof || is_err)
        {
            break;
        }
    }

    pthread_join(worker, NULL);
    close(fd);
    pthread_mutex_destroy(&ctx.lock);
    pthread_cond_destroy(&ctx.not_full);
    pthread_cond_destroy(&ctx.not_empty);

    hasher->last_bytes = ctx.total_bytes;

    size_t digest_size = hashmonke_md_digest_size(md);
    const char *computed = hashmonke_md_final_func(md);

    if (ctx.io_error)
    {
        if (computed)
            free((void *)computed);
        return HASHMONKE_HASH_IO_ERR;
    }

    if (!computed)
        return HASHMONKE_HASH_INTERNAL_ERR;

    bool matched = false;
    if (hash && computed)
    {
        matched = (memcmp(computed, hash, digest_size) == 0);
    }
    if (computed)
        free((void *)computed);

    return matched ? HASHMONKE_HASH_MATCHES : HASHMONKE_HASH_MISMATCH;
}

enum hashmonke_hash_code hashmonke_hasher_hash(
    struct hashmonke_hasher *hasher, int fd, char *hash,
    enum hashmonke_algo algo, hashmonke_hash_progress_cb progress, void *user_data)
{
    enum hashmonke_hash_code code = hash_fd_core(
        hasher, fd, hash, algo, progress, user_data);
    free(hash);
    return code;
}
