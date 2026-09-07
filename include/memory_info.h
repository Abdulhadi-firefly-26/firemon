#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <vector>
#include <string>

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
    void display() const 
    {
        if (name == "MemTotal" || name == "MemFree" || name == "MemAvailable" || name == "Buffers" || name == "Cached" || name == "SwapTotal" || name == "SwapFree") 
        {
            cout << "*** " << setw(20) << left << name << setw(10) << right << fixed << setprecision(3) << value_gb << " GB ***\n";
        }
    }
};

void read_memory_info(const path& mem_path) 
{
    ifstream fin(mem_path);
    if (!fin.is_open()) 
    {
        cerr << "Error: Could not open " << mem_path << "\n";
        return;
    }
    vector<mem_info> memory_list;
    string line;
    while (getline(fin, line)) 
    {
        mem_info data;
        if (data.parse_line(line)) 
        {
            memory_list.push_back(data);
        }
    }
    cout << "\n================ MEMORY INFO ================\n";
    for (const auto& item : memory_list) 
    {
        item.display();
    }
}
void mem_manager(const path& proc_path) 
{
    if (exists(proc_path / "meminfo")) 
    {
        read_memory_info(proc_path / "meminfo");
    } 
    else 
    {
        cout << "Error: meminfo file not present\n";
    }
}
