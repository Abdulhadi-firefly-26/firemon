#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <vector>
#include <string>

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
        // try 
        // {
        space_info si = space(mount_point);
        total_gb = si.capacity / (1024.0 * 1024.0 * 1024.0);
        free_gb = si.free / (1024.0 * 1024.0 * 1024.0);
        used_gb = total_gb - free_gb;
        // }
        // catch (...) 
        // {
        //     total_gb = 0;
        //     free_gb = 0;
        //     used_gb = 0;
        // }
    }
    bool is_valid_disk() const
    {
        if (device.rfind("/dev/", 0) != 0) return false;
        if (mount_point.rfind("/boot", 0) == 0) return false;
        return true;
    }
    void display() const
    {
        cout << "*** " << left << device << "  " << setw(12) << left << mount_point << "  "
             << setw(8) << right << fixed << setprecision(1) << total_gb << " GiB  | "
             << setw(6) << right << used_gb << " GiB Used | " << setw(6) << right
             << free_gb << " GiB Free ***\n";
    }
};
void read_storage_info(const path& mounts_path)
{
    ifstream fin(mounts_path);
    if (!fin.is_open())
    {
        cerr << "Error: Could not open " << mounts_path << "\n";
        return;
    }
    vector<storage_data> storage_list;
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
    cout << "\n================ STORAGE INFO ================\n";
    for (const auto& item : storage_list)
    {
        item.display();
    }
}
void storage_manager(const path& proc_path)
{
    if (exists(proc_path / "mounts"))
    {
        read_storage_info(proc_path / "mounts");
    }
    else
    {
        cout << "Error: mounts file not present\n";
    }
}
