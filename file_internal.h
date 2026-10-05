#pragma once

#include "file.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <io.h>
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strdup
#define strdup _strdup
#endif

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

static inline uint8_t *hashmonke_hex_to_bytes(const char *hex, size_t hex_len, size_t *out_len)
{
    if (hex_len % 2 != 0)
        return NULL;
    size_t n = hex_len / 2;
    uint8_t *bytes = (uint8_t *)malloc(n);
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
        bytes[i] = (uint8_t)((hi << 4) | lo);
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
    return _fullpath(NULL, path, 0);
}

static inline char *hashmonke_resolve_entry_path(const char *base_dir, const char *rel_or_abs)
{
    if (!rel_or_abs || !*rel_or_abs)
        return NULL;

    // Check if rel_or_abs is already absolute (drive letter 'C:' or leading '/' or '\')
    bool is_abs = ((isalpha((unsigned char)rel_or_abs[0]) && rel_or_abs[1] == ':') || rel_or_abs[0] == '/' ||
                   rel_or_abs[0] == '\\');
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

static inline bool hashmonke_process_entry_paths(const char *base_dir, const char *raw_path,
                                                 char **out_display, char **out_abs)
{
    char *unescaped = hashmonke_unescape_path(raw_path);
    if (!unescaped)
        return false;
    hashmonke_strip_quotes(unescaped);

    char *disp_path = _strdup(unescaped);
    char *abs_path = hashmonke_resolve_entry_path(base_dir, unescaped);
    free(unescaped);
    if (!abs_path)
    {
        free(disp_path);
        return false;
    }
    *out_display = disp_path;
    *out_abs = abs_path;
    return true;
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

bool hashmonke_parse_sfv(const char *line, const char *base_dir, struct hashmonke_file_entry *entry);
bool hashmonke_parse_gnu(const char *line, const char *base_dir, struct hashmonke_file_entry *entry);
bool hashmonke_parse_bsd(const char *line, const char *base_dir, struct hashmonke_file_entry *entry);
