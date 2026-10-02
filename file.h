#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum hashmonke_file_format
{
    HASHMONKE_FILE_FORMAT_SFV,
    HASHMONKE_FILE_FORMAT_GNU_MD5,
    HASHMONKE_FILE_FORMAT_BSD,
    HASHMONKE_FILE_FORMAT_AUTODETECT
};

enum hashmonke_algo
{
    HASHMONKE_ALGO_MD5,
    HASHMONKE_ALGO_SHA1,
    HASHMONKE_ALGO_CRC32,
    HASHMONKE_ALGO_SFV = HASHMONKE_ALGO_CRC32
};

struct hashmonke_hash_tuple
{
    /** Hash bytes converted from ASCII; size is determined by algo. Heap
     * allocated; transfer to the fd hasher or free directly. NULL if malformed. */
    char *hash;
    /** Absolute file path, heap allocated; caller frees it. NULL if malformed. */
    char *file_abs_path;
    /** Open file descriptor for the entry, or -1 if opening failed. Not owned
     * by the tuple; callers must close it or transfer it to the fd hasher. */
    int file_fd;
    /** 1-based line number in manifest */
    size_t line_num;
    /** Raw manifest line, heap allocated; caller frees it. */
    char *raw_line;
    /** hashing algorithm to use */
    enum hashmonke_algo algo;
};

struct hashmonke_file;

/**
 * Reads in a file containing a list of hashes
 * if the provided format is AUTODETECT, will read lines and use heuristics to guess the file format.
 * can return NULL if file invalid
 */
struct hashmonke_file *hashmonke_file_init(const char *folder_path, const char *file_path,
                                           enum hashmonke_file_format format);
/**
 * Frees a hashmonke file
 */
void hashmonke_file_free(struct hashmonke_file *file);

enum hashmonke_it_code
{
    HASHMONKE_IT_OK,
    HASHMONKE_IT_EOF,
    HASHMONKE_IT_MALFORMED,
    HASHMONKE_IT_ERR
};

/**
 * Provides an iterator interface to get the next file to be hashed and verified.
 * Thread-safe via internal mutex.
 */
enum hashmonke_it_code hashmonke_file_next_tuple(struct hashmonke_file *file, struct hashmonke_hash_tuple *tup);

enum hashmonke_file_format hashmonke_file_get_format(struct hashmonke_file *file);

#ifdef __cplusplus
}
#endif
