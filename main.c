#include "cli.h"
#include "file.h"
#include "hash.h"
#include "runner.h"
#include "tui_render.h"
#include <io.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

static volatile sig_atomic_t g_interrupted = 0;

static void sigint_handler(int sig)
{
    (void)sig;
    g_interrupted = 1;
}

static bool is_terminal(FILE *stream)
{
    return _isatty(_fileno(stream)) != 0;
}

static double monotonic_seconds(void)
{
    static LARGE_INTEGER freq;
    static int init = 0;
    if (!init)
    {
        QueryPerformanceFrequency(&freq);
        init = 1;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / (double)freq.QuadPart;
}

static void sleep_ms(unsigned int milliseconds)
{
    Sleep(milliseconds);
}

static void wait_for_keypress(void)
{
    printf("Press enter to exit...");
    fflush(stdout);
    (void)getchar();
    putchar('\n');
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

    // Non-TUI run: same state and runner, just no interactive TUI while hashing
    struct tui_render_state *state = tui_render_state_create(manifest, manifest_title);
    if (!state)
    {
        fprintf(stderr, "Error: Failed to initialize state.\n");
        hashmonke_file_free(manifest);
        return 2;
    }

    struct hashmonke_runner *runner = hashmonke_runner_run_with_events(
        manifest, NULL, tui_render_event_callback,
        (struct hashmonke_runner_event_context *)state,
        cli.starting_workers, !cli.no_thread_warmup);
    if (!runner)
    {
        fprintf(stderr, "Error: Failed to initialize runner.\n");
        tui_render_state_free(state);
        hashmonke_file_free(manifest);
        return 2;
    }

    double start_time = monotonic_seconds();

    while (true)
    {
        struct hashmonke_runner_stats stats = hashmonke_runner_get_stats(runner);
        if (stats.is_finished || g_interrupted)
            break;
        sleep_ms(50);
    }

    if (g_interrupted)
        hashmonke_runner_interrupt(runner);
    hashmonke_runner_wait(runner);

    double elapsed_sec = monotonic_seconds() - start_time;
    struct hashmonke_runner_stats final_stats = hashmonke_runner_get_stats(runner);

    // Single unified verification summary and exit code calculation
    int exit_code = print_verification_summary(state, &final_stats, elapsed_sec);
    if (g_interrupted)
        exit_code = 130;

    tui_render_state_free(state);
    hashmonke_runner_free(runner);
    hashmonke_file_free(manifest);

    if (cli.wait_for_input && !g_interrupted)
        wait_for_keypress();

    return exit_code;
}
