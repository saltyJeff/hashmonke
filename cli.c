#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

void hashmonke_print_help(const char *prog_name)
{
    printf("Hashmonke CLI Checksum Verifier\n\n");
    printf("Usage:\n");
    printf("  %s <checksum-file> [options]\n\n", prog_name ? prog_name : "hashmonke");
    printf("Options:\n");
    printf("  --folder <dir>        Base folder for relative paths (default: checksum file directory)\n");
    printf("  --format <fmt>        Checksum file format: auto, sfv, gnu, bsd (default: auto)\n");
    printf("  --starting-workers <n> Number of workers to start with (default: 1, max: 128)\n");
    printf("  --wfi, --wait-for-input  Wait for a key after showing the summary\n");
    printf("  --no-tui              Disable live status; print STATUS<TAB>FILE per entry\n");
    printf("  -h, --help            Show this help message and exit\n");
}

static bool str_case_eq(const char *s1, const char *s2)
{
    while (*s1 && *s2)
    {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? (*s1 + 32) : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? (*s2 + 32) : *s2;
        if (c1 != c2)
            return false;
        s1++;
        s2++;
    }
    return (*s1 == '\0' && *s2 == '\0');
}

struct hashmonke_cli hashmonke_parse_cli(int argc, const char **argv)
{
    struct hashmonke_cli res;
    memset(&res, 0, sizeof(res));
    res.task = HASHMONKE_CLI_VERIFY;
    res.format = HASHMONKE_FILE_FORMAT_AUTODETECT;
    res.starting_workers = 1;

    if (argc < 2)
    {
        res.task = HASHMONKE_CLI_ERR;
        res.err_msg = "No checksum file specified. Use --help for usage.";
        return res;
    }

    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];

        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
        {
            res.task = HASHMONKE_CLI_HELP;
            return res;
        }
        else if (strcmp(arg, "--wfi") == 0 || strcmp(arg, "--wait-for-input") == 0)
        {
            res.wait_for_input = true;
        }
        else if (strcmp(arg, "--no-tui") == 0)
        {
            res.no_tui = true;
        }
        else if (strcmp(arg, "--starting-workers") == 0 || strncmp(arg, "--starting-workers=", 19) == 0)
        {
            const char *value;
            if (arg[18] == '=')
                value = arg + 19;
            else if (i + 1 < argc)
                value = argv[++i];
            else
            {
                res.task = HASHMONKE_CLI_ERR;
                res.err_msg = "Option '--starting-workers' requires a value from 1 to 128.";
                return res;
            }

            char *end = NULL;
            errno = 0;
            unsigned long parsed = strtoul(value, &end, 10);
            if (errno != 0 || end == value || *end != '\0' || parsed == 0 || parsed > 128 || parsed > UINT_MAX)
            {
                res.task = HASHMONKE_CLI_ERR;
                res.err_msg = "Option '--starting-workers' must be an integer from 1 to 128.";
                return res;
            }
            res.starting_workers = (uint32_t)parsed;
        }
        else if (strcmp(arg, "--folder") == 0)
        {
            if (i + 1 >= argc)
            {
                res.task = HASHMONKE_CLI_ERR;
                res.err_msg = "Option '--folder' requires an argument.";
                return res;
            }
            res.folder = argv[++i];
        }
        else if (strncmp(arg, "--folder=", 9) == 0)
        {
            res.folder = arg + 9;
        }
        else if (strcmp(arg, "--format") == 0)
        {
            if (i + 1 >= argc)
            {
                res.task = HASHMONKE_CLI_ERR;
                res.err_msg = "Option '--format' requires an argument.";
                return res;
            }
            const char *fmt_str = argv[++i];
            if (str_case_eq(fmt_str, "auto"))
            {
                res.format = HASHMONKE_FILE_FORMAT_AUTODETECT;
            }
            else if (str_case_eq(fmt_str, "sfv"))
            {
                res.format = HASHMONKE_FILE_FORMAT_SFV;
            }
            else if (str_case_eq(fmt_str, "gnu") || str_case_eq(fmt_str, "md5"))
            {
                res.format = HASHMONKE_FILE_FORMAT_GNU_MD5;
            }
            else if (str_case_eq(fmt_str, "bsd"))
            {
                res.format = HASHMONKE_FILE_FORMAT_BSD;
            }
            else
            {
                res.task = HASHMONKE_CLI_ERR;
                res.err_msg = "Invalid format specified. Must be one of: auto, sfv, gnu, bsd.";
                return res;
            }
        }
        else if (strncmp(arg, "--format=", 9) == 0)
        {
            const char *fmt_str = arg + 9;
            if (str_case_eq(fmt_str, "auto"))
            {
                res.format = HASHMONKE_FILE_FORMAT_AUTODETECT;
            }
            else if (str_case_eq(fmt_str, "sfv"))
            {
                res.format = HASHMONKE_FILE_FORMAT_SFV;
            }
            else if (str_case_eq(fmt_str, "gnu") || str_case_eq(fmt_str, "md5"))
            {
                res.format = HASHMONKE_FILE_FORMAT_GNU_MD5;
            }
            else if (str_case_eq(fmt_str, "bsd"))
            {
                res.format = HASHMONKE_FILE_FORMAT_BSD;
            }
            else
            {
                res.task = HASHMONKE_CLI_ERR;
                res.err_msg = "Invalid format specified. Must be one of: auto, sfv, gnu, bsd.";
                return res;
            }
        }
        else if (arg[0] == '-')
        {
            res.task = HASHMONKE_CLI_ERR;
            res.err_msg = "Unrecognized option. Use --help for usage.";
            return res;
        }
        else
        {
            if (res.hash_file != NULL)
            {
                res.task = HASHMONKE_CLI_ERR;
                res.err_msg = "Multiple checksum files specified. Only one checksum file is allowed.";
                return res;
            }
            res.hash_file = arg;
        }
    }

    if (!res.hash_file && res.task == HASHMONKE_CLI_VERIFY)
    {
        res.task = HASHMONKE_CLI_ERR;
        res.err_msg = "No checksum file specified. Use --help for usage.";
    }

    return res;
}
