
#ifndef _KERNEL_H_
#define _KERNEL_H_
#include <cstdint>
#include <unistd.h>
#include <string>
#include <iostream>

#define MAJOR 0
#define MINOR 11
#define PATCH 1

#define SUPERCALL_HELLO_ECHO "hello1158"

// #define __NR_supercall __NR3264_truncate // 45
#define __NR_supercall 45

#define SUPERCALL_HELLO 0x1000
#define SUPERCALL_KLOG 0x1004

#define SUPERCALL_KERNELPATCH_VER 0x1008
#define SUPERCALL_KERNEL_VER 0x1009


#define SUPERCALL_KPM_LOAD 0x1020
#define SUPERCALL_KPM_UNLOAD 0x1021
#define SUPERCALL_KPM_CONTROL 0x1022

#define SUPERCALL_KPM_NUMS 0x1030
#define SUPERCALL_KPM_LIST 0x1031
#define SUPERCALL_KPM_INFO 0x1032


#define SUPERCALL_HELLO_MAGIC 0x11581158

static inline long hash_key(const char *key)
{
    long hash = 1000000007;
    for (int i = 0; key[i]; i++) {
        hash = hash * 31 + key[i];
    }
    return hash;
}
// be 0a04
static inline long hash_key_cmd(const char *key, long cmd)
{
    long hash = hash_key(key);
    return (hash & 0xFFFF0000) | cmd;
}

// ge 0a05
static inline long ver_and_cmd( long cmd)
{
    uint32_t version_code = (MAJOR << 16) + (MINOR << 8) + PATCH;
    return ((long)version_code << 32) | (0x1158 << 16) | (cmd & 0xFFFF);
}

static inline long compact_cmd(const char *key, long cmd)
{
    long ver = syscall(__NR_supercall, key, ver_and_cmd( SUPERCALL_KERNELPATCH_VER));
    if (ver >= 0xa05) return ver_and_cmd( cmd);
    return hash_key_cmd(key, cmd);
}




class kernel
{
private:
    struct kpm_read
    {
        uint64_t key;
        int pid;
        int size;
        uint64_t addr;
        void *buffer;

    };

    struct kpm_mod
    {
        uint64_t key;
        int pid;
        char *name;
        uintptr_t base;

    };
    uint64_t cmd_read;
    uint64_t cmd_write;
    uint64_t cmd_mod;
    struct kpm_read kread;
    struct kpm_mod kmod;

public:
    //自定义的传参命令  readcmd_keyvertify -? "555_111"
    //readcmd = "555"        readcmd = 0x555
    // keyvertify ="111"     kread.key=0x111
    // ret = sc_kpm_control(key.c_str(),"kpm_kread","555_111",0,0);
    // kernel k(0x555,0x111);
    //剩下的自己优化吧
    int cmd_ctl(std::string key, std::string cmd,std::string key_vertify){
        if(cmd.empty() || key_vertify.empty() || key.empty()) return -1;
        std::string key_cmd = cmd + "_" + key_vertify;

        long ret = syscall(__NR_supercall, key.c_str() , compact_cmd(key.c_str(), SUPERCALL_KPM_CONTROL), "kpm_kread", key_cmd.c_str(), 0, 0);

        return ret;
    }

    kernel(){};

    //自定义的传参命令 此key为随意数字
    void init(uint64_t cmd,uint64_t key){
        cmd_read = cmd;//十六进制
        cmd_write = cmd + 1;
        cmd_mod = cmd + 2;
        kread.key = key;//十六进制
        kmod.key = key;
    };

    void set_pid(int pid){
        kread.pid = pid;
        kmod.pid = pid;
    }

    template<typename T>
    T read(uint64_t addr){
        T data;
        kread.addr = addr & 0xffffffffffff;
        kread.size = sizeof(T);
        kread.buffer = &data;
        int ret = ioctl(-1,cmd_read,&kread);
        if(ret<0){
            //注意 这里物理页缺失会反 -1 可以自己去掉提示
            std::cout<<"read error maybe pa false"<<std::endl;
            return 0;
        }
        return data;
    }

    void read(uint64_t addr, void *buffer, int size){
        kread.addr = addr & 0xffffffffffff;
        kread.size = size;
        kread.buffer = buffer;
        int ret = ioctl(-1,cmd_read,&kread);
        if(ret<0){
            std::cout<<"read error"<<std::endl;
        }
    }

    void write(uint64_t addr,  void *buffer, int size){
        kread.addr = addr & 0xffffffffffff;
        kread.size = size;
        kread.buffer = buffer;
        int ret = ioctl(-1,cmd_write,&kread);
        if(ret<0){
            std::cout<<"write error"<<std::endl;
        }
    }

    template<typename T>
    bool write(uint64_t addr, T data){
        kread.addr = addr  & 0xffffffffffff;
        kread.size = sizeof(T);
        kread.buffer = &data;
        int ret = ioctl(-1,cmd_write,&kread);
        if(ret<0){
            std::cout<<"write error"<<std::endl;
            return false;
        }
        return ret==0;
    }

    uint64_t get_mod_base(std::string name){
        kmod.name = const_cast<char*>(name.c_str());
        int ret = ioctl(-1,cmd_mod,&kmod);
        if(ret<0){
            std::cout<<"get_mod_base error"<<std::endl;
            return 0;
        }
        return kmod.base;
    }


};



#endif /* _KERNEL_H_ */


////示例代码
//int main() {
//kernel k;
//int pid = getPID("bin.mt.plus.canary");
//if(pid<0){
//std::cout<<"获取pid失败"<<std::endl;
//return -1;
//}
//std::cout<<"pid:"<<pid<<std::endl;
////                                       readcmd   keyvertify
//int ret = k.cmd_ctl("apkey","55555","111");
//if(ret<0){
//std::cout<<"错误命令"<<std::endl;
//return -1;
//}
//
//k.init(0x55555,0x111);
//k.set_pid(pid);
//sleep(1);
//std::cout<<"getModuleBase:"<<std::hex<<getModuleBase("libmt1.so",pid)<<std::endl;
//uint64_t mod_base = k.get_mod_base("libmt1.so");
//std::cout<<"mod_base:"<<std::hex<<mod_base<<std::endl;
//int read_data = k.read<int>(mod_base);
//printf("read_data:%d\n",read_data);
//}
