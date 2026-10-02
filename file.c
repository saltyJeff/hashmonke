#include "file_internal.h"

static enum hashmonke_file_format detect_format(FILE *fp, const char *path)
{
    if (hashmonke_str_ends_with_ci(path, ".sfv"))
        return HASHMONKE_FILE_FORMAT_SFV;
    if (hashmonke_str_ends_with_ci(path, ".md5") || hashmonke_str_ends_with_ci(path, ".sha1"))
        return HASHMONKE_FILE_FORMAT_GNU_MD5;

    long start = ftell(fp);
    if (start < 0) return HASHMONKE_FILE_FORMAT_AUTODETECT;
    char *line = NULL;
    size_t cap = 0;
    int status;
    enum hashmonke_file_format detected = HASHMONKE_FILE_FORMAT_GNU_MD5;
    while ((status = hashmonke_read_line(fp, &line, &cap)) > 0)
    {
        hashmonke_trim_right(line);
        if (hashmonke_is_comment_or_empty(line))
            continue;
        const char *p = hashmonke_skip_left_space(line);
        if ((strncasecmp(p, "MD5", 3) == 0 || strncasecmp(p, "SHA1", 4) == 0 ||
             strncasecmp(p, "SHA-1", 5) == 0 || strncasecmp(p, "CRC32", 5) == 0 ||
             strncasecmp(p, "SFV", 3) == 0) && strchr(p, '(') && strstr(p, ") ="))
            detected = HASHMONKE_FILE_FORMAT_BSD;
        else
        {
            if (*p == '\\') ++p;
            const char *q = p;
            while (isxdigit((unsigned char)*q)) ++q;
            size_t n = (size_t)(q - p);
            if ((n == 32 || n == 40) && (*q == ' ' || *q == '\t'))
                detected = HASHMONKE_FILE_FORMAT_GNU_MD5;
            else
                detected = HASHMONKE_FILE_FORMAT_SFV;
        }
        break;
    }
    free(line);
    fseek(fp, start, SEEK_SET);
    return (status < 0) ? HASHMONKE_FILE_FORMAT_AUTODETECT : detected;
}

static bool parse_entry(enum hashmonke_file_format format, const char *line,
                        const char *base_dir, struct hashmonke_file_entry *entry)
{
    memset(entry, 0, sizeof(*entry));
    switch (format)
    {
    case HASHMONKE_FILE_FORMAT_SFV: entry->code = hashmonke_parse_sfv(line, base_dir, entry) ? HASHMONKE_ENTRY_OK : HASHMONKE_ENTRY_ERR; break;
    case HASHMONKE_FILE_FORMAT_GNU_MD5: entry->code = hashmonke_parse_gnu(line, base_dir, entry) ? HASHMONKE_ENTRY_OK : HASHMONKE_ENTRY_ERR; break;
    case HASHMONKE_FILE_FORMAT_BSD: entry->code = hashmonke_parse_bsd(line, base_dir, entry) ? HASHMONKE_ENTRY_OK : HASHMONKE_ENTRY_ERR; break;
    default: entry->code = HASHMONKE_ENTRY_ERR; break;
    }
    if (entry->code != HASHMONKE_ENTRY_OK)
    {
        free((void *)entry->file_path);
        free((void *)entry->hash);
        entry->file_path = NULL;
        entry->hash = NULL;
    }
    return entry->code == HASHMONKE_ENTRY_OK;
}

static void clear_entry(struct hashmonke_file_entry *entry)
{
    free((void *)entry->file_path);
    free((void *)entry->hash);
    memset(entry, 0, sizeof(*entry));
}

struct hashmonke_file *hashmonke_file_init(const char *folder_path, const char *file_path,
                                           enum hashmonke_file_format format)
{
    if (!file_path) return NULL;
    FILE *fp = fopen(file_path, "rb");
    if (!fp) return NULL;

    struct hashmonke_file *file = calloc(1, sizeof(*file));
    if (!file) { fclose(fp); return NULL; }
    file->fp = fp;

    char *base_dir = NULL;
    if (folder_path && *folder_path)
        base_dir = hashmonke_canonical_path(folder_path);
    else
    {
        const char *slash = strrchr(file_path, '/');
        const char *backslash = strrchr(file_path, '\\');
        const char *sep = slash;
        if (!sep || (backslash && backslash > sep)) sep = backslash;
        if (sep)
        {
            size_t n = (size_t)(sep - file_path);
            char *dir = malloc(n + 1);
            if (dir) { memcpy(dir, file_path, n); dir[n] = '\0'; base_dir = hashmonke_canonical_path(dir); free(dir); }
        }
        if (!base_dir) base_dir = hashmonke_canonical_path(".");
    }
    if (!base_dir) { hashmonke_file_free(file); return NULL; }

    unsigned char bom[3];
    if (fread(bom, 1, 3, fp) != 3 || bom[0] != 0xef || bom[1] != 0xbb || bom[2] != 0xbf)
        rewind(fp);
    else
        fseek(fp, 3, SEEK_SET);

    if (format == HASHMONKE_FILE_FORMAT_AUTODETECT)
        format = detect_format(fp, file_path);
    if (format < HASHMONKE_FILE_FORMAT_SFV || format > HASHMONKE_FILE_FORMAT_BSD)
    { free(base_dir); hashmonke_file_free(file); return NULL; }
    file->format = format;

    char *line = NULL;
    size_t cap = 0;
    int status;
    size_t capacity = 0;
    while ((status = hashmonke_read_line(fp, &line, &cap)) > 0)
    {
        hashmonke_trim_right(line);
        if (hashmonke_is_comment_or_empty(line)) continue;
        struct hashmonke_file_entry entry;
        if (!parse_entry(format, line, base_dir, &entry))
            entry.code = HASHMONKE_ENTRY_ERR;
        if (file->num_entries == capacity)
        {
            size_t next = capacity ? capacity * 2 : 16;
            if (next < capacity || next > SIZE_MAX / sizeof(*file->entries))
            { clear_entry(&entry); free(line); free(base_dir); hashmonke_file_free(file); return NULL; }
            void *grown = realloc(file->entries, next * sizeof(*file->entries));
            if (!grown)
            { clear_entry(&entry); free(line); free(base_dir); hashmonke_file_free(file); return NULL; }
            file->entries = grown;
            capacity = next;
        }
        file->entries[file->num_entries++] = entry;
    }
    free(line);
    free(base_dir);
    if (status < 0 || ferror(fp)) { hashmonke_file_free(file); return NULL; }
    return file;
}

void hashmonke_file_free(struct hashmonke_file *file)
{
    if (!file) return;
    if (file->fp) fclose(file->fp);
    for (size_t i = 0; i < file->num_entries; ++i) clear_entry(&file->entries[i]);
    free(file->entries);
    free(file);
}
