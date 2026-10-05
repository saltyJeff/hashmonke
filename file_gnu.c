#include "file_internal.h"

bool hashmonke_parse_gnu(const char *line, const char *base_dir, struct hashmonke_file_entry *entry)
{
    const char *p = line;
    if (*p == '\\')
        p++;
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

    char *disp_path = NULL;
    char *abs_path = NULL;
    if (!hashmonke_process_entry_paths(base_dir, p, &disp_path, &abs_path))
        return false;

    size_t out_len = 0;
    uint8_t *bytes = hashmonke_hex_to_bytes(hex_start, hex_len, &out_len);
    if (!bytes)
    {
        free(abs_path);
        free(disp_path);
        return false;
    }

    entry->hash = bytes;
    entry->file_path = abs_path;
    entry->display_path = disp_path;
    entry->text_mode = !binary_mode;
    entry->algo = algo;
    return true;
}

