#include <iostream>
#include "mem.hpp"

long getModuleBase(const char* module_name, int Pid) {
    std::string moduleName(module_name);
    bool bssOF = false;
    bool lastIsSo = false;
    long startAddr = 0;
    char path[256];
    char line[1024];

    size_t pos = moduleName.find(":bss");
    if (pos != std::string::npos) {
        bssOF = true;
        moduleName = moduleName.substr(0, pos);
    }

    snprintf(path, sizeof(path), "/proc/%d/maps", Pid);

    FILE* file = fopen(path, "r");
    if (!file) {
        return 0;
    }

    while (fgets(line, sizeof(line), file)) {
        if (lastIsSo) {
            if (strstr(line, "[anon:.bss]") != nullptr) {
                sscanf(line, "%lx-%*lx", &startAddr);
                break;
            } else {
                lastIsSo = false;
            }
        }

        if (strstr(line, moduleName.c_str()) != nullptr) {
            if (!bssOF) {
                sscanf(line, "%lx-%*lx", &startAddr);
                break;
            } else {
                lastIsSo = true;
            }
        }
    }

    fclose(file);
    return startAddr;
}

int getPID(const char* PackageName)
{   int pid=-1;
    FILE* fp;
    char cmd[0x100] = "pidof ";
    strcat(cmd, PackageName);
    fp = popen(cmd,"r");
    fscanf(fp,"%d", &pid);
    pclose(fp);

    return pid;
}



int main() {


    Mem mem("your key","kpmreadv");
    uint32_t kver = mem.k_ver();
    printf("kver: %lx\n",kver);
    uint32_t kpver = mem.kp_ver();
    printf("kpver: %lx\n",kpver);
    std::cout << "supercall ready: " << mem.ready() << std::endl;

    int pid = getPID("bin.mt.plus.canary");
    std::cout << "pid: " << pid << std::endl;
    mem.ini(pid);
    uintptr_t base = getModuleBase("libmt1.so",pid);
    std::stringstream ss;
    ss << std::hex << base;
    std::cout << "mt1.so base: " << ss.str() << std::endl;
    std::cout<<"开始读取"<<std::endl;
    int a = mem.parse<int>(base,4);
    std::cout << "int: " << a << std::endl;
    uintptr_t b = mem.parse<uintptr_t>(base,8);
    std::cout << "ptr: " << b << std::endl;
    float c = mem.parse<float>(base,4);
    std::cout << "float: " << c << std::endl;
    double d = mem.parse<double>(base,8);
    std::cout << "double: " << d << std::endl;


    std::cout<<"读取结束"<<std::endl;

    return 0;
}
