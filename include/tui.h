#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cmath>

using namespace std;

namespace tui {

// ── Terminal Size ───────────────────────────────────────────
struct TermSize {
    int cols;
    int rows;
};

inline TermSize get_terminal_size() {
    struct winsize w{};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return {w.ws_col > 0 ? w.ws_col : 80, w.ws_row > 0 ? w.ws_row : 24};
}

// ── ANSI Color Codes ───────────────────────────────────────
namespace color {
    // Reset
    const string RESET   = "\033[0m";
    const string BOLD    = "\033[1m";
    const string DIM     = "\033[2m";
    const string ITALIC  = "\033[3m";

    // Foreground colors (vibrant palette)
    const string FG_BLACK   = "\033[30m";
    const string FG_RED     = "\033[91m";   // Bright red
    const string FG_GREEN   = "\033[92m";   // Bright green
    const string FG_YELLOW  = "\033[93m";   // Bright yellow
    const string FG_BLUE    = "\033[94m";   // Bright blue
    const string FG_MAGENTA = "\033[95m";   // Bright magenta
    const string FG_CYAN    = "\033[96m";   // Bright cyan
    const string FG_WHITE   = "\033[97m";   // Bright white
    const string FG_GRAY    = "\033[90m";   // Gray

    // 256-color support
    inline string fg256(int n) { return "\033[38;5;" + to_string(n) + "m"; }
    inline string bg256(int n) { return "\033[48;5;" + to_string(n) + "m"; }

    // RGB support (truecolor)
    inline string fg_rgb(int r, int g, int b) {
        return "\033[38;2;" + to_string(r) + ";" + to_string(g) + ";" + to_string(b) + "m";
    }
    inline string bg_rgb(int r, int g, int b) {
        return "\033[48;2;" + to_string(r) + ";" + to_string(g) + ";" + to_string(b) + "m";
    }

    // Theme colors (a warm fire/ember palette)
    inline string accent()       { return fg_rgb(255, 120, 50); }   // Ember orange
    inline string accent2()      { return fg_rgb(255, 80, 80); }    // Fire red
    inline string highlight()    { return fg_rgb(255, 200, 80); }   // Warm yellow
    inline string info()         { return fg_rgb(100, 200, 255); }  // Cool blue
    inline string success()      { return fg_rgb(80, 250, 160); }   // Mint green
    inline string warning()      { return fg_rgb(255, 200, 60); }   // Amber
    inline string danger()       { return fg_rgb(255, 80, 80); }    // Red
    inline string muted()        { return fg_rgb(120, 120, 140); }  // Muted gray
    inline string panel_border() { return fg_rgb(80, 80, 100); }    // Subtle border
    inline string panel_title()  { return fg_rgb(255, 140, 60); }   // Title orange
    inline string bar_bg()       { return fg_rgb(50, 50, 65); }     // Dark bar background
}

// ── Box Drawing Characters (Unicode) ───────────────────────
namespace box {
    const string TL = "╭";  // Top-left rounded
    const string TR = "╮";  // Top-right rounded
    const string BL = "╰";  // Bottom-left rounded
    const string BR = "╯";  // Bottom-right rounded
    const string H  = "─";  // Horizontal
    const string V  = "│";  // Vertical
    const string LT = "├";  // Left tee
    const string RT = "┤";  // Right tee
    const string TT = "┬";  // Top tee
    const string BT = "┴";  // Bottom tee
    const string CR = "┼";  // Cross
}

// ── Screen Control ─────────────────────────────────────────
inline void enter_alt_screen() {
    // Switch to alternate screen buffer, clear screen & scrollback buffer, move cursor home
    cout << "\033[?1049h\033[2J\033[3J\033[H" << flush;
}

inline void exit_alt_screen() {
    // Switch back to normal screen buffer
    cout << "\033[?1049l" << flush;
}

inline void clear_screen() {
    // \033[H moves to (1,1), \033[2J clears viewport, \033[3J clears scrollback
    cout << "\033[H\033[2J\033[3J";
}

inline void move_home() {
    cout << "\033[H";
}

inline void hide_cursor() {
    cout << "\033[?25l" << flush;
}

inline void show_cursor() {
    cout << "\033[?25h" << flush;
}

inline void move_to(int row, int col) {
    cout << "\033[" << row << ";" << col << "H";
}

// ── String Helpers ─────────────────────────────────────────
inline string repeat_str(const string& s, int n) {
    string result;
    result.reserve(s.size() * static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) result += s;
    return result;
}

inline string pad_right(const string& s, int width) {
    if (static_cast<int>(s.size()) >= width) return s.substr(0, static_cast<size_t>(width));
    return s + string(static_cast<size_t>(width - static_cast<int>(s.size())), ' ');
}

inline string pad_left(const string& s, int width) {
    if (static_cast<int>(s.size()) >= width) return s.substr(0, static_cast<size_t>(width));
    return string(static_cast<size_t>(width - static_cast<int>(s.size())), ' ') + s;
}

inline string center(const string& s, int width) {
    if (static_cast<int>(s.size()) >= width) return s.substr(0, static_cast<size_t>(width));
    int pad = width - static_cast<int>(s.size());
    int left_pad = pad / 2;
    int right_pad = pad - left_pad;
    return string(static_cast<size_t>(left_pad), ' ') + s + string(static_cast<size_t>(right_pad), ' ');
}

// ── Progress Bar Rendering ─────────────────────────────────
// Renders a beautiful gradient progress bar
inline string progress_bar(double fraction, int width, const string& fill_color_start, const string& fill_color_end) {
    (void)fill_color_end; // Reserved for future gradient support
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;

    int filled = static_cast<int>(round(fraction * width));
    int empty = width - filled;

    // Block characters for smooth bar
    const string FULL_BLOCK  = "█";
    const string LIGHT_SHADE = "░";

    stringstream ss;

    // Parse start/end colors for gradient (simple linear interpolation)
    // We'll use a simpler approach: just use the start color for the filled part
    ss << fill_color_start;
    for (int i = 0; i < filled; ++i) {
        ss << FULL_BLOCK;
    }
    ss << color::bar_bg();
    for (int i = 0; i < empty; ++i) {
        ss << LIGHT_SHADE;
    }
    ss << color::RESET;

    return ss.str();
}

// Convenience: auto-color bar based on percentage (green → yellow → red)
inline string auto_bar(double fraction, int width) {
    string col;
    if (fraction < 0.5) {
        col = color::success();
    } else if (fraction < 0.75) {
        col = color::warning();
    } else {
        col = color::danger();
    }
    return progress_bar(fraction, width, col, col);
}

// Inverse auto-color bar (for battery: red when low, green when high)
inline string inverse_bar(double fraction, int width) {
    string col;
    if (fraction > 0.5) {
        col = color::success();
    } else if (fraction > 0.25) {
        col = color::warning();
    } else {
        col = color::danger();
    }
    return progress_bar(fraction, width, col, col);
}

// ── Panel Drawing ──────────────────────────────────────────
// Draws a bordered panel with a title
inline void draw_panel_top(int width, const string& title) {
    string border_col = color::panel_border();
    string title_col = color::BOLD + color::panel_title();

    cout << border_col << box::TL;

    if (!title.empty()) {
        string decorated = " 🔥 " + title + " ";
        int line_len = width - 2 - static_cast<int>(decorated.size()) + 3; // +3 for emoji width compensation
        if (line_len < 0) line_len = 0;
        int left_len = 2;
        int right_len = line_len - left_len;
        if (right_len < 0) right_len = 0;

        cout << repeat_str(box::H, left_len);
        cout << title_col << decorated << color::RESET;
        cout << border_col << repeat_str(box::H, right_len);
    } else {
        cout << repeat_str(box::H, width - 2);
    }

    cout << box::TR << color::RESET << "\n";
}

inline void draw_panel_separator(int width) {
    string border_col = color::panel_border();
    cout << border_col << box::LT << repeat_str(box::H, width - 2) << box::RT << color::RESET << "\n";
}

inline void draw_panel_bottom(int width) {
    string border_col = color::panel_border();
    cout << border_col << box::BL << repeat_str(box::H, width - 2) << box::BR << color::RESET << "\n";
}

// Print a line inside a panel (auto-padded to panel width)
// content_width = panel_width - 2 (for the two vertical borders)
inline void draw_panel_line(int /*panel_width*/, const string& content) {
    string border_col = color::panel_border();
    // We can't easily measure ANSI string width, so we trust the caller
    // to provide content that fits within (panel_width - 4) visible chars
    cout << border_col << box::V << color::RESET << " " << content;

    // We need to pad to fill the line. Since content has ANSI codes,
    // we can't use string length. The caller should handle padding.
    cout << " " << border_col << box::V << color::RESET << "\n";
}

// Draw an empty line inside a panel
inline void draw_panel_empty(int width) {
    string border_col = color::panel_border();
    cout << border_col << box::V << string(static_cast<size_t>(width - 2), ' ') << box::V << color::RESET << "\n";
}

// ── Header / Banner ────────────────────────────────────────
inline void draw_header(int width) {
    string fire = color::accent();
    string fire2 = color::accent2();
    string highlight_col = color::highlight();
    string dim = color::DIM;
    string bold = color::BOLD;
    string reset = color::RESET;

    // Compact ASCII art logo
    string logo_line1 = fire2 + "  ╔═╗" + fire + "╦" + fire2 + "╦═╗╔═╗" + fire + "╔╦╗" + fire2 + "╔═╗╔╗╔" + reset;
    string logo_line2 = fire + "  ╠╣ " + fire2 + "║" + fire + "╠╦╝║╣ " + fire2 + "║║║" + fire + "║ ║║║║" + reset;
    string logo_line3 = fire2 + "  ╚  " + fire + "╩" + fire2 + "╩╚═╚═╝" + fire + "╩ ╩" + fire2 + "╚═╝╝╚╝" + reset;

    string subtitle = dim + "───── System Monitor ─────" + reset;

    // Calculate rough center (the logo is ~23 visible chars wide)
    int logo_vis_width = 23;
    int left_pad = (width - logo_vis_width) / 2;
    if (left_pad < 0) left_pad = 0;
    string pad_str(static_cast<size_t>(left_pad), ' ');

    int sub_vis_width = 26;
    int sub_pad = (width - sub_vis_width) / 2;
    if (sub_pad < 0) sub_pad = 0;
    string sub_pad_str(static_cast<size_t>(sub_pad), ' ');

    cout << pad_str << logo_line1 << "\n";
    cout << pad_str << logo_line2 << "\n";
    cout << pad_str << logo_line3 << "\n";
    cout << sub_pad_str << subtitle << "\n";
    cout << "\n";
}

// ── Footer ─────────────────────────────────────────────────
inline void draw_footer(int width) {
    string dim = color::DIM + color::muted();
    string accent_col = color::accent();
    string reset = color::RESET;

    string footer = dim + " Press " + accent_col + "q" + dim + " to quit │ Refreshes every " + accent_col + "1s" + reset;
    cout << "\n" << footer << "\n";
    (void)width; // width available for future use
}

// ── Key-Value Row Helper ───────────────────────────────────
// Renders: "  Label          Value     [bar]"
inline string kv_row(const string& label, const string& value, int label_width, int value_width) {
    string label_col = color::muted();
    string val_col = color::BOLD + color::FG_WHITE;
    string reset = color::RESET;

    stringstream ss;
    ss << label_col << pad_right(label, label_width) << reset
       << val_col << pad_left(value, value_width) << reset;
    return ss.str();
}

// Same but with a progress bar appended
inline string kv_row_bar(const string& label, const string& value, double fraction,
                         int label_width, int value_width, int bar_width, bool inverse = false) {
    string label_col = color::muted();
    string val_col = color::BOLD + color::FG_WHITE;
    string reset = color::RESET;

    stringstream ss;
    ss << label_col << pad_right(label, label_width) << reset
       << val_col << pad_left(value, value_width) << reset
       << "  ";
    if (inverse) {
        ss << inverse_bar(fraction, bar_width);
    } else {
        ss << auto_bar(fraction, bar_width);
    }

    // Percentage label
    int pct = static_cast<int>(round(fraction * 100));
    string pct_col;
    if (!inverse) {
        if (fraction < 0.5) pct_col = color::success();
        else if (fraction < 0.75) pct_col = color::warning();
        else pct_col = color::danger();
    } else {
        if (fraction > 0.5) pct_col = color::success();
        else if (fraction > 0.25) pct_col = color::warning();
        else pct_col = color::danger();
    }
    ss << " " << pct_col << pad_left(to_string(pct) + "%", 4) << reset;

    return ss.str();
}

} // namespace tui
