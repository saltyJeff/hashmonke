#include "tui_render.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

enum tui_row_status
{
    TUI_ROW_PENDING = 0,
    TUI_ROW_IN_PROGRESS,
    TUI_ROW_MATCHED,
    TUI_ROW_MISMATCH,
    TUI_ROW_MISSING,
    TUI_ROW_ERROR,
};

struct tui_file_row_state
{
    size_t line_number;
    const char *display_path;
    const char *file_path;
    enum tui_row_status status;
    uint64_t bytes_processed;
    uint64_t file_size;
    double throughput_mb_s;
};

struct tui_render_state
{
    CRITICAL_SECTION lock;
    struct tui_file_row_state *rows;
    size_t num_rows;
    const char *manifest_title;
    int scroll_top;
    double start_time;
};

static double render_monotonic_seconds(void)
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

static void format_size_human(uint64_t bytes, char *buf, size_t buf_sz)
{
    if (bytes >= 1024ULL * 1024ULL * 1024ULL)
        snprintf(buf, buf_sz, "%.1f GB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    else if (bytes >= 1024ULL * 1024ULL)
        snprintf(buf, buf_sz, "%.1f MB", (double)bytes / (1024.0 * 1024.0));
    else if (bytes >= 1024ULL)
        snprintf(buf, buf_sz, "%.1f KB", (double)bytes / 1024.0);
    else
        snprintf(buf, buf_sz, "%llu B", (unsigned long long)bytes);
}

struct tui_render_state *tui_render_state_create(const struct hashmonke_file *manifest,
                                                 const char *manifest_title)
{
    if (!manifest)
        return NULL;

    struct tui_render_state *state = (struct tui_render_state *)calloc(1, sizeof(struct tui_render_state));
    if (!state)
        return NULL;

    InitializeCriticalSection(&state->lock);

    state->manifest_title = manifest_title ? manifest_title : "manifest";
    state->num_rows = manifest->num_entries;
    state->scroll_top = 0;
    state->start_time = render_monotonic_seconds();

    if (state->num_rows > 0)
    {
        state->rows = (struct tui_file_row_state *)calloc(state->num_rows, sizeof(struct tui_file_row_state));
        if (!state->rows)
        {
            DeleteCriticalSection(&state->lock);
            free(state);
            return NULL;
        }

        for (size_t i = 0; i < state->num_rows; ++i)
        {
            state->rows[i].line_number = manifest->entries[i].line_number;
            state->rows[i].display_path = manifest->entries[i].display_path ? manifest->entries[i].display_path : manifest->entries[i].file_path;
            state->rows[i].file_path = manifest->entries[i].file_path;
            state->rows[i].status = TUI_ROW_PENDING;
        }
    }

    return state;
}

void tui_render_state_free(struct tui_render_state *state)
{
    if (!state)
        return;
    DeleteCriticalSection(&state->lock);
    free(state->rows);
    free(state);
}

void tui_render_event_callback(const struct hashmonke_runner_event *event,
                               struct hashmonke_runner_event_context *context)
{
    struct tui_render_state *state = (struct tui_render_state *)context;
    if (!state || event->entry_index >= state->num_rows)
        return;

    EnterCriticalSection(&state->lock);
    struct tui_file_row_state *r = &state->rows[event->entry_index];
    if (event->type == HASHMONKE_RUNNER_EVENT_START)
    {
        if (event->status == HASHMONKE_HASH_MALFORMED)
            r->status = TUI_ROW_ERROR;
        else
            r->status = TUI_ROW_IN_PROGRESS;
        r->bytes_processed = 0;
        r->file_size = 0;
        r->throughput_mb_s = 0.0;
    }
    else if (event->type == HASHMONKE_RUNNER_EVENT_PROGRESS)
    {
        r->status = TUI_ROW_IN_PROGRESS;
        r->bytes_processed = event->bytes_processed;
        r->file_size = event->file_size;
        r->throughput_mb_s = event->throughput_mb_s;
    }
    else if (event->type == HASHMONKE_RUNNER_EVENT_COMPLETE)
    {
        switch (event->status)
        {
        case HASHMONKE_HASH_MATCHES:
            r->status = TUI_ROW_MATCHED;
            break;
        case HASHMONKE_HASH_MISMATCH:
            r->status = TUI_ROW_MISMATCH;
            break;
        case HASHMONKE_HASH_IO_ERR:
            r->status = TUI_ROW_MISSING;
            break;
        default:
            r->status = TUI_ROW_ERROR;
            break;
        }
        r->bytes_processed = event->bytes_processed;
        r->file_size = event->file_size;
        r->throughput_mb_s = event->throughput_mb_s;
    }
    LeaveCriticalSection(&state->lock);
}

bool tui_render_handle_key(struct tui *tui, struct tui_render_state *state,
                           enum tui_key key)
{
    if (!tui || !state)
        return false;

    int rows = 24, cols = 80;
    tui_get_size(tui, &rows, &cols);
    int visible_rows = rows - 7;
    if (visible_rows < 1)
        visible_rows = 1;
    int max_scroll = (int)state->num_rows - visible_rows;
    if (max_scroll < 0)
        max_scroll = 0;

    switch (key)
    {
    case TUI_KEY_UP:
    case TUI_KEY_MOUSE_WHEEL_UP:
        if (state->scroll_top > 0)
            state->scroll_top--;
        return true;

    case TUI_KEY_DOWN:
    case TUI_KEY_MOUSE_WHEEL_DOWN:
        if (state->scroll_top < max_scroll)
            state->scroll_top++;
        return true;

    case TUI_KEY_PAGE_UP:
        state->scroll_top -= (visible_rows > 1 ? visible_rows - 1 : 1);
        if (state->scroll_top < 0)
            state->scroll_top = 0;
        return true;

    case TUI_KEY_PAGE_DOWN:
        state->scroll_top += (visible_rows > 1 ? visible_rows - 1 : 1);
        if (state->scroll_top > max_scroll)
            state->scroll_top = max_scroll;
        return true;

    case TUI_KEY_HOME:
        state->scroll_top = 0;
        return true;

    case TUI_KEY_END:
        state->scroll_top = max_scroll;
        return true;

    default:
        return false;
    }
}

void tui_render_frame(struct tui *tui, struct tui_render_state *state,
                      const struct hashmonke_runner_stats *stats)
{
    if (!tui || !state || !stats)
        return;

    tui_begin_frame(tui);

    int rows = 24, cols = 80;
    tui_get_size(tui, &rows, &cols);

    // Title
    tui_start_line(tui);
    tui_text_styled(tui, "Hashmonke · ", "\033[1m");
    tui_text_styled(tui, state->manifest_title ? state->manifest_title : "manifest", "\033[1m");
    tui_end_line(tui);

    // Blank line
    tui_start_line(tui);
    tui_end_line(tui);

    // Progress
    tui_start_line(tui);
    tui_text(tui, "Progress  ");
    int bar_width = 26;
    if (cols < 70)
        bar_width = 16;
    if (cols < 50)
        bar_width = 10;
    tui_progress_bar(tui, stats->total_files_processed, state->num_rows, bar_width);
    int pct = state->num_rows > 0 ? (int)((stats->total_files_processed * 100ULL) / state->num_rows) : 100;
    char prog_stats[64];
    snprintf(prog_stats, sizeof(prog_stats), "  %d%% · %u/%zu", pct, stats->total_files_processed, state->num_rows);
    tui_text(tui, prog_stats);
    tui_end_line(tui);

    // Files
    tui_start_line(tui);
    tui_text(tui, "Files     ");
    char file_stats[128];
    snprintf(file_stats, sizeof(file_stats), "\033[32m✓\033[0m %u  \033[31m✗\033[0m %u  \033[33m?\033[0m %u  \033[31;1m!\033[0m %u",
             stats->files_matched, stats->files_failed, stats->files_missing, stats->files_malformed);
    tui_text(tui, file_stats);
    tui_end_line(tui);

    // Workers
    tui_start_line(tui);
    tui_text(tui, "Workers   ");
    char worker_stats[64];
    uint32_t min_w = stats->min_hash_workers == UINT32_MAX ? 0 : stats->min_hash_workers;
    if (stats->is_finished)
    {
        if (min_w > 0 && stats->max_hash_workers > min_w)
            snprintf(worker_stats, sizeof(worker_stats), "min %u · max %u · finished",
                     min_w, stats->max_hash_workers);
        else
            snprintf(worker_stats, sizeof(worker_stats), "%u · finished", stats->max_hash_workers);
    }
    else
    {
        if (min_w > 0 && stats->max_hash_workers > min_w)
            snprintf(worker_stats, sizeof(worker_stats), "min %u · max %u · current %u",
                     min_w, stats->max_hash_workers, stats->active_workers);
        else
            snprintf(worker_stats, sizeof(worker_stats), "%u active", stats->active_workers);
    }
    tui_text(tui, worker_stats);
    tui_end_line(tui);

    // Total
    tui_start_line(tui);
    tui_text(tui, "Total     ");
    char total_stats[64];
    double rate = stats->current_throughput_mb_s;
    if (rate <= 0.0 && stats->total_bytes_hashed > 0 && state && state->start_time > 0.0)
    {
        double elapsed = render_monotonic_seconds() - state->start_time;
        if (elapsed > 0.0)
            rate = ((double)stats->total_bytes_hashed / (1024.0 * 1024.0)) / elapsed;
    }

    if (stats->is_finished)
    {
        if (stats->min_throughput_mb_s > 0.0 && stats->max_throughput_mb_s > stats->min_throughput_mb_s + 0.05)
        {
            snprintf(total_stats, sizeof(total_stats), "min %.1f · max %.1f · avg %.1f MB/s",
                     stats->min_throughput_mb_s, stats->max_throughput_mb_s, rate);
        }
        else if (rate > 0.0)
        {
            snprintf(total_stats, sizeof(total_stats), "avg %.1f MB/s", rate);
        }
        else
        {
            snprintf(total_stats, sizeof(total_stats), "0.0 MB/s");
        }
    }
    else
    {
        if (stats->min_throughput_mb_s > 0.0 && stats->max_throughput_mb_s > stats->min_throughput_mb_s + 0.05)
        {
            snprintf(total_stats, sizeof(total_stats), "min %.1f · max %.1f · current %.1f MB/s",
                     stats->min_throughput_mb_s, stats->max_throughput_mb_s, rate);
        }
        else if (rate > 0.0)
        {
            snprintf(total_stats, sizeof(total_stats), "current %.1f MB/s", rate);
        }
        else
        {
            snprintf(total_stats, sizeof(total_stats), "-- MB/s");
        }
    }
    tui_text(tui, total_stats);
    tui_end_line(tui);

    // Blank line
    tui_start_line(tui);
    tui_end_line(tui);

    // File rows
    int visible_rows = rows - 7;
    if (visible_rows < 1)
        visible_rows = 1;
    int max_scroll = (int)state->num_rows - visible_rows;
    if (max_scroll < 0)
        max_scroll = 0;
    if (state->scroll_top > max_scroll)
        state->scroll_top = max_scroll;
    if (state->scroll_top < 0)
        state->scroll_top = 0;

    EnterCriticalSection(&state->lock);
    size_t end_idx = (size_t)(state->scroll_top + visible_rows);
    if (end_idx > state->num_rows)
        end_idx = state->num_rows;

    for (size_t i = (size_t)state->scroll_top; i < end_idx; ++i)
    {
        const struct tui_file_row_state *r = &state->rows[i];
        const char *marker = " ";
        const char *style = NULL;
        switch (r->status)
        {
        case TUI_ROW_MATCHED:
            marker = "✓";
            style = "\033[32m";
            break;
        case TUI_ROW_MISMATCH:
            marker = "✗";
            style = "\033[31m";
            break;
        case TUI_ROW_MISSING:
            marker = "?";
            style = "\033[33m";
            break;
        case TUI_ROW_ERROR:
            marker = "!";
            style = "\033[31;1m";
            break;
        case TUI_ROW_IN_PROGRESS:
            marker = "◯";
            style = "\033[36m";
            break;
        case TUI_ROW_PENDING:
        default:
            marker = " ";
            style = NULL;
            break;
        }

        char right_stats[128] = "";
        if (r->status == TUI_ROW_IN_PROGRESS)
        {
            char proc_buf[32];
            char tot_buf[32];
            format_size_human(r->bytes_processed, proc_buf, sizeof(proc_buf));
            format_size_human(r->file_size, tot_buf, sizeof(tot_buf));

            if (r->file_size > 0)
            {
                int pct_file = (int)((r->bytes_processed * 100ULL) / r->file_size);
                if (pct_file > 100)
                    pct_file = 100;
                snprintf(right_stats, sizeof(right_stats), "%.1f MB/s · %s/%s (%d%%)",
                         r->throughput_mb_s, proc_buf, tot_buf, pct_file);
            }
            else
            {
                snprintf(right_stats, sizeof(right_stats), "%.1f MB/s · %s",
                         r->throughput_mb_s, proc_buf);
            }
        }

        tui_file_row(tui, marker, style, r->display_path, right_stats);
    }
    LeaveCriticalSection(&state->lock);

    tui_end_frame(tui);
}

static void render_sleep_ms(unsigned int ms)
{
    Sleep(ms);
}

int print_verification_summary(const struct tui_render_state *state,
                               const struct hashmonke_runner_stats *final_stats,
                               double elapsed_sec)
{
    if (elapsed_sec <= 0.0)
        elapsed_sec = 0.001;

    double total_mb = (double)final_stats->total_bytes_hashed / (1024.0 * 1024.0);
    double avg_throughput = total_mb / elapsed_sec;

    printf("\n============================================================\n");
    printf("Verification Summary:\n");
    printf("  Total Processed:       %u\n", final_stats->total_files_processed);
    printf("  Hashing Workers:       %u min / %u max\n",
           final_stats->max_hash_workers == 0 ? 0 : final_stats->min_hash_workers,
           final_stats->max_hash_workers);
    printf("  Matched:               %u\n", final_stats->files_matched);
    printf("  Failed / Mismatches:   %u\n", final_stats->files_failed);
    printf("  Missing / Unreadable:  %u\n", final_stats->files_missing);
    printf("  Malformed Records:     %u\n", final_stats->files_malformed);
    printf("  Total Data Hashed:     %.2f MB\n", total_mb);
    printf("  Elapsed Time:          %.2f s\n", elapsed_sec);
    printf("  Average Throughput:    %.2f MB/s\n", avg_throughput);
    printf("============================================================\n");

    if (state)
    {
        for (size_t i = 0; i < state->num_rows; ++i)
        {
            const struct tui_file_row_state *r = &state->rows[i];
            const char *st = "err";
            switch (r->status)
            {
            case TUI_ROW_MATCHED:
                st = "match";
                break;
            case TUI_ROW_MISMATCH:
                st = "mismatch";
                break;
            case TUI_ROW_MISSING:
                st = "missing";
                break;
            case TUI_ROW_ERROR:
            default:
                st = "err";
                break;
            }
            const char *p = r->display_path ? r->display_path
                            : (r->file_path ? r->file_path : "<unknown>");
            printf("%-10s %s\n", st, p);
        }
    }
    fflush(stdout);

    if (final_stats->has_error)
    {
        fprintf(stderr, "Error: Verification stopped before the manifest was fully processed.\n");
        return 2;
    }
    if (final_stats->total_files_processed == 0)
    {
        fprintf(stderr, "Error: Checksum manifest contained no entries.\n");
        return 2;
    }
    if (final_stats->files_failed > 0 || final_stats->files_missing > 0 || final_stats->files_malformed > 0)
    {
        return 1;
    }
    return 0;
}

int tui_render_run(struct hashmonke_file *manifest, const char *manifest_title,
                   uint32_t starting_workers, bool thread_warmup,
                   bool wait_for_input, volatile sig_atomic_t *interrupted)
{
    struct tui *tui = tui_init();
    if (!tui)
        return 2;

    struct tui_render_state *state = tui_render_state_create(manifest, manifest_title);
    if (!state)
    {
        tui_shutdown(tui);
        return 2;
    }

    struct hashmonke_runner *runner = hashmonke_runner_run_with_events(
        manifest,
        NULL,
        tui_render_event_callback,
        (struct hashmonke_runner_event_context *)state,
        starting_workers,
        thread_warmup);

    if (!runner)
    {
        tui_render_state_free(state);
        tui_shutdown(tui);
        return 2;
    }

    double start_time = render_monotonic_seconds();

    while (true)
    {
        struct hashmonke_runner_stats stats = hashmonke_runner_get_stats(runner);
        if (stats.is_finished || (interrupted && *interrupted))
            break;

        enum tui_key k = tui_poll_key(tui);
        if (k == TUI_KEY_QUIT)
        {
            if (interrupted)
                *interrupted = 1;
            break;
        }

        tui_render_handle_key(tui, state, k);
        tui_render_frame(tui, state, &stats);
        render_sleep_ms(30);
    }

    if (interrupted && *interrupted)
        hashmonke_runner_interrupt(runner);
    hashmonke_runner_wait(runner);
    double elapsed_sec = render_monotonic_seconds() - start_time;

    struct hashmonke_runner_stats final_stats = hashmonke_runner_get_stats(runner);

    // Final frame
    tui_render_frame(tui, state, &final_stats);

    if (wait_for_input && (!interrupted || !*interrupted))
    {
        while (!interrupted || !*interrupted)
        {
            enum tui_key k = tui_poll_key(tui);
            if (k == TUI_KEY_ENTER || k == TUI_KEY_QUIT)
                break;
            tui_render_handle_key(tui, state, k);
            tui_render_frame(tui, state, &final_stats);
            render_sleep_ms(30);
        }
    }

    tui_shutdown(tui);

    int exit_code = print_verification_summary(state, &final_stats, elapsed_sec);
    if (interrupted && *interrupted)
        exit_code = 130;

    tui_render_state_free(state);
    hashmonke_runner_free(runner);
    hashmonke_file_free(manifest);
    return exit_code;
}

