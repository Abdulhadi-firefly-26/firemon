#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <string>
#include <cmath>
#include <sstream>
#include "tui.h"

using namespace std;
using namespace std::filesystem;

class battery_data
{
private:
    string status;
    int capacity;
    double power_w;
    double voltage_v;
    double current_a;
    double temp_c;
    bool bat_exists;
    bool status_available;
    bool capacity_available;
    bool power_available;
    bool voltage_available;
    bool current_available;
    bool temp_available;
    static bool read_attribute(const path& bat_path, const string& name, long long& value)
    {
        ifstream input(bat_path / name);
        return static_cast<bool>(input >> value);
    }

public:
    battery_data():status("Unknown"), capacity(0), power_w(0.0), voltage_v(0.0), current_a(0.0), temp_c(0.0), bat_exists(false), status_available(false), capacity_available(false), power_available(false), voltage_available(false), current_available(false), temp_available(false){}
    void read_data(const path& bat_path)
    {
        string type;
        ifstream f_type(bat_path / "type");
        if (!(f_type >> type) || type != "Battery")
        {
            bat_exists = false;
            return;
        }
        bat_exists = true;
        ifstream f_status(bat_path / "status");
        status_available = static_cast<bool>(f_status >> status);
        ifstream f_capacity(bat_path / "capacity");
        capacity_available = static_cast<bool>(f_capacity >> capacity);
        long long raw_value = 0;
        if (read_attribute(bat_path, "voltage_now", raw_value))
        {
            voltage_v = static_cast<double>(raw_value) / 1000000.0;
            voltage_available = true;
        }
        if (read_attribute(bat_path, "current_now", raw_value))
        {
            current_a = std::abs(static_cast<double>(raw_value)) / 1000000.0;
            current_available = true;
        }
        if (read_attribute(bat_path, "power_now", raw_value))
        {
            power_w = std::abs(static_cast<double>(raw_value)) / 1000000.0;
            power_available = true;
        }
        if ((!power_available || power_w == 0.0) && voltage_available && current_available)
        {
            power_w = voltage_v * current_a;
            power_available = true;
        }
        if (read_attribute(bat_path, "temp", raw_value))
        {
            temp_c = static_cast<double>(raw_value) / 10.0;
            temp_available = true;
        }
    }
    bool has_battery() const
    {
        return bat_exists;
    }
    string get_status() const { return status_available ? status : "N/A"; }
    int get_capacity() const { return capacity_available ? capacity : -1; }
    bool has_capacity() const { return capacity_available; }
    double get_power() const { return power_w; }
    bool has_power() const { return power_available; }
    double get_voltage() const { return voltage_v; }
    bool has_voltage() const { return voltage_available; }
    double get_current() const { return current_a; }
    bool has_current() const { return current_available; }
    double get_temp() const { return temp_c; }
    bool has_temp() const { return temp_available; }
};

inline void render_battery_panel(const path& root_path, int panel_width)
{
    using namespace tui;
    using namespace tui::color;

    draw_panel_top(panel_width, "BATTERY");

    path sys_power = root_path / "sys/class/power_supply";
    battery_data bat;
    bool found = false;

    if (is_directory(sys_power))
    {
        for (const auto& entry : directory_iterator(sys_power))
        {
            if (entry.is_directory())
            {
                bat.read_data(entry.path());
                if (bat.has_battery())
                {
                    found = true;
                    break;
                }
            }
        }
    }

    int content_w = panel_width - 4;
    int label_w = 14;
    int value_w = 10;

    if (!found)
    {
        string msg = muted() + " ⚡ No internal battery detected" + RESET;
        int pad_needed = content_w - 31;
        if (pad_needed < 0) pad_needed = 0;
        draw_panel_line(panel_width, msg + string(static_cast<size_t>(pad_needed), ' '));
        draw_panel_bottom(panel_width);
        return;
    }

    // Status with icon
    string status_str = bat.get_status();
    string status_icon;
    string status_col;
    if (status_str == "Charging") {
        status_icon = "⚡";
        status_col = success();
    } else if (status_str == "Discharging") {
        status_icon = "🔋";
        status_col = warning();
    } else if (status_str == "Full") {
        status_icon = "✓";
        status_col = success();
    } else {
        status_icon = "?";
        status_col = muted();
    }

    string status_line = muted() + pad_right("Status", label_w) + RESET + 
                         BOLD + status_col + pad_left(status_icon + " " + status_str, value_w + 2) + RESET;
    int status_pad = content_w - label_w - value_w - 2;
    if (status_pad < 0) status_pad = 0;
    draw_panel_line(panel_width, status_line + string(static_cast<size_t>(status_pad), ' '));

    // Capacity bar
    if (bat.has_capacity()) {
        int cap = bat.get_capacity();
        double frac = cap / 100.0;
        int bar_w = content_w - label_w - value_w - 9;
        if (bar_w < 10) bar_w = 10;

        stringstream val_ss;
        val_ss << cap << "%";
        string row = kv_row_bar("Charge", val_ss.str(), frac, label_w, value_w, bar_w, true);
        draw_panel_line(panel_width, row);
    }

    draw_panel_separator(panel_width);

    // Detail measurements
    string detail_pad(static_cast<size_t>(content_w - label_w - value_w), ' ');

    if (bat.has_power()) {
        stringstream ss;
        ss << fixed << setprecision(1) << bat.get_power() << " W";
        draw_panel_line(panel_width, kv_row("Power Draw", ss.str(), label_w, value_w) + detail_pad);
    }
    if (bat.has_voltage()) {
        stringstream ss;
        ss << fixed << setprecision(2) << bat.get_voltage() << " V";
        draw_panel_line(panel_width, kv_row("Voltage", ss.str(), label_w, value_w) + detail_pad);
    }
    if (bat.has_current()) {
        stringstream ss;
        ss << fixed << setprecision(2) << bat.get_current() << " A";
        draw_panel_line(panel_width, kv_row("Current", ss.str(), label_w, value_w) + detail_pad);
    }
    if (bat.has_temp()) {
        stringstream ss;
        ss << fixed << setprecision(1) << bat.get_temp() << " °C";
        draw_panel_line(panel_width, kv_row("Temperature", ss.str(), label_w, value_w) + detail_pad);
    }

    draw_panel_bottom(panel_width);
}
