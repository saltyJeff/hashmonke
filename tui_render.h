#pragma once

#include "file.h"
#include "runner.h"
#include "tui.h"
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>

struct tui_render_state;

/**
 * Creates and initializes the TUI render state from a parsed manifest.
 */
struct tui_render_state *tui_render_state_create(const struct hashmonke_file *manifest,
                                                 const char *manifest_title);

/**
 * Frees the TUI render state.
 */
void tui_render_state_free(struct tui_render_state *state);

/**
 * Runner event callback that updates row status in the TUI render state.
 */
void tui_render_event_callback(const struct hashmonke_runner_event *event,
                               struct hashmonke_runner_event_context *context);

/**
 * Renders a complete frame to the terminal using immediate-mode TUI calls.
 */
void tui_render_frame(struct tui *tui, struct tui_render_state *state,
                      const struct hashmonke_runner_stats *stats);

/**
 * Handles a key event (scrolling / pagination). Returns true if the key was handled.
 */
bool tui_render_handle_key(struct tui *tui, struct tui_render_state *state,
                           enum tui_key key);

/**
 * Runs the interactive TUI verification loop until completion or interruption,
 * shutting down the TUI and printing the final summary.
 *
 * @return exit code (0 for success, 1 for mismatch/missing, 2 for error, 130 for interrupt).
 */
int tui_render_run(struct hashmonke_file *manifest, const char *manifest_title,
                   uint32_t starting_workers, bool thread_warmup,
                   bool wait_for_input, volatile sig_atomic_t *interrupted);

