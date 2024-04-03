#ifndef KSU_SUSFS_H
#define KSU_SUSFS_H

#include <linux/types.h>
#include <linux/utsname.h>

/* shared with userspace ksu_susfs tool */
#define CMD_SUSFS_ADD_SUSPICIOUS_PATH 0x55555
#define CMD_SUSFS_ADD_MOUNT_TYPE 0x55556
#define CMD_SUSFS_ADD_MOUNT_PATH 0x55557
#define CMD_SUSFS_ADD_SUSPICIOUS_KSTAT 0x55558
#define CMD_SUSFS_UPDATE_SUSPICIOUS_KSTAT 0x55559
#define CMD_SUSFS_ADD_TRY_UMOUNT 0x5555a
#define CMD_SUSFS_ADD_UNAME 0x5555b

#define SUSFS_MAX_LEN_PATHNAME 128
#define SUSFS_MAX_LEN_MOUNT_TYPE_NAME 32

#define HIDE_IN_MAPS_SPOOF_STAT_ONLY 0
#define HIDE_IN_MAPS_HIDE_ENTRYS 1
/* non shared to userspace ksu_susfs tool */
#define SYSCALL_FAMILY_ALL_ENOENT 0
#define SYSCALL_FAMILY_OPENAT 1
#define SYSCALL_FAMILY_MKNOD 2
#define SYSCALL_FAMILY_MKDIRAT 3
#define SYSCALL_FAMILY_RMDIR 4
#define SYSCALL_FAMILY_UNLINKAT 5
#define SYSCALL_FAMILY_SYMLINKAT_NEWNAME 6
#define SYSCALL_FAMILY_LINKAT_OLDNAME 7
#define SYSCALL_FAMILY_LINKAT_NEWNAME 8
#define SYSCALL_FAMILY_RENAMEAT2_OLDNAME 9
#define SYSCALL_FAMILY_RENAMEAT2_NEWNAME 10
#define SYSCALL_FAMILY_TRUNCATE 11
#define SYSCALL_FAMILY_FACCESSAT 12
#define SYSCALL_FAMILY_CHDIR 13

#define getname_safe(name) (name == NULL ? ERR_PTR(-EINVAL) : getname(name))
#define putname_safe(name) (IS_ERR(name) ? NULL : putname(name))

#define uid_matches_suspicious_path() (current_uid().val >= 10000)
#define uid_matches_suspicious_mount() (current_uid().val >= 10000)
#define uid_matches_suspicious_kstat() (current_uid().val >= 10000)

struct st_susfs_suspicious_path {
    char                   name[SUSFS_MAX_LEN_PATHNAME];
    unsigned long          ino;
};

struct st_susfs_suspicious_mount_type {
    char                   name[SUSFS_MAX_LEN_MOUNT_TYPE_NAME];
};

struct st_susfs_suspicious_mount_path {
    char                   name[SUSFS_MAX_LEN_PATHNAME];
    unsigned long          ino;
};

struct st_susfs_suspicious_kstat {
    unsigned long          target_ino; // the ino after bind mounted or overlayed
    char                   target_pathname[SUSFS_MAX_LEN_PATHNAME];
    unsigned int           hide_in_maps;
    unsigned long          spoofed_ino;
    dev_t		           spoofed_dev;
	dev_t		           spoofed_rdev;
    umode_t		           spoofed_mode;
    unsigned int	       spoofed_st_nlink;
    unsigned int	       spoofed_st_uid;
    unsigned int	       spoofed_st_gid;
    long	               spoofed_atime_tv_sec;
    long	               spoofed_mtime_tv_sec;
    long	               spoofed_ctime_tv_sec;
    long		           spoofed_atime_tv_nsec;
    long		           spoofed_mtime_tv_nsec;
    long		           spoofed_ctime_tv_nsec;
};

struct st_susfs_try_umount {
    char                   name[SUSFS_MAX_LEN_PATHNAME];
    //bool                   check_mnt;
    //int                    flags;
};

struct st_susfs_suspicious_path_list {
    struct list_head                        list;
    struct st_susfs_suspicious_path         info;
};

struct st_susfs_suspicious_mount_type_list {
    struct list_head                        list;
    struct st_susfs_suspicious_mount_type   info;
};

struct st_susfs_suspicious_mount_path_list {
    struct list_head                        list;
    struct st_susfs_suspicious_mount_path   info;
};

struct st_susfs_suspicious_kstat_list {
    struct list_head                        list;
    struct st_susfs_suspicious_kstat        info;
};

struct st_susfs_try_umount_list {
    struct list_head                        list;
    struct st_susfs_try_umount              info;
};

struct st_susfs_uname {
    char        sysname[__NEW_UTS_LEN+1];
    char        nodename[__NEW_UTS_LEN+1];
    char        release[__NEW_UTS_LEN+1];
    char        version[__NEW_UTS_LEN+1];
    char        machine[__NEW_UTS_LEN+1];
};

int susfs_add_suspicious_path(struct st_susfs_suspicious_path* __user user_info);
int susfs_add_mount_type(struct st_susfs_suspicious_mount_type* __user user_info);
int susfs_add_mount_path(struct st_susfs_suspicious_mount_path* __user user_info);
int susfs_add_suspicious_kstat(struct st_susfs_suspicious_kstat* __user user_info);
int susfs_update_suspicious_kstat(struct st_susfs_suspicious_kstat* __user user_info);
int susfs_add_try_umount(struct st_susfs_try_umount* __user user_info);
int susfs_add_uname(struct st_susfs_uname* __user user_info);

int susfs_is_suspicious_path(struct path* file, int* errno_to_be_changed, int syscall_family);
int susfs_suspicious_path(struct filename* name, int* errno_to_be_changed, int syscall_family);
int susfs_suspicious_ino_for_filldir64(unsigned long ino);
int susfs_is_suspicious_mount(struct vfsmount* mnt, struct path* root);
void susfs_suspicious_kstat(unsigned long ino, struct stat* out_stat);
int susfs_suspicious_kstat_or_hide_in_maps(unsigned long target_ino, unsigned long* orig_ino, dev_t* orig_dev);
void susfs_try_umount(void);
void susfs_spoof_uname(struct new_utsname* tmp);

void susfs_change_error_no_by_pathname(char* pathname, int* errno_to_be_changed, int syscall_family);

void __init susfs_init(void);

#endif
