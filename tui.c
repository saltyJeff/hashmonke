#include "tui.h"

#include <conio.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

struct tui
{
    char *buf;
    size_t buf_cap;
    size_t buf_len;
    int rows;
    int cols;
    int line_chars;
    bool active;
    HANDLE h_stdout;
    HANDLE h_stdin;
    DWORD orig_out_mode;
    DWORD orig_in_mode;
    UINT orig_cp;
    UINT orig_in_cp;
};

static void tui_buf_append(struct tui *tui, const char *str, size_t len)
{
    if (tui->buf_len + len >= tui->buf_cap)
    {
        size_t next = tui->buf_cap ? tui->buf_cap * 2 : 16384;
        while (next <= tui->buf_len + len)
            next *= 2;
        char *grown = (char *)realloc(tui->buf, next);
        if (!grown)
            return;
        tui->buf = grown;
        tui->buf_cap = next;
    }
    memcpy(tui->buf + tui->buf_len, str, len);
    tui->buf_len += len;
}

static void tui_buf_str(struct tui *tui, const char *str)
{
    if (str)
        tui_buf_append(tui, str, strlen(str));
}

void tui_get_size(struct tui *tui, int *rows, int *cols)
{
    int r = 24;
    int c = 80;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(tui->h_stdout, &csbi))
    {
        c = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        r = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }

    if (r < 10)
        r = 10;
    if (c < 40)
        c = 40;

    tui->rows = r;
    tui->cols = c;

    if (rows)
        *rows = r;
    if (cols)
        *cols = c;
}

struct tui *tui_init(void)
{
    if (!_isatty(_fileno(stdout)))
        return NULL;

    struct tui *tui = (struct tui *)calloc(1, sizeof(struct tui));
    if (!tui)
        return NULL;

    tui->buf_cap = 32768;
    tui->buf = (char *)malloc(tui->buf_cap);
    if (!tui->buf)
    {
        free(tui);
        return NULL;
    }

    tui->h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    tui->h_stdin = GetStdHandle(STD_INPUT_HANDLE);

    GetConsoleMode(tui->h_stdout, &tui->orig_out_mode);
    GetConsoleMode(tui->h_stdin, &tui->orig_in_mode);

    tui->orig_cp = GetConsoleOutputCP();
    tui->orig_in_cp = GetConsoleCP();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Enable VT sequences and mouse input
    DWORD out_mode = tui->orig_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    SetConsoleMode(tui->h_stdout, out_mode);

    DWORD in_mode = ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT;
    SetConsoleMode(tui->h_stdin, in_mode);

    tui->active = true;

    // Enter alternate screen, hide cursor, enable mouse reporting
    const char *setup_seq = "\033[?1049h\033[?25l\033[?1000h\033[?1006h\033[2J\033[H";
    (void)_write(_fileno(stdout), setup_seq, (unsigned int)strlen(setup_seq));
    fflush(stdout);

    tui_get_size(tui, &tui->rows, &tui->cols);
    return tui;
}

void tui_shutdown(struct tui *tui)
{
    if (!tui)
        return;

    if (tui->active)
    {
        // Disable mouse tracking, restore cursor, exit alternate screen
        const char *teardown_seq = "\033[?1000l\033[?1006l\033[?25h\033[?1049l";
        (void)_write(_fileno(stdout), teardown_seq, (unsigned int)strlen(teardown_seq));
        fflush(stdout);

        SetConsoleOutputCP(tui->orig_cp);
        SetConsoleCP(tui->orig_in_cp);
        SetConsoleMode(tui->h_stdout, tui->orig_out_mode);
        SetConsoleMode(tui->h_stdin, tui->orig_in_mode);
        tui->active = false;
    }

    free(tui->buf);
    free(tui);
}

void tui_begin_frame(struct tui *tui)
{
    if (!tui)
        return;
    tui->buf_len = 0;
    tui_get_size(tui, &tui->rows, &tui->cols);
    tui_buf_str(tui, "\033[H");
}

void tui_end_frame(struct tui *tui)
{
    if (!tui || !tui->buf)
        return;
    tui_buf_str(tui, "\033[J");
    (void)_write(_fileno(stdout), tui->buf, (unsigned int)tui->buf_len);
    fflush(stdout);
}

void tui_start_line(struct tui *tui)
{
    if (!tui)
        return;
    tui->line_chars = 0;
}

void tui_text(struct tui *tui, const char *str)
{
    if (!tui || !str)
        return;
    tui_buf_str(tui, str);

    // Track character count (excluding UTF-8 continuation bytes and ANSI escapes)
    const unsigned char *p = (const unsigned char *)str;
    while (*p)
    {
        if (*p == '\033')
        {
            while (*p && *p != 'm')
                p++;
            if (*p)
                p++;
            continue;
        }
        if ((*p & 0xC0) != 0x80)
            tui->line_chars++;
        p++;
    }
}

void tui_text_styled(struct tui *tui, const char *str, const char *ansi_style)
{
    if (!tui || !str)
        return;
    if (ansi_style)
        tui_buf_str(tui, ansi_style);
    tui_text(tui, str);
    if (ansi_style)
        tui_buf_str(tui, "\033[0m");
}

void tui_progress_bar(struct tui *tui, uint64_t completed, uint64_t total, int bar_width)
{
    if (!tui || bar_width <= 0)
        return;

    int filled = 0;
    if (total > 0)
    {
        if (completed >= total)
            filled = bar_width;
        else
            filled = (int)((completed * (uint64_t)bar_width) / total);
    }
    if (filled > bar_width)
        filled = bar_width;

    // Filled blocks: █ (\xe2\x96\x88)
    for (int i = 0; i < filled; ++i)
    {
        tui_buf_append(tui, "\xe2\x96\x88", 3);
        tui->line_chars++;
    }
    // Unfilled blocks: ░ (\xe2\x96\x91)
    for (int i = filled; i < bar_width; ++i)
    {
        tui_buf_append(tui, "\xe2\x96\x91", 3);
        tui->line_chars++;
    }
}

void tui_end_line(struct tui *tui)
{
    if (!tui)
        return;
    tui_buf_str(tui, "\033[K\r\n");
    tui->line_chars = 0;
}

void tui_stat_row(struct tui *tui, const char *label, const char *value)
{
    if (!tui)
        return;
    tui_start_line(tui);
    char padded[32];
    snprintf(padded, sizeof(padded), "%-10s", label ? label : "");
    tui_text(tui, padded);
    tui_text(tui, value ? value : "");
    tui_end_line(tui);
}

void tui_file_row(struct tui *tui, const char *marker, const char *marker_style,
                  const char *path, const char *right_stats)
{
    if (!tui)
        return;

    tui_start_line(tui);
    tui_text(tui, "  ");
    if (marker && *marker)
    {
        tui_text_styled(tui, marker, marker_style);
    }
    else
    {
        tui_text(tui, " ");
    }
    tui_text(tui, "  ");

    int max_width = tui->cols > 0 ? tui->cols : 80;
    int stats_len = 0;
    if (right_stats && *right_stats)
    {
        const unsigned char *p = (const unsigned char *)right_stats;
        while (*p)
        {
            if ((*p & 0xC0) != 0x80)
                stats_len++;
            p++;
        }
    }
    int avail_for_path = max_width - 5 - (stats_len > 0 ? stats_len + 3 : 0);
    if (avail_for_path < 10)
        avail_for_path = 10;

    int path_len = path ? (int)strlen(path) : 0;
    if (path_len > avail_for_path && avail_for_path > 3)
    {
        char trunc[256];
        int keep = avail_for_path - 3;
        if (keep > (int)sizeof(trunc) - 4)
            keep = (int)sizeof(trunc) - 4;
        if (keep < 0)
            keep = 0;
        memcpy(trunc, path, keep);
        trunc[keep] = '.';
        trunc[keep + 1] = '.';
        trunc[keep + 2] = '.';
        trunc[keep + 3] = '\0';
        tui_text(tui, trunc);
    }
    else
    {
        tui_text(tui, path ? path : "");
    }

    if (stats_len > 0)
    {
        // Reserve 2 characters margin from right border to prevent console auto-wrapping
        int target_col = max_width - stats_len - 2;
        while (tui->line_chars < target_col)
            tui_text(tui, " ");
        tui_text(tui, right_stats);
    }

    tui_end_line(tui);
}

static enum tui_key parse_ansi_sequence(const unsigned char *buf, size_t len)
{
    if (len < 2)
        return TUI_KEY_QUIT;

    if (buf[1] == '[')
    {
        if (len >= 3)
        {
            if (buf[2] == 'A')
                return TUI_KEY_UP;
            if (buf[2] == 'B')
                return TUI_KEY_DOWN;
            if (buf[2] == 'H')
                return TUI_KEY_HOME;
            if (buf[2] == 'F')
                return TUI_KEY_END;
            if (buf[2] == '5' && len >= 4 && buf[3] == '~')
                return TUI_KEY_PAGE_UP;
            if (buf[2] == '6' && len >= 4 && buf[3] == '~')
                return TUI_KEY_PAGE_DOWN;
            if (buf[2] == '1' && len >= 4 && buf[3] == '~')
                return TUI_KEY_HOME;
            if (buf[2] == '4' && len >= 4 && buf[3] == '~')
                return TUI_KEY_END;

            // SGR mouse sequence: \033[<64;...M or \033[<65;...M
            if (buf[2] == '<')
            {
                int btn = 0;
                size_t i = 3;
                while (i < len && buf[i] >= '0' && buf[i] <= '9')
                {
                    btn = btn * 10 + (buf[i] - '0');
                    i++;
                }
                if (btn == 64)
                    return TUI_KEY_MOUSE_WHEEL_UP;
                if (btn == 65)
                    return TUI_KEY_MOUSE_WHEEL_DOWN;
            }
        }
    }
    else if (buf[1] == 'O')
    {
        if (len >= 3)
        {
            if (buf[2] == 'H')
                return TUI_KEY_HOME;
            if (buf[2] == 'F')
                return TUI_KEY_END;
        }
    }
    return TUI_KEY_OTHER;
}

enum tui_key tui_poll_key(struct tui *tui)
{
    (void)tui;

    if (_kbhit())
    {
        int c = _getch();
        if (c == 0 || c == 224)
        {
            int ext = _getch();
            switch (ext)
            {
            case 72:
                return TUI_KEY_UP;
            case 80:
                return TUI_KEY_DOWN;
            case 73:
                return TUI_KEY_PAGE_UP;
            case 81:
                return TUI_KEY_PAGE_DOWN;
            case 71:
                return TUI_KEY_HOME;
            case 79:
                return TUI_KEY_END;
            default:
                return TUI_KEY_OTHER;
            }
        }
        else if (c == 27)
        {
            // Escape or start of ANSI sequence
            unsigned char seq[32];
            seq[0] = 27;
            size_t n = 1;
            while (_kbhit() && n < sizeof(seq) - 1)
            {
                seq[n++] = (unsigned char)_getch();
            }
            if (n == 1)
                return TUI_KEY_QUIT;
            return parse_ansi_sequence(seq, n);
        }
        else if (c == '\r' || c == '\n')
        {
            return TUI_KEY_ENTER;
        }
        else if (c == 'q' || c == 'Q' || c == 3)
        {
            return TUI_KEY_QUIT;
        }
        return TUI_KEY_OTHER;
    }
    return TUI_KEY_NONE;
}

