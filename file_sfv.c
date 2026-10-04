#include "file_internal.h"

bool hashmonke_parse_sfv(const char *line, const char *base_dir, struct hashmonke_file_entry *entry)
{
    size_t len = strlen(line);
    if (len < 9)
        return false;

    // Find the last whitespace delimiter
    const char *last_space = NULL;
    for (ptrdiff_t i = (ptrdiff_t)len - 1; i >= 0; --i)
    {
        if (line[i] == ' ' || line[i] == '\t')
        {
            last_space = &line[i];
            break;
        }
    }
    if (!last_space)
        return false;

    const char *hex = last_space + 1;
    if (strlen(hex) != 8)
        return false;
    for (int i = 0; i < 8; ++i)
    {
        if (!isxdigit((unsigned char)hex[i]))
            return false;
    }

    size_t path_len = last_space - line;
    if (path_len == SIZE_MAX)
        return false;
    char *path_buf = (char *)malloc(path_len + 1);
    if (!path_buf)
        return false;
    memcpy(path_buf, line, path_len);
    path_buf[path_len] = '\0';
    hashmonke_trim_right(path_buf);
    char *p = hashmonke_skip_left_space(path_buf);
    if (*p == '\0')
    {
        free(path_buf);
        return false;
    }

    char *unescaped = hashmonke_unescape_path(p);
    free(path_buf);
    if (!unescaped)
        return false;
    hashmonke_strip_quotes(unescaped);

    char *disp_path = strdup(unescaped);
    char *abs_path = hashmonke_resolve_entry_path(base_dir, unescaped);
    free(unescaped);
    if (!abs_path)
    {
        free(disp_path);
        return false;
    }

    size_t out_len = 0;
    uint8_t *bytes = hashmonke_hex_to_bytes(hex, 8, &out_len);
    if (!bytes || out_len != 4)
    {
        if (bytes)
            free(bytes);
        free(abs_path);
        free(disp_path);
        return false;
    }

    entry->hash = bytes;
    entry->file_path = abs_path;
    entry->display_path = disp_path;
    entry->text_mode = false;
    entry->algo = HASHMONKE_ALGO_SFV;
    return true;
}

