#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include "tui.h"

using namespace std;
using namespace std::filesystem;

class mem_info 
{
private:
    string name;
    double value_kb;
    double value_gb;
public:
    mem_info() : name(""), value_kb(0.0), value_gb(0.0) {}
    bool parse_line(const string& line) 
    {
        size_t colon_pos = line.find(':');
        if (colon_pos == string::npos) 
        {
            return false;
        }
        name = line.substr(0, colon_pos);
        string remaining = line.substr(colon_pos + 1);
        stringstream ss(remaining);        
        if (ss >> value_kb) 
        {
            value_gb = value_kb / (1024.0 * 1024.0);
            return true;
        }
        return false;
    }
    string get_name() const { return name; }
    double get_kb() const { return value_kb; }
    double get_gb() const { return value_gb; }
};

struct mem_summary {
    double total_gb = 0;
    double free_gb = 0;
    double available_gb = 0;
    double buffers_gb = 0;
    double cached_gb = 0;
    double swap_total_gb = 0;
    double swap_free_gb = 0;
    bool has_data = false;
};

inline mem_summary read_memory_data(const path& mem_path) 
{
    mem_summary summary;
    ifstream fin(mem_path);
    if (!fin.is_open()) return summary;

    string line;
    while (getline(fin, line)) 
    {
        mem_info data;
        if (data.parse_line(line)) 
        {
            if (data.get_name() == "MemTotal")      summary.total_gb = data.get_gb();
            else if (data.get_name() == "MemFree")   summary.free_gb = data.get_gb();
            else if (data.get_name() == "MemAvailable") summary.available_gb = data.get_gb();
            else if (data.get_name() == "Buffers")   summary.buffers_gb = data.get_gb();
            else if (data.get_name() == "Cached")    summary.cached_gb = data.get_gb();
            else if (data.get_name() == "SwapTotal") summary.swap_total_gb = data.get_gb();
            else if (data.get_name() == "SwapFree")  summary.swap_free_gb = data.get_gb();
            summary.has_data = true;
        }
    }
    return summary;
}

inline void render_memory_panel(const path& proc_path, int panel_width)
{
    using namespace tui;
    using namespace tui::color;

    tui::draw_panel_top(panel_width, "MEMORY");
    
    if (!exists(proc_path / "meminfo"))
    {
        draw_panel_line(panel_width, danger() + " ✗ meminfo not found" + RESET + string(static_cast<size_t>(panel_width - 24), ' '));
        draw_panel_bottom(panel_width);
        return;
    }

    mem_summary mem = read_memory_data(proc_path / "meminfo");
    if (!mem.has_data)
    {
        draw_panel_line(panel_width, danger() + " ✗ Could not read memory data" + RESET + string(static_cast<size_t>(panel_width - 33), ' '));
        draw_panel_bottom(panel_width);
        return;
    }

    int content_w = panel_width - 4;
    int label_w = 14;
    int value_w = 10;
    int bar_w = content_w - label_w - value_w - 9; // 9 = spacing + pct
    if (bar_w < 10) bar_w = 10;

    // RAM usage
    double used_gb = mem.total_gb - mem.available_gb;
    double ram_frac = (mem.total_gb > 0) ? (used_gb / mem.total_gb) : 0.0;

    stringstream val_ss;
    val_ss << fixed << setprecision(2) << used_gb << "/" << setprecision(1) << mem.total_gb << " GiB";

    string row = kv_row_bar("RAM Usage", val_ss.str(), ram_frac, label_w, value_w, bar_w);
    // Pad the row to fill panel width (approximate)
    draw_panel_line(panel_width, row);
    
    // Swap usage
    double swap_used = mem.swap_total_gb - mem.swap_free_gb;
    double swap_frac = (mem.swap_total_gb > 0) ? (swap_used / mem.swap_total_gb) : 0.0;

    stringstream swap_ss;
    if (mem.swap_total_gb > 0) {
        swap_ss << fixed << setprecision(2) << swap_used << "/" << setprecision(1) << mem.swap_total_gb << " GiB";
    } else {
        swap_ss << "N/A";
    }

    row = kv_row_bar("Swap Usage", swap_ss.str(), swap_frac, label_w, value_w, bar_w);
    draw_panel_line(panel_width, row);
    
    draw_panel_separator(panel_width);

    // Detail rows
    stringstream buf_ss, cache_ss, free_ss;
    buf_ss << fixed << setprecision(3) << mem.buffers_gb << " GiB";
    cache_ss << fixed << setprecision(3) << mem.cached_gb << " GiB";
    free_ss << fixed << setprecision(3) << mem.free_gb << " GiB";

    string detail_pad(static_cast<size_t>(content_w - label_w - value_w), ' ');

    draw_panel_line(panel_width, kv_row("Buffers", buf_ss.str(), label_w, value_w) + detail_pad);
    draw_panel_line(panel_width, kv_row("Cached", cache_ss.str(), label_w, value_w) + detail_pad);
    draw_panel_line(panel_width, kv_row("Free (raw)", free_ss.str(), label_w, value_w) + detail_pad);

    draw_panel_bottom(panel_width);
}
