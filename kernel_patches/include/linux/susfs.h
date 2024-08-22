#ifndef KSU_SUSFS_H
#define KSU_SUSFS_H

#include <linux/version.h>
#include <linux/types.h>
#include <linux/utsname.h>
#include <linux/mount.h>

/********/
/* ENUM */
/********/
/* shared with userspace ksu_susfs tool */
#define CMD_SUSFS_ADD_SUS_PATH 0x55555
#define CMD_SUSFS_ADD_SUS_MOUNT 0x55556
#define CMD_SUSFS_ADD_SUS_KSTAT 0x55558
#define CMD_SUSFS_UPDATE_SUS_KSTAT 0x55559
#define CMD_SUSFS_ADD_TRY_UMOUNT 0x5555a
#define CMD_SUSFS_SET_UNAME 0x5555b
#define CMD_SUSFS_ADD_SUS_KSTAT_STATICALLY 0x5555c
#define CMD_SUSFS_ENABLE_LOG 0x5555d
#define CMD_SUSFS_ADD_SUS_MAPS_STATICALLY 0x5555e
#define CMD_SUSFS_ADD_SUS_PROC_FD_LINK 0x5555f
#define CMD_SUSFS_ADD_SUS_MAPS 0x55560
#define CMD_SUSFS_UPDATE_SUS_MAPS 0x55561
#define CMD_SUSFS_ADD_SUS_MEMFD 0x55562
#define CMD_SUSFS_ADD_SUS_KSTATFS 0x55563
#define CMD_SUSFS_SUS_SU 0x60000

#define SUSFS_MAX_LEN_PATHNAME 256 // 256 should address many paths already unless you are doing some strange experimental stuff, then set your own desired length
#define SUSFS_MAX_LEN_MFD_NAME 248
#define SUSFS_MAX_SUS_MNTS 300 // I think 300 is now enough? This includes the mount entries for each process and sus mounts added by user 
#define SUSFS_MAX_SUS_MAPS 200 // I think 200 is now enough? Tell me why if you have over 200 entries

#define SUSFS_MAP_FILES_ACTION_REMOVE_WRITE_PERM 1
#define SUSFS_MAP_FILES_ACTION_HIDE_DENTRY 2

/*********/
/* MACRO */
/*********/
#define getname_safe(name) (name == NULL ? ERR_PTR(-EINVAL) : getname(name))
#define putname_safe(name) (IS_ERR(name) ? NULL : putname(name))

#define uid_matches_suspicious_path() (current_uid().val > 2000)
#define uid_matches_proc_need_to_reorder_mnt_id() (current_uid().val >= 10000)

/**********/
/* STRUCT */
/**********/
/* sus_path */
struct st_sus_path_err_retval {
	int other_syscalls;
	int mknodat;
	int mkdirat;
	int rmdir;
	int unlinkat;
	int symlinkat_newname;
	int linkat_oldname;
	int linkat_newname;
	int renameat2_oldname;
	int renameat2_newname;
};

struct st_susfs_sus_path {
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long           target_ino;
};

struct st_susfs_sus_path_list {
	struct list_head                        list;
	struct st_susfs_sus_path                info;
};

/* sus_mount */
struct st_susfs_sus_mount {
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long           target_dev;
};

struct st_susfs_sus_mount_list {
	struct list_head                        list;
	struct st_susfs_sus_mount               info;
};

/* sus_kstat */
struct st_susfs_sus_kstat {
	bool                    is_statically;
	unsigned long           target_ino; // the ino after bind mounted or overlayed
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long           spoofed_ino;
	unsigned long           spoofed_dev;
	unsigned int            spoofed_nlink;
	long long               spoofed_size;
	long                    spoofed_atime_tv_sec;
	long                    spoofed_mtime_tv_sec;
	long                    spoofed_ctime_tv_sec;
	long                    spoofed_atime_tv_nsec;
	long                    spoofed_mtime_tv_nsec;
	long                    spoofed_ctime_tv_nsec;
	unsigned long           spoofed_blksize;
	unsigned long long      spoofed_blocks;
};

struct st_susfs_sus_kstat_list {
	struct list_head                        list;
	struct st_susfs_sus_kstat               info;
};

/* sus_kstatfs */
struct st_susfs_sus_kstatfs {
	unsigned long           target_ino; // the ino after bind mounted or overlayed
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	char                    spoofed_pathname[SUSFS_MAX_LEN_PATHNAME];
};

struct st_susfs_sus_kstatfs_list {
	struct list_head                        list;
	struct st_susfs_sus_kstatfs             info;
};

/* sus_maps */
struct st_susfs_sus_maps {
	bool                    is_statically;
	int                     compare_mode;
	bool                    is_isolated_entry;
	bool                    is_file;
	unsigned long           prev_target_ino;
	unsigned long           next_target_ino;
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long           target_ino;
	unsigned long           target_dev;
	unsigned long long      target_pgoff;
	unsigned long           target_prot;
	unsigned long           target_addr_size;
	char                    spoofed_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long           spoofed_ino;
	unsigned long           spoofed_dev;
	unsigned long long      spoofed_pgoff;
	unsigned long           spoofed_prot;
	bool                    need_to_spoof_pathname;
	bool                    need_to_spoof_ino;
	bool                    need_to_spoof_dev;
	bool                    need_to_spoof_pgoff;
	bool                    need_to_spoof_prot;
};

struct st_susfs_sus_maps_list {
	struct list_head                        list;
	struct st_susfs_sus_maps                info;
};

/* try_umount */
struct st_susfs_try_umount {
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	int                     mnt_mode;
};

struct st_susfs_try_umount_list {
	struct list_head                        list;
	struct st_susfs_try_umount              info;
};

/* sus_proc_fd_link */
struct st_susfs_sus_proc_fd_link {
	char                    target_link_name[SUSFS_MAX_LEN_PATHNAME];
	char                    spoofed_link_name[SUSFS_MAX_LEN_PATHNAME];
};

struct st_susfs_sus_proc_fd_link_list {
	struct list_head                        list;
	struct st_susfs_sus_proc_fd_link        info;
};

/* sus_memfd */
struct st_susfs_sus_memfd {
	char                    target_pathname[SUSFS_MAX_LEN_MFD_NAME];
};

struct st_susfs_sus_memfd_list {
	struct list_head                        list;
	struct st_susfs_sus_memfd               info;
};

/* spoof_uname */
struct st_susfs_uname {
	char        release[__NEW_UTS_LEN+1];
	char        version[__NEW_UTS_LEN+1];
};

/* sus_su */
struct st_sus_su {
	bool        enabled;
	char        drv_path[256];
	int         maj_dev_num;
};

/***********************/
/* FORWARD DECLARATION */
/***********************/
/* sus_path */
int susfs_add_sus_path(struct st_susfs_sus_path* __user user_info);
int susfs_sus_ino_for_filldir64(unsigned long ino);
int susfs_is_sus_path_list_empty(void);
/* sus_mount */
int susfs_add_sus_mount(struct st_susfs_sus_mount* __user user_info);
void susfs_sus_mount(struct mnt_namespace *ns);
/* sus_kstat */
int susfs_add_sus_kstat(struct st_susfs_sus_kstat* __user user_info);
int susfs_update_sus_kstat(struct st_susfs_sus_kstat* __user user_info);
/* sus_kstatfs */
int susfs_add_sus_kstatfs(struct st_susfs_sus_kstatfs* __user user_info);
/* sus_maps */
int susfs_add_sus_maps(struct st_susfs_sus_maps* __user user_info);
int susfs_update_sus_maps(struct st_susfs_sus_maps* __user user_info);
int susfs_sus_maps(unsigned long target_ino, unsigned long target_addr_size,
					unsigned long* orig_ino, dev_t* orig_dev, vm_flags_t* flags,
					unsigned long long* pgoff, struct vm_area_struct* vma, char* out_name);
int susfs_is_sus_maps_list_empty(void);
/* sus_map_files */
void susfs_sus_map_files_readlink(unsigned long target_ino, char* pathname);
int susfs_sus_map_files_instantiate(struct vm_area_struct* vma);
/* sus_proc_fd_link */
int susfs_add_sus_proc_fd_link(struct st_susfs_sus_proc_fd_link* __user user_info);
int susfs_sus_proc_fd_link(char *pathname, int len);
int susfs_is_sus_proc_fd_link_list_empty(void);
/* sus_memfd */
int susfs_add_sus_memfd(struct st_susfs_sus_memfd* __user user_info);
int susfs_sus_memfd(char *memfd_name);
/* try_umount */
int susfs_add_try_umount(struct st_susfs_try_umount* __user user_info);
void susfs_try_umount(uid_t target_uid);
/* spoof_uname */
int susfs_set_uname(struct st_susfs_uname* __user user_info);
int susfs_spoof_uname(struct new_utsname* tmp);
/* set_log */
void susfs_set_log(bool enabled);
/* sus_su */
#ifdef CONFIG_KSU_SUSFS_SUS_SU
int susfs_sus_su(struct st_sus_su* __user user_info);
#endif
/* susfs_init */
void __init susfs_init(void);

/* not used */
//void susfs_d_path(struct vfsmount *vfs_mnt, struct dentry *vfs_dentry);


#endif
