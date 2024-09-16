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
    std::string _name = "kpmreadv";
    int _pid;
   // char result[4096];
public:
    Mem(std::string key): _key(key) {}
    //ctl0 name args "pid_addr_size"
//     std::string read(std::string args,int size){
//         if(args.empty()){
//             return "";
//         }
//         if(size > 4096){
//             return "size too big";
//         }
//         char result[4096];
//         memset(result,0,4096);
//         long ret = sc_kpm_control(_key.c_str(),_name.c_str(),args.c_str(),result,sizeof(result));
//         if(ret < 0){
//             return "";
//         }
//         //打印字节码
// //        std::cout << "字节码: \n";
// //        for(int i = 0; i < 4096; i++){
// //            printf("%02x ",result[i]);
// //        }
//         return std::string(result,size);
//     }

    bool read(std::string args,unsigned char* result,int size){
        if(args.empty()){
            return 0 ;
        }
        if(size > 4096){
            return 0 ;
        }
        memset(result,0,size);
        long ret = sc_kpm_control(_key.c_str(),_name.c_str(),args.c_str(),(char*)result,sizeof(result));
        if(ret < 0){
            return 0 ;
        }
        return 1 ;
    }

    //获取模块基址    //1pid_name   /a/b/c/d/libxxx.so name = libxxx.so
    uint64_t get_module_base(std::string name){
        uint64_t base = 0;
        if(name.empty()){
            return 0;
        }
        std::string cmd = "1"+std::to_string(_pid) + "_" + name;

        long ret = sc_kpm_control(_key.c_str(),name.c_str(),cmd.c_str(),(char*)&base,sizeof(base));
        if(ret < 0){
            return 0;
        }
        return base;
    }

    void ini(int pid){
        this->_pid = pid;
    }
    //解析字节码
    template<typename T>
    T parse(uint64_t addr){
        if(!addr)return 0;
        //addr 转成16进制字符串
        std::stringstream ss;
        ss << std::hex << addr;
        //std::cout << "addr: " << ss.str() << std::endl;
        std::string args = "0" +std::to_string(_pid) + "_" + ss.str() + "_" + std::to_string(sizeof(T));
//        std::string result = read(args,sizeof(T));
//        if(result.empty()){
//            return 0;
//        }
//        return *(T*)result.data();

        T result;
        read(args,(unsigned char*)&result,sizeof(T));
        return result;

    }

    uintptr_t readptr(uint64_t addr){
        return parse<uintptr_t>(addr);
    }
    uintptr_t readptrs(std::vector<uint64_t> addrs){
        uintptr_t result = parse<uintptr_t>(addrs[0]);
        for(int i = 1; i < addrs.size(); i++){
            result = parse<uintptr_t>(result + addrs[i]);
        }
        return result;
    }
    int readint(uint64_t addr){
        return parse<int>(addr);
    }
    float readfloat(uint64_t addr){
        return parse<float>(addr);
    }
    double readdouble(uint64_t addr){
        return parse<double>(addr);
    }

    std::string  loadkpm(std::string path,std::string args){
        long ret = sc_kpm_load(_key.c_str(),path.c_str(),args.c_str(),0);
        //std::cout << "ret: " << ret << std::endl;
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
    std::string get_info(){
        char result[4096];//字节码
        memset(result,0,4096);
        int ret = sc_kpm_info(_key.c_str(),_name.c_str(),result,sizeof(result));
        if(ret < 0){
            return "";
        }
        return std::string(result,4096);
    }

};






#endif // __MEM_HPP__

