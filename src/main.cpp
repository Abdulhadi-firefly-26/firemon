#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <csignal>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "tui.h"
#include "cpu_info.h"
#include "memory_info.h"
#include "storage_info.h"
#include "battery_info.h"

using namespace std;
using namespace filesystem;

static volatile sig_atomic_t g_running = 1;

void signal_handler(int /*sig*/) {
    g_running = 0;
}

// Non-blocking key check
bool key_pressed(char& ch) {
    struct termios oldt{}, newt{};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    ssize_t n = read(STDIN_FILENO, &ch, 1);
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return n > 0;
}

void render_dashboard(const filesystem::path& proc, const filesystem::path& root,
                      const cpu_snapshot& prev_cpu, const cpu_snapshot& curr_cpu,
                      const string& cpu_model) {
    tui::TermSize ts = tui::get_terminal_size();
    int panel_width = ts.cols - 2;
    if (panel_width < 40) panel_width = 40;

    // Move to (1,1) and clear screen
    tui::clear_screen();
    tui::draw_header(ts.cols);
    
    render_cpu_panel(prev_cpu, curr_cpu, cpu_model, panel_width);
    cout << "\n";
    render_memory_panel(proc, panel_width);
    cout << "\n";
    render_storage_panel(proc, panel_width);
    cout << "\n";
    render_battery_panel(root, panel_width);

    tui::draw_footer(ts.cols);
    
    cout << flush;
}

int main() 
{
    // Setup signal handler for clean exit
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    filesystem::path p = getenv("HOME");
    path root = p / "../..";
    path proc = root / "proc";

    // Switch to alternate screen buffer (like htop/less/vim) and hide cursor
    tui::enter_alt_screen();
    tui::hide_cursor();

    // Read CPU model once
    string cpu_model = read_cpu_model(proc);

    // Take initial CPU snapshot
    cpu_snapshot prev_snap = read_cpu_snapshot(proc);

    // Wait a tiny bit for first delta
    this_thread::sleep_for(chrono::milliseconds(250));

    while (g_running) {
        cpu_snapshot curr_snap = read_cpu_snapshot(proc);

        render_dashboard(proc, root, prev_snap, curr_snap, cpu_model);

        prev_snap = curr_snap;

        // Wait ~1 second, checking for key press every 50ms
        for (int i = 0; i < 20 && g_running; ++i) {
            char ch = 0;
            if (key_pressed(ch)) {
                if (ch == 'q' || ch == 'Q') {
                    g_running = 0;
                    break;
                }
            }
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    }

    // Restore terminal state cleanly
    tui::show_cursor();
    tui::exit_alt_screen();
    cout << tui::color::accent() << "Firemon" << tui::color::RESET << " — Goodbye! 👋\n" << flush;

    return 0;
}