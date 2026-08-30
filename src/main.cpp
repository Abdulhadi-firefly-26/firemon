#include<iostream>
#include<fstream>
#include<filesystem>
#include<cstdlib>
#include"memory_info.h"
using namespace std;
using namespace filesystem;

filesystem::path p=getenv("HOME");
path root = p/"../..";
path proc = root/"proc";
int main()
{
    // for (auto& entry : directory_iterator(proc)) 
    // {
    //     cout << (entry.is_directory() ? "[DIR] " : "[FILE] ") << entry.path().filename() << "\n";
    // }
    // cout<<p<<endl;
    // cout<<root<<endl<<proc<<endl;
    proc_manager(proc);
}