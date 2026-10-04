#pragma once
#include "hash.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum hashmonke_file_format
{
    HASHMONKE_FILE_FORMAT_SFV,
    HASHMONKE_FILE_FORMAT_GNU_MD5,
    HASHMONKE_FILE_FORMAT_BSD,
    HASHMONKE_FILE_FORMAT_AUTODETECT
};

enum hashmonke_entry_code
{
    HASHMONKE_ENTRY_OK,
    HASHMONKE_ENTRY_ERR,
};

struct hashmonke_file_entry
{
    const char *file_path;
    // Hash bytes converted from ASCII; size is determined by algo.
    uint8_t *hash;
    enum hashmonke_algo algo;
    enum hashmonke_entry_code code;
    bool text_mode;
};

struct hashmonke_file
{
    FILE *fp;
    enum hashmonke_file_format format;
    size_t num_entries;
    struct hashmonke_file_entry *entries;
};

/**
 * Reads in a file containing a list of hashes
 * if the provided format is AUTODETECT, will read lines and use heuristics to guess the file format.
 * can return NULL if file invalid
 */
struct hashmonke_file *hashmonke_file_init(const char *folder_path, const char *file_path,
                                           enum hashmonke_file_format format);
/**
 * Frees a hashmonke file and its entries
 */
void hashmonke_file_free(struct hashmonke_file *file);
