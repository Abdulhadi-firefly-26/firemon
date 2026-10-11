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

class storage_data 
{
private:
    string device;
    string mount_point;
    string fs_type;
    double total_gb;
    double free_gb;
    double used_gb; 
public:
    storage_data(string dev, string mount, string type) : device(dev), mount_point(mount), fs_type(type)
    {
        total_gb = 0;
        free_gb = 0;
        used_gb = 0;
    }
    void calculate_space()
    {
        space_info si = space(mount_point);
        total_gb = static_cast<double>(si.capacity) / (1024.0 * 1024.0 * 1024.0);
        free_gb = static_cast<double>(si.free) / (1024.0 * 1024.0 * 1024.0);
        used_gb = total_gb - free_gb;
    }
    bool is_valid_disk() const
    {
        if (device.rfind("/dev/", 0) != 0) return false;
        if (mount_point.rfind("/boot", 0) == 0) return false;
        // Filter out loop devices (snaps, etc.)
        if (device.rfind("/dev/loop", 0) == 0) return false;
        // Filter out snap mounts
        if (mount_point.find("/snap/") != string::npos) return false;
        // Filter squashfs (snap filesystem)
        if (fs_type == "squashfs") return false;
        return true;
    }
    string get_device() const { return device; }
    string get_mount() const { return mount_point; }
    string get_fs_type() const { return fs_type; }
    double get_total() const { return total_gb; }
    double get_free() const { return free_gb; }
    double get_used() const { return used_gb; }
    double get_fraction() const { return (total_gb > 0) ? (used_gb / total_gb) : 0.0; }
};

inline vector<storage_data> read_storage_data(const path& mounts_path)
{
    vector<storage_data> storage_list;
    ifstream fin(mounts_path);
    if (!fin.is_open()) return storage_list;

    string dev, mount, type, dummy;
    while (fin >> dev >> mount >> type)
    {
        getline(fin, dummy);
        storage_data data(dev, mount, type);
        if (data.is_valid_disk())
        {
            data.calculate_space();
            storage_list.push_back(data);
        }
    }
    return storage_list;
}

inline void render_storage_panel(const path& proc_path, int panel_width)
{
    using namespace tui;
    using namespace tui::color;

    draw_panel_top(panel_width, "STORAGE");

    if (!exists(proc_path / "mounts"))
    {
        draw_panel_line(panel_width, danger() + " ✗ mounts file not found" + RESET + string(static_cast<size_t>(panel_width - 27), ' '));
        draw_panel_bottom(panel_width);
        return;
    }

    vector<storage_data> disks = read_storage_data(proc_path / "mounts");

    if (disks.empty())
    {
        draw_panel_line(panel_width, warning() + " No physical disks detected" + RESET + string(static_cast<size_t>(panel_width - 31), ' '));
        draw_panel_bottom(panel_width);
        return;
    }

    int content_w = panel_width - 4;
    int label_w = 14;
    int value_w = 10;
    int bar_w = content_w - label_w - value_w - 9;
    if (bar_w < 10) bar_w = 10;

    for (size_t i = 0; i < disks.size(); ++i)
    {
        const auto& d = disks[i];

        // Device name as a sub-header
        string dev_label = d.get_device();
        string mount_label = d.get_mount();
        string fs_label = d.get_fs_type();

        string header_line = BOLD + info() + " " + dev_label + RESET + 
                             muted() + "  →  " + RESET + 
                             FG_WHITE + mount_label + RESET + 
                             muted() + "  (" + fs_label + ")" + RESET;
        
        // Rough visible width for padding
        int vis_w = 1 + static_cast<int>(dev_label.size()) + 5 + static_cast<int>(mount_label.size()) + 3 + static_cast<int>(fs_label.size()) + 1;
        int pad_needed = content_w - vis_w;
        if (pad_needed < 0) pad_needed = 0;
        
        draw_panel_line(panel_width, header_line + string(static_cast<size_t>(pad_needed), ' '));

        // Usage bar
        stringstream val_ss;
        val_ss << fixed << setprecision(1) << d.get_used() << "/" << d.get_total() << " GiB";

        string row = kv_row_bar("Used", val_ss.str(), d.get_fraction(), label_w, value_w, bar_w);
        draw_panel_line(panel_width, row);

        // Free space
        stringstream free_ss;
        free_ss << fixed << setprecision(1) << d.get_free() << " GiB free";
        string free_row = muted() + "  " + pad_right(free_ss.str(), content_w - 2) + RESET;
        draw_panel_line(panel_width, free_row);

        if (i < disks.size() - 1) {
            draw_panel_separator(panel_width);
        }
    }

    draw_panel_bottom(panel_width);
}
