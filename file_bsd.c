#include "file_internal.h"

bool hashmonke_parse_bsd(const char *line, const char *base_dir, struct hashmonke_file_entry *entry)
{
    const char *p = hashmonke_skip_left_space(line);

    enum hashmonke_algo algo;
    size_t expected_hex_len;

    if (strncasecmp(p, "MD5", 3) == 0 && (p[3] == ' ' || p[3] == '('))
    {
        algo = HASHMONKE_ALGO_MD5;
        expected_hex_len = 32;
        p += 3;
    }
    else if (strncasecmp(p, "SHA1", 4) == 0 && (p[4] == ' ' || p[4] == '('))
    {
        algo = HASHMONKE_ALGO_SHA1;
        expected_hex_len = 40;
        p += 4;
    }
    else if (strncasecmp(p, "SHA-1", 5) == 0 && (p[5] == ' ' || p[5] == '('))
    {
        algo = HASHMONKE_ALGO_SHA1;
        expected_hex_len = 40;
        p += 5;
    }
    else if (strncasecmp(p, "CRC32", 5) == 0 && (p[5] == ' ' || p[5] == '('))
    {
        algo = HASHMONKE_ALGO_SFV;
        expected_hex_len = 8;
        p += 5;
    }
    else if (strncasecmp(p, "SFV", 3) == 0 && (p[3] == ' ' || p[3] == '('))
    {
        algo = HASHMONKE_ALGO_SFV;
        expected_hex_len = 8;
        p += 3;
    }
    else
    {
        return false;
    }

    p = hashmonke_skip_left_space(p);
    if (*p != '(')
        return false;
    p++;

    const char *eq = strstr(p, ") = ");
    if (!eq)
    {
        eq = strstr(p, ")=");
    }
    if (!eq)
        return false;

    size_t path_len = eq - p;
    const char *hex = strchr(eq, '=') + 1;
    hex = hashmonke_skip_left_space(hex);

    if (strlen(hex) != expected_hex_len)
        return false;
    for (size_t i = 0; i < expected_hex_len; ++i)
    {
        if (!isxdigit((unsigned char)hex[i]))
            return false;
    }

    if (path_len == SIZE_MAX)
        return false;
    char *path_buf = (char *)malloc(path_len + 1);
    if (!path_buf)
        return false;
    memcpy(path_buf, p, path_len);
    path_buf[path_len] = '\0';
    char *unescaped = hashmonke_unescape_path(path_buf);
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
    uint8_t *bytes = hashmonke_hex_to_bytes(hex, expected_hex_len, &out_len);
    if (!bytes)
    {
        free(abs_path);
        free(disp_path);
        return false;
    }

    entry->hash = bytes;
    entry->file_path = abs_path;
    entry->display_path = disp_path;
    entry->text_mode = false;
    entry->algo = algo;
    return true;
}

