#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include "tui.h"

using namespace std;
using namespace std::filesystem;

struct cpu_times {
    long long user = 0, nice = 0, system = 0, idle = 0;
    long long iowait = 0, irq = 0, softirq = 0, steal = 0;

    long long total() const {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }
    long long active() const {
        return total() - idle - iowait;
    }
};

struct cpu_snapshot {
    cpu_times aggregate;
    vector<cpu_times> cores;
};

inline cpu_snapshot read_cpu_snapshot(const path& proc_path)
{
    cpu_snapshot snap;
    ifstream fin(proc_path / "stat");
    if (!fin.is_open()) return snap;

    string line;
    while (getline(fin, line))
    {
        if (line.rfind("cpu", 0) != 0) continue;

        stringstream ss(line);
        string label;
        ss >> label;

        cpu_times t;
        ss >> t.user >> t.nice >> t.system >> t.idle >> t.iowait >> t.irq >> t.softirq >> t.steal;

        if (label == "cpu") {
            snap.aggregate = t;
        } else {
            snap.cores.push_back(t);
        }
    }
    return snap;
}

inline double calc_usage(const cpu_times& prev, const cpu_times& curr) {
    long long total_delta = curr.total() - prev.total();
    long long active_delta = curr.active() - prev.active();
    if (total_delta <= 0) return 0.0;
    return static_cast<double>(active_delta) / static_cast<double>(total_delta);
}

// Read CPU model name
inline string read_cpu_model(const path& proc_path) {
    ifstream fin(proc_path / "cpuinfo");
    if (!fin.is_open()) return "Unknown CPU";
    string line;
    while (getline(fin, line)) {
        if (line.rfind("model name", 0) == 0) {
            size_t pos = line.find(':');
            if (pos != string::npos) {
                string name = line.substr(pos + 1);
                // Trim leading whitespace
                size_t start = name.find_first_not_of(" \t");
                if (start != string::npos) return name.substr(start);
            }
        }
    }
    return "Unknown CPU";
}

inline void render_cpu_panel(const cpu_snapshot& prev, const cpu_snapshot& curr,
                             const string& cpu_model, int panel_width)
{
    using namespace tui;
    using namespace tui::color;

    draw_panel_top(panel_width, "CPU");

    int content_w = panel_width - 4;

    // CPU model name (truncated if needed)
    string model_disp = cpu_model;
    if (static_cast<int>(model_disp.size()) > content_w - 4) {
        model_disp = model_disp.substr(0, static_cast<size_t>(content_w - 7)) + "...";
    }
    string model_line = info() + BOLD + " " + model_disp + RESET;
    int model_pad = content_w - static_cast<int>(model_disp.size()) - 1;
    if (model_pad < 0) model_pad = 0;
    draw_panel_line(panel_width, model_line + string(static_cast<size_t>(model_pad), ' '));

    // Overall usage
    double overall = calc_usage(prev.aggregate, curr.aggregate);
    int label_w = 14;
    int value_w = 10;
    int bar_w = content_w - label_w - value_w - 9;
    if (bar_w < 10) bar_w = 10;

    stringstream overall_ss;
    overall_ss << fixed << setprecision(1) << (overall * 100) << "%";
    string row = kv_row_bar("Total Usage", overall_ss.str(), overall, label_w, value_w, bar_w);
    draw_panel_line(panel_width, row);

    draw_panel_separator(panel_width);

    // Per-core bars (compact — 2 cores per row if space permits)
    int num_cores = static_cast<int>(min(prev.cores.size(), curr.cores.size()));
    if (num_cores == 0) {
        string no_core = muted() + " No per-core data available" + RESET;
        int np = content_w - 27;
        if (np < 0) np = 0;
        draw_panel_line(panel_width, no_core + string(static_cast<size_t>(np), ' '));
    } else {
        // Two columns if terminal is wide enough
        bool two_col = (content_w >= 60);
        int col_w = two_col ? (content_w / 2 - 1) : content_w;
        int core_bar_w = col_w - 14; // "Core XX  XX% " 
        if (core_bar_w < 6) core_bar_w = 6;

        for (int i = 0; i < num_cores; ) {
            stringstream line_ss;

            // First core
            double usage = calc_usage(prev.cores[static_cast<size_t>(i)], curr.cores[static_cast<size_t>(i)]);
            int pct = static_cast<int>(round(usage * 100));
            string pct_col;
            if (usage < 0.5) pct_col = success();
            else if (usage < 0.75) pct_col = warning();
            else pct_col = danger();

            stringstream core_label;
            core_label << "Core " << setw(2) << right << i;

            line_ss << muted() << core_label.str() << RESET << " "
                    << auto_bar(usage, core_bar_w) << " "
                    << pct_col << pad_left(to_string(pct) + "%", 4) << RESET;

            ++i;

            if (two_col && i < num_cores) {
                // Second core
                double usage2 = calc_usage(prev.cores[static_cast<size_t>(i)], curr.cores[static_cast<size_t>(i)]);
                int pct2 = static_cast<int>(round(usage2 * 100));
                string pct_col2;
                if (usage2 < 0.5) pct_col2 = success();
                else if (usage2 < 0.75) pct_col2 = warning();
                else pct_col2 = danger();

                stringstream core_label2;
                core_label2 << "Core " << setw(2) << right << i;

                line_ss << "  " << muted() << core_label2.str() << RESET << " "
                        << auto_bar(usage2, core_bar_w) << " "
                        << pct_col2 << pad_left(to_string(pct2) + "%", 4) << RESET;
                ++i;
            }

            draw_panel_line(panel_width, line_ss.str());
        }
    }

    draw_panel_bottom(panel_width);
}
