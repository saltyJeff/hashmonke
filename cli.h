#pragma once

#include "file.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum hashmonke_cli_task
{
    HASHMONKE_CLI_VERIFY,
    HASHMONKE_CLI_HELP,
    HASHMONKE_CLI_ERR
};

struct hashmonke_cli
{
    enum hashmonke_cli_task task;
    const char *folder;
    const char *hash_file;
    enum hashmonke_file_format format;
    bool wait_for_input;
    bool no_tui;
    bool no_thread_warmup;
    uint32_t starting_workers;
    const char *err_msg;
};

struct hashmonke_cli hashmonke_parse_cli(int argc, const char **argv);
void hashmonke_print_help(const char *prog_name);

#ifdef __cplusplus
}
#endif
