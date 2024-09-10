
#ifndef _KERNEL_H_
#define _KERNEL_H_
#include <cstdint>
#include <unistd.h>
#include <string>

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
    return hash & 0xFFFF0000 | cmd;
}

// ge 0a05
static inline long ver_and_cmd(const char *key, long cmd)
{
    uint32_t version_code = (MAJOR << 16) + (MINOR << 8) + PATCH;
    return ((long)version_code << 32) | (0x1158 << 16) | (cmd & 0xFFFF);
}

static inline long compact_cmd(const char *key, long cmd)
{
    long ver = syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KERNELPATCH_VER));
    if (ver >= 0xa05) return ver_and_cmd(key, cmd);
    return hash_key_cmd(key, cmd);
}

struct kpm_read{
    int pid;
    uint64_t addr;
    int size;
    void *buffer;
};

class kernel
{
private:
    uint64_t cmd_read;
    uint64_t cmd_write;
    struct kpm_read kread;
    
public:
    //自定义的传参命令  super_key     self_cmd = 555   cmd_read = 0x555
    // ret = sc_kpm_control(key.c_str(),"kpm_kread","555",0,0);
    // kernel k(0x555);
    //剩下的自己优化吧
    int cmd_ctl(std::string key, std::string  cmd){
        if(key.empty()) return -1;
        if(cmd.empty()) return -1;
        long ret = syscall(__NR_supercall, key.c_str() , compact_cmd(key.c_str(), SUPERCALL_KPM_CONTROL), "kpm_kread", cmd.c_str(), 0, 0);

        return ret;
    }



    //自定义的传参命令
    kernel(uint64_t cmd){
        cmd_read = cmd;//十六进制 
        cmd_write = cmd + 1;
    };

    void set_pid(int pid){
        kread.pid = pid;
    }

    template<typename T>
    T read(uint64_t addr){
        T data;
        kread.addr = addr;
        kread.size = sizeof(T);
        kread.buffer = &data;
        int ret = ioctl(-1,cmd_read,&kread)
        return data;
    }

    template<typename T>
    bool write(uint64_t addr, T data){
        kread.addr = addr;
        kread.size = sizeof(T);
        kread.buffer = &data;
        int ret = ioctl(-1,cmd_write,&kread)
        return ret==0;
    }


    ~kernel();
};



#endif /* _KERNEL_H_ */
