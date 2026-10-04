#include "file_internal.h"

bool hashmonke_parse_gnu(const char *line, const char *base_dir, struct hashmonke_file_entry *entry)
{
    const char *p = line;
    bool is_escaped_line = false;
    if (*p == '\\')
    {
        is_escaped_line = true;
        p++;
    }
    p = hashmonke_skip_left_space(p);

    const char *hex_start = p;
    while (*p && isxdigit((unsigned char)*p))
        p++;
    size_t hex_len = p - hex_start;
    if (hex_len != 32 && hex_len != 40)
        return false;

    enum hashmonke_algo algo = (hex_len == 32) ? HASHMONKE_ALGO_MD5 : HASHMONKE_ALGO_SHA1;

    if (*p != ' ' && *p != '\t')
        return false;
    p++;

    bool binary_mode = false;
    if (*p == '*' || *p == ' ' || *p == '?')
    {
        binary_mode = (*p == '*');
        p++;
    }

    while (*p == ' ' || *p == '\t')
        p++;
    if (*p == '\0')
        return false;

    (void)is_escaped_line;
    char *unescaped = hashmonke_unescape_path(p);
    if (!unescaped)
        return false;
    hashmonke_strip_quotes(unescaped);

    char *abs_path = hashmonke_resolve_entry_path(base_dir, unescaped);
    free(unescaped);
    if (!abs_path)
        return false;

    size_t out_len = 0;
    uint8_t *bytes = hashmonke_hex_to_bytes(hex_start, hex_len, &out_len);
    if (!bytes)
    {
        free(abs_path);
        return false;
    }

    entry->hash = bytes;
    entry->file_path = abs_path;
    entry->text_mode = !binary_mode;
    entry->algo = algo;
    return true;
}

