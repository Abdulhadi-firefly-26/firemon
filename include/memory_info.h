#include<iostream>
#include<fstream>
#include<filesystem>
#include<iomanip>
using namespace std;
using namespace filesystem;

void memory_info(path mem);

void proc_manager(path proc)
{
    for (auto& entry : directory_iterator(proc)) 
    {
        if (entry.is_directory()) continue;
        cout << (entry.is_directory() ? "[DIR] " : "[FILE] ") << entry.path().filename() << "\n";

    }
    if (exists(proc/"meminfo"))
    {
        memory_info(proc/"meminfo");
    }
    else cout<<"mem_info_not_present\n";
}

void memory_info(path mem)
{
    cout<<mem<<endl;
    string data;
    ifstream fin(mem);
    data.resize(file_size(mem));
    fin.read(&data[0],data.size());
    cout<<data<<endl;
}