#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <string>

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

public:
    battery_data()
    {
        status = "Unknown";
        capacity = 0;
        power_w = 0.0;
        voltage_v = 0.0;
        current_a = 0.0;
        temp_c = 0.0;
        exists = false;
    }

    void read_data(const path& bat_path)
    {
        ifstream f_status(bat_path / "status");
        ifstream f_capacity(bat_path / "capacity");
        ifstream f_power(bat_path / "power_now");
        ifstream f_voltage(bat_path / "voltage_now");
        ifstream f_current(bat_path / "current_now");
        ifstream f_temp(bat_path / "temp");

        if (f_status.is_open() && f_capacity.is_open())
        {
            exists = true;
            f_status >> status;
            f_capacity >> capacity;

            long long raw_val = 0;
            if (f_power >> raw_val)
            {
                power_w = raw_val / 1000000.0;
            }
            if (f_voltage >> raw_val)
            {
                voltage_v = raw_val / 1000000.0;
            }
            if (f_current >> raw_val)
            {
                current_a = raw_val / 1000000.0;
            }
            if (f_temp >> raw_val)
            {
                temp_c = raw_val / 10.0;
            }
        }
    }

    bool has_battery() const
    {
        return exists;
    }

    void display() const
    {
        cout << "\n================= BATTERY INFO ================\n";
        cout << "*** " << setw(15) << left << "Status:" << setw(10) << right << status << "       ***\n";
        cout << "*** " << setw(15) << left << "Capacity:" << setw(9) << right << capacity << "%      ***\n";
        cout << "*** " << setw(15) << left << "Power:" << setw(9) << right << fixed << setprecision(1) << power_w << " W      ***\n";
        cout << "*** " << setw(15) << left << "Voltage:" << setw(9) << right << fixed << setprecision(1) << voltage_v << " V      ***\n";
        cout << "*** " << setw(15) << left << "Current:" << setw(9) << right << fixed << setprecision(2) << current_a << " A      ***\n";
        cout << "*** " << setw(15) << left << "Temperature:" << setw(7) << right << fixed << setprecision(0) << temp_c << " C      ***\n";
    }
};

void battery_manager(const path& root_path)
{
    path sys_power = root_path / "sys/class/power_supply";
    battery_data bat;

    if (exists(sys_power))
    {
        for (const auto& entry : directory_iterator(sys_power))
        {
            string folder_name = entry.path().filename().string();
            if (folder_name.rfind("BAT", 0) == 0)
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

