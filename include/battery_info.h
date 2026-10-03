#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <string>
#include <cmath>

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
    bool exists;
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
    void display_measurement(const string& label, double value, int precision, const string& unit, bool available) const
    {
        cout << "*** " << setw(15) << left << label << setw(9) << right;
        if (available)
        {
            cout << fixed << setprecision(precision) << value;
            if (unit == "%")
            {
                cout << unit;
            }
            else
            {
                cout << " " << unit;
            }
        }
        else
        {
            cout << "N/A";
        }
        cout << "      ***\n";
    }

public:
    battery_data():status("Unknown"), capacity(0), power_w(0.0), voltage_v(0.0), current_a(0.0), temp_c(0.0), exists(false), status_available(false), capacity_available(false), power_available(false), voltage_available(false), current_available(false), temp_available(false){}
    void read_data(const path& bat_path)
    {
        string type;
        ifstream f_type(bat_path / "type");
        if (!(f_type >> type) || type != "Battery")
        {
            exists = false;
            return;
        }
        exists = true;
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
    bool has_battery() 
    {
        return exists;
    }
    void display()
    {
        cout << "\n================= BATTERY INFO ================\n";
        cout << "*** " << setw(15) << left << "Status:" << setw(10) << right << (status_available ? status : "N/A") << "       ***\n";
        display_measurement("Capacity:", capacity, 0, "%", capacity_available);
        display_measurement("Power:", power_w, 1, "W", power_available);
        display_measurement("Voltage:", voltage_v, 1, "V", voltage_available);
        display_measurement("Current:", current_a, 2, "A", current_available);
        display_measurement("Temperature:", temp_c, 0, "C", temp_available);
    }
};
void battery_manager(const path& root_path)
{
    path sys_power = root_path / "sys/class/power_supply";
    battery_data bat;
    if (is_directory(sys_power))
    {
        for (const auto& entry : directory_iterator(sys_power))
        {
            if (entry.is_directory())
            {
                bat.read_data(entry.path());
                if (bat.has_battery())
                {
                    bat.display();
                    return;
                }
            }
        }
    }
    cout << "\n================= BATTERY INFO ================\n";
    cout << "Error: No internal battery device detected\n";
}
