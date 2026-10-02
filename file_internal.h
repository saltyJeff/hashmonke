#pragma once

#include "file.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#if defined(_WIN32) || defined(__MINGW32__)
#include <io.h>
#else
#include <unistd.h>
#endif
#if !defined(_WIN32) && !defined(__MINGW32__)
#include <errno.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline int hashmonke_open_entry(const char *path, bool binary_mode)
{
#if defined(_WIN32) || defined(__MINGW32__)
    return open(path, O_RDONLY | (binary_mode ? O_BINARY : O_TEXT));
#else
    (void)binary_mode;
    return open(path, O_RDONLY);
#endif
}

/* Read one complete text line, growing the buffer as needed. Returns 1 on a
 * line, 0 at clean EOF, and -1 on allocation or I/O failure. */
static inline int hashmonke_read_line(FILE *fp, char **line, size_t *capacity)
{
    size_t length = 0;
    int ch;
    while ((ch = fgetc(fp)) != EOF)
    {
        if (length + 1 >= *capacity)
        {
            size_t next_capacity = *capacity ? *capacity * 2 : 256;
            if (next_capacity <= *capacity)
                return -1;
            char *next = (char *)realloc(*line, next_capacity);
            if (!next)
                return -1;
            *line = next;
            *capacity = next_capacity;
        }
        (*line)[length++] = (char)ch;
        if (ch == '\n')
            break;
    }
    if (ch == EOF && ferror(fp))
        return -1;
    if (length == 0)
        return 0;
    (*line)[length] = '\0';
    return 1;
}

/* -------------------------------------------------------------------------
 * Lightweight, zero-allocation String Scanner
 * ------------------------------------------------------------------------- */
struct hashmonke_scanner
{
    const char *start;
    const char *cur;
    const char *end;
};

static inline void hashmonke_scanner_init(struct hashmonke_scanner *s, const char *str)
{
    s->start = str ? str : "";
    s->cur = s->start;
    s->end = s->start + strlen(s->start);
}

static inline bool hashmonke_scanner_eof(const struct hashmonke_scanner *s)
{
    return s->cur >= s->end;
}

static inline char hashmonke_scanner_peek(const struct hashmonke_scanner *s)
{
    return (s->cur < s->end) ? *s->cur : '\0';
}

static inline char hashmonke_scanner_next(struct hashmonke_scanner *s)
{
    return (s->cur < s->end) ? *(s->cur++) : '\0';
}

static inline size_t hashmonke_scanner_skip_ws(struct hashmonke_scanner *s)
{
    size_t count = 0;
    while (s->cur < s->end && (*s->cur == ' ' || *s->cur == '\t'))
    {
        s->cur++;
        count++;
    }
    return count;
}

static inline bool hashmonke_scanner_match_char(struct hashmonke_scanner *s, char expected)
{
    if (s->cur < s->end && *s->cur == expected)
    {
        s->cur++;
        return true;
    }
    return false;
}

static inline bool hashmonke_scanner_match_str(struct hashmonke_scanner *s, const char *prefix)
{
    size_t len = strlen(prefix);
    if ((size_t)(s->end - s->cur) >= len && strncmp(s->cur, prefix, len) == 0)
    {
        s->cur += len;
        return true;
    }
    return false;
}

static inline bool hashmonke_scanner_match_str_ci(struct hashmonke_scanner *s, const char *prefix)
{
    size_t len = strlen(prefix);
    if ((size_t)(s->end - s->cur) >= len && strncasecmp(s->cur, prefix, len) == 0)
    {
        s->cur += len;
        return true;
    }
    return false;
}

static inline size_t hashmonke_scanner_scan_hex_token(struct hashmonke_scanner *s, char *out, size_t out_sz)
{
    size_t len = 0;
    while (s->cur < s->end && isxdigit((unsigned char)*s->cur))
    {
        if (len + 1 < out_sz)
        {
            out[len] = *s->cur;
        }
        len++;
        s->cur++;
    }
    if (out_sz > 0)
    {
        size_t term_idx = (len < out_sz) ? len : (out_sz - 1);
        out[term_idx] = '\0';
    }
    return len;
}

static inline bool hashmonke_scanner_scan_fixed_hex(struct hashmonke_scanner *s, size_t expected_len, char *out,
                                                    size_t out_sz)
{
    if (expected_len + 1 > out_sz)
        return false;
    if ((size_t)(s->end - s->cur) < expected_len)
        return false;
    for (size_t i = 0; i < expected_len; ++i)
    {
        if (!isxdigit((unsigned char)s->cur[i]))
            return false;
        out[i] = s->cur[i];
    }
    out[expected_len] = '\0';
    s->cur += expected_len;
    return true;
}

static inline bool hashmonke_scanner_scan_until_str(struct hashmonke_scanner *s, const char *needle, char *out,
                                                    size_t out_sz)
{
    const char *pos = strstr(s->cur, needle);
    if (!pos || pos >= s->end)
        return false;
    size_t len = pos - s->cur;
    if (len + 1 > out_sz)
        return false;
    memcpy(out, s->cur, len);
    out[len] = '\0';
    s->cur = pos + strlen(needle);
    return true;
}

static inline bool hashmonke_scanner_scan_remaining(struct hashmonke_scanner *s, char *out, size_t out_sz)
{
    hashmonke_scanner_skip_ws(s);
    if (hashmonke_scanner_eof(s))
        return false;
    size_t len = s->end - s->cur;
    if (len + 1 > out_sz)
        return false;
    memcpy(out, s->cur, len);
    out[len] = '\0';
    s->cur = s->end;
    return true;
}

static inline bool hashmonke_scanner_split_trailing_token(const struct hashmonke_scanner *s, char *head_out,
                                                          size_t head_sz, char *tail_out, size_t tail_sz)
{
    if (s->cur >= s->end)
        return false;
    const char *last_ws = NULL;
    size_t remaining = (size_t)(s->end - s->cur);
    for (size_t i = remaining; i > 0; --i)
    {
        const char *p = s->cur + (i - 1);
        if (*p == ' ' || *p == '\t')
        {
            last_ws = p;
            break;
        }
    }
    if (!last_ws)
        return false;

    // Head is s->cur ... last_ws
    const char *head_start = s->cur;
    while (head_start < last_ws && (*head_start == ' ' || *head_start == '\t'))
        head_start++;
    const char *head_end = last_ws;
    while (head_end > head_start && (*(head_end - 1) == ' ' || *(head_end - 1) == '\t'))
        head_end--;
    size_t head_len = head_end - head_start;
    if (head_len == 0 || head_len + 1 > head_sz)
        return false;
    memcpy(head_out, head_start, head_len);
    head_out[head_len] = '\0';

    // Tail is last_ws + 1 ... s->end
    const char *tail_start = last_ws + 1;
    while (tail_start < s->end && (*tail_start == ' ' || *tail_start == '\t'))
        tail_start++;
    const char *tail_end = s->end;
    while (tail_end > tail_start && (*(tail_end - 1) == ' ' || *(tail_end - 1) == '\t'))
        tail_end--;
    size_t tail_len = tail_end - tail_start;
    if (tail_len == 0 || tail_len + 1 > tail_sz)
        return false;
    memcpy(tail_out, tail_start, tail_len);
    tail_out[tail_len] = '\0';

    return true;
}

/* -------------------------------------------------------------------------
 * Utility & Hex Helpers
 * ------------------------------------------------------------------------- */
static inline int hashmonke_hex_char_to_val(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static inline char *hashmonke_hex_to_bytes(const char *hex, size_t hex_len, size_t *out_len)
{
    if (hex_len % 2 != 0)
        return NULL;
    size_t n = hex_len / 2;
    char *bytes = (char *)malloc(n);
    if (!bytes)
        return NULL;
    for (size_t i = 0; i < n; ++i)
    {
        int hi = hashmonke_hex_char_to_val(hex[2 * i]);
        int lo = hashmonke_hex_char_to_val(hex[2 * i + 1]);
        if (hi < 0 || lo < 0)
        {
            free(bytes);
            return NULL;
        }
        bytes[i] = (char)((hi << 4) | lo);
    }
    if (out_len)
        *out_len = n;
    return bytes;
}

static inline char *hashmonke_unescape_path(const char *src)
{
    size_t len = strlen(src);
    char *out = (char *)malloc(len + 1);
    if (!out)
        return NULL;
    size_t j = 0;
    for (size_t i = 0; i < len; ++i)
    {
        if (src[i] == '\\' && i + 1 < len)
        {
            char next = src[i + 1];
            if (next == ' ')
            {
                out[j++] = ' ';
                i++;
            }
            else if (next == '\\')
            {
                out[j++] = '\\';
                i++;
            }
            else if (next == 'n')
            {
                out[j++] = '\n';
                i++;
            }
            else if (next == 'r')
            {
                out[j++] = '\r';
                i++;
            }
            else if (next == 't')
            {
                out[j++] = '\t';
                i++;
            }
            else
            {
                out[j++] = '\\';
            }
        }
        else
        {
            out[j++] = src[i];
        }
    }
    out[j] = '\0';
    return out;
}

static inline void hashmonke_strip_quotes(char *str)
{
    size_t len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"')
    {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

static inline char *hashmonke_canonical_path(const char *path)
{
    if (!path)
        return NULL;
#if defined(_WIN32) || defined(__MINGW32__)
    return _fullpath(NULL, path, 0);
#else
    char *res = realpath(path, NULL);
    if (!res)
    {
        // Preserve missing manifest targets while keeping the API's absolute-path contract.
        if (path[0] == '/')
            return strdup(path);

        size_t cwd_cap = 256;
        char *cwd = NULL;
        bool got_cwd = false;
        while (cwd_cap <= SIZE_MAX / 2)
        {
            char *next = (char *)realloc(cwd, cwd_cap);
            if (!next)
            {
                free(cwd);
                return NULL;
            }
            cwd = next;
            if (getcwd(cwd, cwd_cap))
            {
                got_cwd = true;
                break;
            }
            if (errno != ERANGE)
            {
                free(cwd);
                return NULL;
            }
            cwd_cap *= 2;
        }
        if (!got_cwd)
        {
            free(cwd);
            return NULL;
        }
        size_t cwd_len = strlen(cwd);
        size_t path_len = strlen(path);
        if (path_len > SIZE_MAX - 2 || cwd_len > SIZE_MAX - path_len - 2)
        {
            free(cwd);
            return NULL;
        }
        char *absolute = (char *)malloc(cwd_len + path_len + 2);
        if (absolute)
            snprintf(absolute, cwd_len + path_len + 2, "%s/%s", cwd, path);
        free(cwd);
        return absolute;
    }
    return res;
#endif
}

static inline char *hashmonke_resolve_entry_path(const char *base_dir, const char *rel_or_abs)
{
    if (!rel_or_abs || !*rel_or_abs)
        return NULL;

    // Check if rel_or_abs is already absolute (drive letter 'C:' or leading '/' or '\')
    bool is_abs = ((isalpha((unsigned char)rel_or_abs[0]) && rel_or_abs[1] == ':') ||
                   rel_or_abs[0] == '/' || rel_or_abs[0] == '\\');
    if (is_abs)
    {
        return hashmonke_canonical_path(rel_or_abs);
    }

    if (!base_dir || !*base_dir)
    {
        return hashmonke_canonical_path(rel_or_abs);
    }

    size_t base_len = strlen(base_dir);
    size_t rel_len = strlen(rel_or_abs);
    bool has_sep = (base_dir[base_len - 1] == '/' || base_dir[base_len - 1] == '\\');

    // Allocate exact length needed: base + separator + rel + null-terminator
    size_t extra_len = has_sep ? 1 : 2;
    if (rel_len > SIZE_MAX - extra_len || base_len > SIZE_MAX - rel_len - extra_len)
        return NULL;
    size_t alloc_sz = base_len + rel_len + extra_len;
    char *combined = (char *)malloc(alloc_sz);
    if (!combined)
        return NULL;

    if (has_sep)
        snprintf(combined, alloc_sz, "%s%s", base_dir, rel_or_abs);
    else
        snprintf(combined, alloc_sz, "%s/%s", base_dir, rel_or_abs);

    char *canonical = hashmonke_canonical_path(combined);
    free(combined);
    return canonical;
}


static inline char *hashmonke_trim_right(char *str)
{
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n' || isspace((unsigned char)str[len - 1])))
    {
        str[--len] = '\0';
    }
    return str;
}

static inline char *hashmonke_skip_left_space(const char *str)
{
    while (*str && isspace((unsigned char)*str))
        str++;
    return (char *)str;
}

static inline bool hashmonke_is_comment_or_empty(const char *str)
{
    while (*str && isspace((unsigned char)*str))
        str++;
    return (*str == '\0' || *str == ';' || *str == '#');
}

static inline bool hashmonke_str_ends_with_ci(const char *str, const char *suffix)
{
    size_t str_len = strlen(str);
    size_t sfx_len = strlen(suffix);
    if (str_len < sfx_len)
        return false;
    const char *p1 = str + str_len - sfx_len;
    const char *p2 = suffix;
    while (*p1 && *p2)
    {
        if (tolower((unsigned char)*p1) != tolower((unsigned char)*p2))
            return false;
        p1++;
        p2++;
    }
    return true;
}

bool hashmonke_parse_sfv(const char *line, const char *base_dir, struct hashmonke_hash_tuple *tup);
bool hashmonke_parse_gnu(const char *line, const char *base_dir, struct hashmonke_hash_tuple *tup);
bool hashmonke_parse_bsd(const char *line, const char *base_dir, struct hashmonke_hash_tuple *tup);

#ifdef __cplusplus
}
#endif
