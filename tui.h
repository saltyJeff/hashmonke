#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum tui_key
{
    TUI_KEY_NONE = 0,
    TUI_KEY_UP,
    TUI_KEY_DOWN,
    TUI_KEY_PAGE_UP,
    TUI_KEY_PAGE_DOWN,
    TUI_KEY_HOME,
    TUI_KEY_END,
    TUI_KEY_MOUSE_WHEEL_UP,
    TUI_KEY_MOUSE_WHEEL_DOWN,
    TUI_KEY_ENTER,
    TUI_KEY_QUIT,
    TUI_KEY_OTHER,
};

struct tui;

/**
 * Initializes terminal in alternate screen buffer, raw mode, mouse reporting enabled.
 * Returns NULL if stdout is not a terminal or initialization fails.
 */
struct tui *tui_init(void);

/**
 * Restores original terminal modes, mouse reporting, and screen buffer.
 */
void tui_shutdown(struct tui *tui);

/**
 * Gets current terminal dimensions (rows, cols).
 */
void tui_get_size(struct tui *tui, int *rows, int *cols);

/**
 * Starts a new frame. Resets the internal rendering buffer and positions cursor at (1,1).
 */
void tui_begin_frame(struct tui *tui);

/**
 * Finishes the frame and flushes the entire screen buffer in a single write.
 */
void tui_end_frame(struct tui *tui);

/**
 * Immediate-mode line and text rendering calls.
 */
void tui_start_line(struct tui *tui);
void tui_text(struct tui *tui, const char *str);
void tui_text_styled(struct tui *tui, const char *str, const char *ansi_style);
void tui_progress_bar(struct tui *tui, uint64_t completed, uint64_t total, int bar_width);
void tui_end_line(struct tui *tui);

/**
 * Table and row helpers.
 */
void tui_stat_row(struct tui *tui, const char *label, const char *value);
void tui_file_row(struct tui *tui, const char *marker, const char *marker_style,
                  const char *path, const char *right_stats);

/**
 * Polls for non-blocking key or mouse-wheel input.
 */
enum tui_key tui_poll_key(struct tui *tui);

