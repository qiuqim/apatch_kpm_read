#ifndef __MEM_HPP__
#define __MEM_HPP__

#include <string>
#include <sstream>
#include <vector>
#include "my_kpm.c"


class Mem {
    std::string _key;
    std::string _path;
    std::string _args;
    std::string _name;
    int _pid;
public:
    Mem(std::string key,std::string name): _key(key),_name(name) {}
    //ctl0 name args "pid_addr_size"
    std::string read(std::string args){
        char result[4096];//字节码
        memset(result,0,4096);
        long ret = sc_kpm_control(_key.c_str(),_name.c_str(),args.c_str(),result,sizeof(result));
        if(ret < 0){
            return "";
        }
        //打印字节码
//        std::cout << "字节码: \n";
//        for(int i = 0; i < 4096; i++){
//            printf("%02x ",result[i]);
//        }
        return std::string(result,4096);
    }
    void ini(int pid){
        this->_pid = pid;
    }
    //解析字节码
    template<typename T>
    T parse(uint64_t addr,int size){
        if(!addr)return 0;
        //addr 转成16进制字符串
        std::stringstream ss;
        ss << std::hex << addr;
        std::cout << "addr: " << ss.str() << std::endl;
        std::string args = std::to_string(_pid) + "_" + ss.str() + "_" + std::to_string(size);
        std::string result = read(args);
        if(result.empty()){
            return 0;
        }
        return *(T*)result.data();
       // return 0;
    }

    uintptr_t readptr(uint64_t addr){
        return parse<uintptr_t>(addr,8);
    }
    uintptr_t readptrs(std::vector<uint64_t> addrs){
        uintptr_t result = parse<uintptr_t>(addrs[0],8);
        for(int i = 1; i < addrs.size(); i++){
            result = parse<uintptr_t>(result + addrs[i],8);
        }
        return result;
    }
    int readint(uint64_t addr){
        return parse<int>(addr,4);
    }
    float readfloat(uint64_t addr){
        return parse<float>(addr,4);
    }
    double readdouble(uint64_t addr){
        return parse<double>(addr,8);
    }

    std::string  loadkpm(std::string path,std::string args){
        long ret = sc_kpm_load(_key.c_str(),path.c_str(),args.c_str(),0);
        std::cout << "ret: " << ret << std::endl;
        if(ret < 0){
            return "加载失败/已加载";
        }
        return "success";
    }
    std::string  unloadkpm(std::string name){
        long ret = sc_kpm_unload(_key.c_str(),name.c_str(),0);

        if(ret < 0){
            return "已卸载";
        }
        return "success";
    }
    //kernel version
    uint32_t k_ver(){
        uint32_t kver = sc_k_ver(_key.c_str());
       // printf("kver: %lx\n",kver);
        return kver;
    }
    //kernel patch version
    uint32_t  kp_ver(){
        uint32_t kpver = sc_kp_ver(_key.c_str());
      //  printf("kpver: %lx\n",kpver);
        return kpver;
    }
    bool ready(){
        return sc_ready(_key.c_str());
    }


};





#endif // __MEM_HPP__

