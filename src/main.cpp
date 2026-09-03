#include<iostream>
#include<fstream>
#include<filesystem>
#include<cstdlib>
#include"memory_info.h"
#include"storage_info.h"

using namespace std;
using namespace filesystem;

filesystem::path p=getenv("HOME");
path root = p/"../..";
path proc = root/"proc";


int main() 
{
    mem_manager(proc);
    storage_manager(proc);
    return 0;
}