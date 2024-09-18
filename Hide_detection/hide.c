static const struct proc_fs_info mnt_info[] = {
		{ MNT_NOSUID, ",nosuid" },
		{ MNT_NODEV, ",nodev" },
		{ MNT_NOEXEC, ",noexec" },
		{ MNT_NOATIME, ",noatime" },
		{ MNT_NODIRATIME, ",nodiratime" },
		{ MNT_RELATIME, ",relatime" },
		{ 0, NULL }
	};
// 用于过滤的关键词
static const char *blacklist[] = {
    "overlay", "magisk", "APatch", "zygisk", "dex2oat"
};
static const char* modulemnt = "/data/adbs";
//思路来自Zygisk-Assistant
/*
`/proc/mounts`: 使用 `proc_mounts_show` 函数输出挂载点信息。

`/proc/self/mountinfo`: 使用 `show_mountinfo` 函数输出详细的挂载信息。 Zygisk-Assistant检测到会进行umount2，如何重挂载

在输出挂载信息时，检查是否包含敏感的挂载信息，如 `overlay` 或 `magisk`，如果发现则清空输出。
*/
