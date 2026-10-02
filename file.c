#include "file_internal.h"
#include <pthread.h>

struct hashmonke_file
{
    FILE *fp;
    char *base_dir;
    enum hashmonke_file_format format;
    long start_offset;
    size_t current_line_num;
    pthread_mutex_t lock;
};

static enum hashmonke_file_format detect_file_format(FILE *fp, long start_offset, const char *file_path)
{
    if (hashmonke_str_ends_with_ci(file_path, ".sfv"))
    {
        return HASHMONKE_FILE_FORMAT_SFV;
    }
    if (hashmonke_str_ends_with_ci(file_path, ".md5") || hashmonke_str_ends_with_ci(file_path, ".sha1"))
    {
        return HASHMONKE_FILE_FORMAT_GNU_MD5;
    }

    // Inspect first valid line
    fseek(fp, start_offset, SEEK_SET);
    char *buf = NULL;
    size_t capacity = 0;
    int read_status;
    while ((read_status = hashmonke_read_line(fp, &buf, &capacity)) > 0)
    {
        char *line = hashmonke_trim_right(buf);
        if (hashmonke_is_comment_or_empty(line))
            continue;

        line = hashmonke_skip_left_space(line);
        // Check BSD
        if ((strncasecmp(line, "MD5", 3) == 0 || strncasecmp(line, "SHA1", 4) == 0 ||
             strncasecmp(line, "CRC32", 5) == 0 || strncasecmp(line, "SFV", 3) == 0) &&
            strstr(line, ") ="))
        {
            fseek(fp, start_offset, SEEK_SET);
            free(buf);
            return HASHMONKE_FILE_FORMAT_BSD;
        }

        // Check GNU
        const char *p = line;
        if (*p == '\\')
            p++;
        const char *hstart = p;
        while (*p && isxdigit((unsigned char)*p))
            p++;
        size_t hlen = p - hstart;
        if ((hlen == 32 || hlen == 40) && (*p == ' ' || *p == '\t'))
        {
            fseek(fp, start_offset, SEEK_SET);
            free(buf);
            return HASHMONKE_FILE_FORMAT_GNU_MD5;
        }

        // Check SFV (last token is 8 hex)
        size_t len = strlen(line);
        if (len >= 9)
        {
            const char *last_sp = strrchr(line, ' ');
            const char *last_tab = strrchr(line, '\t');
            const char *last = last_sp;
            if (!last || (last_tab && last_tab > last))
                last = last_tab;
            if (last && strlen(last + 1) == 8)
            {
                bool all_hex = true;
                for (int i = 0; i < 8; ++i)
                {
                    if (!isxdigit((unsigned char)last[1 + i]))
                    {
                        all_hex = false;
                        break;
                    }
                }
                if (all_hex)
                {
                    fseek(fp, start_offset, SEEK_SET);
                    free(buf);
                    return HASHMONKE_FILE_FORMAT_SFV;
                }
            }
        }
        break;
    }

    fseek(fp, start_offset, SEEK_SET);
    free(buf);
    return HASHMONKE_FILE_FORMAT_GNU_MD5;
}

struct hashmonke_file *hashmonke_file_init(const char *folder_path, const char *file_path,
                                           enum hashmonke_file_format format)
{
    if (!file_path)
        return NULL;

    FILE *fp = fopen(file_path, "rb");
    if (!fp)
        return NULL;

    struct hashmonke_file *f = (struct hashmonke_file *)calloc(1, sizeof(struct hashmonke_file));
    if (!f)
    {
        fclose(fp);
        return NULL;
    }
    f->fp = fp;
    if (pthread_mutex_init(&f->lock, NULL) != 0)
    {
        fclose(fp);
        free(f);
        return NULL;
    }
    f->current_line_num = 0;

    // Resolve base directory
    if (folder_path && *folder_path)
    {
        f->base_dir = hashmonke_canonical_path(folder_path);
    }
    else
    {
        const char *last_slash = strrchr(file_path, '/');
        const char *last_bslash = strrchr(file_path, '\\');
        const char *sep = last_slash;
        if (!sep || (last_bslash && last_bslash > sep))
            sep = last_bslash;
        if (sep)
        {
            size_t dlen = sep - file_path;
            char *dir_buf = (char *)malloc(dlen + 1);
            if (dir_buf)
            {
                memcpy(dir_buf, file_path, dlen);
                dir_buf[dlen] = '\0';
                f->base_dir = hashmonke_canonical_path(dir_buf);
                free(dir_buf);
            }
        }
        if (!f->base_dir)
        {
            f->base_dir = hashmonke_canonical_path(".");
        }
    }

    if (!f->base_dir)
    {
        fclose(f->fp);
        pthread_mutex_destroy(&f->lock);
        free(f);
        return NULL;
    }

    // Check UTF-8 BOM
    uint8_t bom[3];
    if (fread(bom, 1, 3, f->fp) == 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)
    {
        f->start_offset = 3;
    }
    else
    {
        f->start_offset = 0;
        fseek(f->fp, 0, SEEK_SET);
    }

    // Determine format
    if (format == HASHMONKE_FILE_FORMAT_AUTODETECT)
    {
        f->format = detect_file_format(f->fp, f->start_offset, file_path);
    }
    else
    {
        f->format = format;
    }

    return f;
}

void hashmonke_file_free(struct hashmonke_file *file)
{
    if (!file)
        return;
    if (file->fp)
    {
        fclose(file->fp);
        file->fp = NULL;
    }
    pthread_mutex_destroy(&file->lock);
    if (file->base_dir)
    {
        free(file->base_dir);
        file->base_dir = NULL;
    }
    free(file);
}

enum hashmonke_file_format hashmonke_file_get_format(struct hashmonke_file *file)
{
    return file ? file->format : HASHMONKE_FILE_FORMAT_AUTODETECT;
}

enum hashmonke_it_code hashmonke_file_next_tuple(struct hashmonke_file *file, struct hashmonke_hash_tuple *tup)
{
    if (!file || !tup)
        return HASHMONKE_IT_ERR;

    memset(tup, 0, sizeof(*tup));
    tup->file_fd = -1;

    pthread_mutex_lock(&file->lock);

    char *buf = NULL;
    size_t capacity = 0;
    int read_status;
    while ((read_status = hashmonke_read_line(file->fp, &buf, &capacity)) > 0)
    {
        file->current_line_num++;
        char *raw_copy = strdup(buf);
        char *line = hashmonke_trim_right(buf);

        if (hashmonke_is_comment_or_empty(line))
        {
            free(raw_copy);
            continue;
        }

        bool ok = false;
        switch (file->format)
        {
        case HASHMONKE_FILE_FORMAT_SFV:
            ok = hashmonke_parse_sfv(line, file->base_dir, tup);
            break;
        case HASHMONKE_FILE_FORMAT_GNU_MD5:
            ok = hashmonke_parse_gnu(line, file->base_dir, tup);
            break;
        case HASHMONKE_FILE_FORMAT_BSD:
            ok = hashmonke_parse_bsd(line, file->base_dir, tup);
            break;
        default:
            ok = hashmonke_parse_bsd(line, file->base_dir, tup) || hashmonke_parse_gnu(line, file->base_dir, tup) ||
                 hashmonke_parse_sfv(line, file->base_dir, tup);
            break;
        }

        tup->line_num = file->current_line_num;
        tup->raw_line = raw_copy;

        if (!ok)
        {
            tup->hash = NULL;
            tup->file_abs_path = NULL;
            tup->algo = (file->format == HASHMONKE_FILE_FORMAT_SFV) ? HASHMONKE_ALGO_CRC32 : HASHMONKE_ALGO_MD5;
            free(buf);
            pthread_mutex_unlock(&file->lock);
            return HASHMONKE_IT_MALFORMED;
        }

        free(buf);
        pthread_mutex_unlock(&file->lock);
        return HASHMONKE_IT_OK;
    }

    free(buf);
    if (read_status < 0 || ferror(file->fp))
    {
        pthread_mutex_unlock(&file->lock);
        return HASHMONKE_IT_ERR;
    }

    pthread_mutex_unlock(&file->lock);
    return HASHMONKE_IT_EOF;
}
