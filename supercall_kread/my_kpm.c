


#include "my_kpm.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>


static inline long ver_and_cmd(const char *key, long cmd)
{
    uint32_t version_code = (MAJOR << 16) + (MINOR << 8) + PATCH;
    return ((long)version_code << 32) | (0x1158 << 16) | (cmd & 0xFFFF);
}


static inline uint32_t sc_kp_ver(const char *key)
{
    if (!key || !key[0]) return -EINVAL;
    long ret = syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KERNELPATCH_VER));
    return (uint32_t)ret;
}


static inline uint32_t sc_k_ver(const char *key)
{
    if (!key || !key[0]) return -EINVAL;
    long ret = syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KERNEL_VER));
    return (uint32_t)ret;
}

static inline long sc_hello(const char *key)
{
    if (!key || !key[0]) return -EINVAL;
    long ret = syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_HELLO));
    return ret;
}

static inline bool sc_ready(const char *key)
{
    return sc_hello(key) == SUPERCALL_HELLO_MAGIC;
}

static inline long sc_kpm_load(const char *key, const char *path, const char *args, void *reserved)
{
    if (!key || !key[0]) return -EINVAL;
    if (!path || strlen(path) <= 0) return -EINVAL;
    long ret = syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KPM_LOAD), path, args, reserved);
    return ret;
}

static inline long sc_kpm_control(const char *key, const char *name, const char *ctl_args, char *out_msg, long outlen)
{
    if (!key || !key[0]) return -EINVAL;
    if (!name || strlen(name) <= 0) return -EINVAL;
    if (!ctl_args || strlen(ctl_args) <= 0) return -EINVAL;
    long ret = syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KPM_CONTROL), name, ctl_args, out_msg, outlen);
    return ret;
}

static inline long sc_kpm_unload(const char *key, const char *name, void *reserved)
{
    if (!key || !key[0]) return -EINVAL;
    if (!name || strlen(name) <= 0) return -EINVAL;
    long ret = syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KPM_UNLOAD), name, reserved);
    return ret;
}
static inline long sc_kpm_info(const char *key, const char *name, char *buf, int buf_len)
{
    if (!key || !key[0]) return -EINVAL;
    if (!buf || buf_len <= 0) return -EINVAL;
    long ret = syscall(__NR_supercall, key, ver_and_cmd(key, SUPERCALL_KPM_INFO), name, buf, buf_len);
    return ret;
}

int kpm_load(const char *key, const char *path, const char *args)
{
    int rc = sc_kpm_load(key, path, args, 0);
    return rc;
}

int kpm_control(const char *key, const char *name, const char *ctl_args,char *out_byte_code)
{
    char buf[4096] = { '\0' };
    int rc = sc_kpm_control(key, name, ctl_args, buf, sizeof(buf));
    sprintf(out_byte_code, "%s", buf);
    return rc;
}

int kpm_unload(const char *key, const char *name)
{
    int rc = sc_kpm_unload(key, name, 0);
    return rc;
}

int kpm_info(const char *key, const char *name,char*outmsg)
{
    char buf[4096];
    int rc = sc_kpm_info(key, name, buf, sizeof(buf));
    if (rc > 0) {
        sprintf(outmsg, "%s", buf);
        return 0;
    }
    return rc;
}