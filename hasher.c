#include "hasher.h"
#include "time_util.h"
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>

#define HASHMONKE_BUFFER_SIZE (1 * 1024 * 1024)

struct hashmonke_hasher
{
    char *buf[2];
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
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE not_full;
    CONDITION_VARIABLE not_empty;
    bool io_error;
    bool interrupted;
    size_t total_bytes;
    struct hashmonke_hash_ctrl *ctrl;
    uint64_t start_ms;
};


static DWORD WINAPI compute_worker(LPVOID arg)
{
    struct pipeline_ctx *ctx = (struct pipeline_ctx *)arg;
    while (true)
    {
        EnterCriticalSection(&ctx->lock);
        while (ctx->count == 0)
        {
            SleepConditionVariableCS(&ctx->not_empty, &ctx->lock, INFINITE);
        }
        int slot_idx = ctx->read_idx;
        size_t len = ctx->slots[slot_idx].len;
        bool is_eof = ctx->slots[slot_idx].is_eof;
        bool is_err = ctx->slots[slot_idx].is_err;
        char *data = ctx->slots[slot_idx].data;
        LeaveCriticalSection(&ctx->lock);

        if (len > 0 && !ctx->interrupted)
        {
            if (ctx->ctrl && atomic_load(&ctx->ctrl->cancel))
            {
                ctx->interrupted = true;
            }
            else
            {
                hashmonke_md_update_func(ctx->md, data, len);
                if (ctx->ctrl)
                {
                    atomic_fetch_add(&ctx->ctrl->bytes_hashed, len);
                    uint64_t now = hashmonke_monotonic_ms();
                    atomic_store(&ctx->ctrl->ms_elapsed, (size_t)(now >= ctx->start_ms ? now - ctx->start_ms : 0));
                    if (ctx->ctrl->progress_cb)
                    {
                        struct hashmonke_hasher_progress_event pe;
                        pe.file_path = ctx->ctrl->file_path;
                        pe.bytes_chunk = len;
                        pe.bytes_hashed = atomic_load(&ctx->ctrl->bytes_hashed);
                        pe.bytes_total = atomic_load(&ctx->ctrl->bytes_total);
                        ctx->ctrl->progress_cb(&pe, ctx->ctrl->progress_context);
                    }
                }
            }
        }

        if (is_err || ctx->interrupted)
            break;

        EnterCriticalSection(&ctx->lock);
        ctx->read_idx = (ctx->read_idx + 1) % 2;
        ctx->count--;
        WakeConditionVariable(&ctx->not_full);
        LeaveCriticalSection(&ctx->lock);

        if (is_eof)
        {
            break;
        }
    }
    return 0;
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

static struct hashmonke_md *create_md(enum hashmonke_algo algo)
{
    switch (algo)
    {
    case HASHMONKE_ALGO_MD5:
        return hashmonke_md_md5();
    case HASHMONKE_ALGO_SHA1:
        return hashmonke_md_sha1();
    case HASHMONKE_ALGO_SFV:
        return hashmonke_md_crc32();
    default:
        return NULL;
    }
}

enum hashmonke_hash_code hashmonke_hasher_hash(struct hashmonke_hasher *hasher, const char *file_path, bool text_mode,
                                               const struct hashmonke_hash *expected,
                                               struct hashmonke_hash_ctrl *ctrl)
{
    if (!hasher || !file_path)
        return HASHMONKE_HASH_INTERNAL_ERR;

    if (!expected)
        return HASHMONKE_HASH_INTERNAL_ERR;

    if (ctrl && atomic_load(&ctrl->cancel))
        return HASHMONKE_HASH_INTERRUPTED;

    int flags = O_RDONLY;
#ifdef _O_BINARY
    flags |= (text_mode ? 0 : _O_BINARY);
#elif defined(O_BINARY)
    flags |= (text_mode ? 0 : O_BINARY);
#endif

    int fd = _open(file_path, flags);
    if (fd < 0)
        return HASHMONKE_HASH_IO_ERR;

    uint64_t start_ms = hashmonke_monotonic_ms();

    if (ctrl)
    {
        ctrl->file_path = file_path;
        atomic_store(&ctrl->bytes_hashed, 0);
        atomic_store(&ctrl->ms_elapsed, 0);
        struct _stat64 st;
        if (_fstat64(fd, &st) == 0 && (st.st_mode & _S_IFREG))
            atomic_store(&ctrl->bytes_total, (uint64_t)st.st_size);
        else
            atomic_store(&ctrl->bytes_total, 0);

        if (ctrl->progress_cb)
        {
            struct hashmonke_hasher_progress_event pe;
            pe.file_path = file_path;
            pe.bytes_chunk = 0;
            pe.bytes_hashed = 0;
            pe.bytes_total = atomic_load(&ctrl->bytes_total);
            ctrl->progress_cb(&pe, ctrl->progress_context);
        }
    }

    struct hashmonke_md *md = create_md(expected->algo);
    if (!md)
    {
        _close(fd);
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
    ctx.interrupted = false;
    ctx.ctrl = ctrl;
    ctx.start_ms = start_ms;

    InitializeCriticalSection(&ctx.lock);
    InitializeConditionVariable(&ctx.not_full);
    InitializeConditionVariable(&ctx.not_empty);

    HANDLE worker = CreateThread(NULL, 0, compute_worker, &ctx, 0, NULL);
    if (!worker)
    {
        _close(fd);
        const uint8_t *d = hashmonke_md_final_func(md);
        free((void *)d);
        DeleteCriticalSection(&ctx.lock);
        return HASHMONKE_HASH_INTERNAL_ERR;
    }

    while (true)
    {
        if (ctrl && atomic_load(&ctrl->cancel))
        {
            ctx.interrupted = true;
            break;
        }

        EnterCriticalSection(&ctx.lock);
        while (ctx.count == 2)
        {
            SleepConditionVariableCS(&ctx.not_full, &ctx.lock, INFINITE);
        }
        int slot_idx = ctx.write_idx;
        LeaveCriticalSection(&ctx.lock);

        if (ctrl && atomic_load(&ctrl->cancel))
        {
            ctx.interrupted = true;
            break;
        }

        size_t n = 0;
        bool is_eof = false;
        bool is_err = false;

        // Read directly into the pipeline buffer; short reads are not EOF.
        while (n < HASHMONKE_BUFFER_SIZE)
        {
            if (ctrl && atomic_load(&ctrl->cancel))
            {
                ctx.interrupted = true;
                break;
            }

            int bytes = _read(ctx.fd, ctx.slots[slot_idx].data + n,
                              (unsigned int)(HASHMONKE_BUFFER_SIZE - n));
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

        if (ctx.interrupted)
        {
            break;
        }

        EnterCriticalSection(&ctx.lock);
        ctx.slots[slot_idx].len = n;
        ctx.slots[slot_idx].is_eof = is_eof;
        ctx.slots[slot_idx].is_err = is_err;
        ctx.total_bytes += n;
        ctx.write_idx = (ctx.write_idx + 1) % 2;
        ctx.count++;
        WakeConditionVariable(&ctx.not_empty);
        LeaveCriticalSection(&ctx.lock);

        if (is_eof || is_err)
        {
            break;
        }
    }

    // Wake up compute worker if interrupted
    if (ctx.interrupted)
    {
        EnterCriticalSection(&ctx.lock);
        int slot_idx = ctx.write_idx;
        ctx.slots[slot_idx].len = 0;
        ctx.slots[slot_idx].is_eof = true;
        ctx.slots[slot_idx].is_err = false;
        ctx.write_idx = (ctx.write_idx + 1) % 2;
        ctx.count++;
        WakeConditionVariable(&ctx.not_empty);
        LeaveCriticalSection(&ctx.lock);
    }

    WaitForSingleObject(worker, INFINITE);
    CloseHandle(worker);
    _close(fd);
    DeleteCriticalSection(&ctx.lock);

    size_t digest_size = hashmonke_md_digest_size(md);
    const uint8_t *computed = hashmonke_md_final_func(md);

    if (ctrl)
    {
        uint64_t now = hashmonke_monotonic_ms();
        atomic_store(&ctrl->ms_elapsed, (size_t)(now >= start_ms ? now - start_ms : 0));
    }

    if (ctx.interrupted)
    {
        if (computed)
            free((void *)computed);
        return HASHMONKE_HASH_INTERRUPTED;
    }

    if (ctx.io_error)
    {
        if (computed)
            free((void *)computed);
        return HASHMONKE_HASH_IO_ERR;
    }

    if (!computed)
        return HASHMONKE_HASH_INTERNAL_ERR;

    bool matched = hashmonke_hash_matches(expected, computed, digest_size);
    free((void *)computed);

    return matched ? HASHMONKE_HASH_MATCHES : HASHMONKE_HASH_MISMATCH;
}

