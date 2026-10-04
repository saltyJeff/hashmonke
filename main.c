#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "cli.h"
#include "file.h"
#include "hash.h"
#include "runner.h"
#include "tui_render.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t g_interrupted = 0;

static void sigint_handler(int sig)
{
    (void)sig;
    g_interrupted = 1;
}

static bool is_terminal(FILE *stream)
{
    return isatty(fileno(stream)) != 0;
}

static double monotonic_seconds(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
}

static void sleep_ms(unsigned int milliseconds)
{
    struct timespec duration;
    duration.tv_sec = milliseconds / 1000;
    duration.tv_nsec = (long)(milliseconds % 1000) * 1000000L;
    nanosleep(&duration, NULL);
}

static void wait_for_keypress(void)
{
    printf("Press enter to exit...");
    fflush(stdout);
    (void)getchar();
    putchar('\n');
}

static bool g_is_tty = false;
static bool g_no_tui = false;

static void file_status_callback(const char *file_path, enum hashmonke_hash_code code)
{
    if (g_is_tty)
    {
        fprintf(stderr, "\r\33[2K");
        fflush(stderr);
    }

    const char *status_str = "UNKNOWN";
    switch (code)
    {
    case HASHMONKE_HASH_MATCHES:
        status_str = "OK";
        break;
    case HASHMONKE_HASH_MISMATCH:
        status_str = "FAILED";
        break;
    case HASHMONKE_HASH_IO_ERR:
        status_str = "MISSING";
        break;
    case HASHMONKE_HASH_MALFORMED:
        status_str = "MALFORMED";
        break;
    case HASHMONKE_HASH_INTERNAL_ERR:
        status_str = "ERROR";
        break;
    case HASHMONKE_HASH_INTERRUPTED:
        status_str = "CANCELLED";
        break;
    }

    if (g_no_tui)
        printf("%s\t%s\n", status_str, file_path ? file_path : "<unknown>");
    else
        printf("%-10s %s\n", status_str, file_path ? file_path : "<unknown>");
    fflush(stdout);
}

int main(int argc, const char **argv)
{
    struct hashmonke_cli cli = hashmonke_parse_cli(argc, argv);

    if (cli.task == HASHMONKE_CLI_HELP)
    {
        hashmonke_print_help(argv[0]);
        return 0;
    }
    if (cli.task == HASHMONKE_CLI_ERR)
    {
        fprintf(stderr, "Error: %s\n", cli.err_msg ? cli.err_msg : "Invalid arguments.");
        return 2;
    }

    struct hashmonke_file *manifest = hashmonke_file_init(cli.folder, cli.hash_file, cli.format);
    if (!manifest)
    {
        fprintf(stderr, "Error: Failed to open or read checksum file '%s'.\n", cli.hash_file);
        return 2;
    }

    signal(SIGINT, sigint_handler);

    const char *slash = strrchr(cli.hash_file, '/');
    const char *backslash = strrchr(cli.hash_file, '\\');
    const char *sep = slash;
    if (!sep || (backslash && backslash > sep))
        sep = backslash;
    const char *manifest_title = sep ? sep + 1 : cli.hash_file;

    bool interactive_tty = is_terminal(stdout) && is_terminal(stderr);

    // Interactive TUI run
    if (!cli.no_tui && interactive_tty)
    {
        return tui_render_run(manifest, manifest_title, cli.starting_workers,
                              !cli.no_thread_warmup, cli.wait_for_input, &g_interrupted);
    }

    // Non-TUI / piped run
    g_no_tui = cli.no_tui;
    g_is_tty = !g_no_tui && interactive_tty;

    struct hashmonke_runner *runner = hashmonke_runner_run_with_options(
        manifest, file_status_callback, cli.starting_workers, !cli.no_thread_warmup);
    if (!runner)
    {
        fprintf(stderr, "Error: Failed to initialize runner.\n");
        hashmonke_file_free(manifest);
        return 2;
    }

    double start_time = monotonic_seconds();

    while (true)
    {
        struct hashmonke_runner_stats stats = hashmonke_runner_get_stats(runner);
        if (stats.is_finished || g_interrupted)
            break;

        if (g_is_tty)
        {
            fprintf(stderr,
                    "\r[Live: %u files (%u OK, %u fail) | %u workers | %.1f MB/s]",
                    stats.total_files_processed,
                    stats.files_matched,
                    stats.files_failed + stats.files_missing + stats.files_malformed,
                    stats.active_workers,
                    stats.current_throughput_mb_s);
            fflush(stderr);
        }
        sleep_ms(100);
    }

    if (g_interrupted)
        hashmonke_runner_interrupt(runner);
    hashmonke_runner_wait(runner);
    double elapsed_sec = monotonic_seconds() - start_time;

    if (g_is_tty)
    {
        fprintf(stderr, "\r\33[2K");
        fflush(stderr);
    }

    struct hashmonke_runner_stats final_stats = hashmonke_runner_get_stats(runner);

    if (elapsed_sec <= 0.0)
        elapsed_sec = 0.001;

    double total_mb = (double)final_stats.total_bytes_hashed / (1024.0 * 1024.0);
    double avg_throughput = total_mb / elapsed_sec;

    printf("\n============================================================\n");
    printf("Verification Summary:\n");
    printf("  Total Processed:       %u\n", final_stats.total_files_processed);
    printf("  Hashing Workers:       %u min / %u max\n",
           final_stats.max_hash_workers == 0 ? 0 : final_stats.min_hash_workers,
           final_stats.max_hash_workers);
    printf("  Matched:               %u\n", final_stats.files_matched);
    printf("  Failed / Mismatches:   %u\n", final_stats.files_failed);
    printf("  Missing / Unreadable:  %u\n", final_stats.files_missing);
    printf("  Malformed Records:     %u\n", final_stats.files_malformed);
    printf("  Total Data Hashed:     %.2f MB\n", total_mb);
    printf("  Elapsed Time:          %.2f s\n", elapsed_sec);
    printf("  Average Throughput:    %.2f MB/s\n", avg_throughput);
    printf("============================================================\n");
    fflush(stdout);

    int exit_code = 0;
    if (g_interrupted)
    {
        exit_code = 130;
    }
    else if (final_stats.has_error)
    {
        fprintf(stderr, "Error: Verification stopped before the manifest was fully processed.\n");
        exit_code = 2;
    }
    else if (final_stats.total_files_processed == 0)
    {
        fprintf(stderr, "Error: Checksum manifest contained no entries.\n");
        exit_code = 2;
    }
    else if (final_stats.files_failed > 0 || final_stats.files_missing > 0 || final_stats.files_malformed > 0)
    {
        exit_code = 1;
    }

    hashmonke_runner_free(runner);
    hashmonke_file_free(manifest);

    if (cli.wait_for_input)
        wait_for_keypress();

    return exit_code;
}

