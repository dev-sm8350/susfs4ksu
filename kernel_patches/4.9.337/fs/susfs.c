#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/printk.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/list.h>
#include <linux/init_task.h>
#include <linux/spinlock.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/susfs.h>

LIST_HEAD(LH_SUSPICIOUS_PATH);
LIST_HEAD(LH_KSTAT_SPOOFER);
LIST_HEAD(LH_SUSPICIOUS_MOUNT_TYPE);
LIST_HEAD(LH_SUSPICIOUS_MOUNT_PATH);
LIST_HEAD(LH_TRY_UMOUNT_PATH);
struct st_susfs_uname my_uname;

spinlock_t susfs_spin_lock;
bool is_log_enable = true;
#define SUSFS_LOGI(fmt, ...) if (is_log_enable) pr_info("susfs: " fmt, ##__VA_ARGS__)
#define SUSFS_LOGE(fmt, ...) if (!is_log_enable) pr_err("susfs: " fmt, ##__VA_ARGS__)


int susfs_add_sus_path(struct st_susfs_suspicious_path* __user user_info) {
    struct st_susfs_suspicious_path_list *cursor, *temp;
    struct st_susfs_suspicious_path_list *new_list = NULL;
	struct st_susfs_suspicious_path info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_suspicious_path))) {
		SUSFS_LOGE("failed copying from userspace.\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_SUSPICIOUS_PATH, list) {
        if (!strcmp(info.name, cursor->info.name)) {
            SUSFS_LOGE("pathname: '%s' is already created in LH_SUSPICIOUS_PATH.\n", info.name);
            return 1;
        }
    }

    new_list = kmalloc(sizeof(struct st_susfs_suspicious_path_list), GFP_KERNEL);
    if (!new_list) {
		SUSFS_LOGE("No enough memory.\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(struct st_susfs_suspicious_path));

    INIT_LIST_HEAD(&new_list->list);
    spin_lock(&susfs_spin_lock);
    list_add_tail(&new_list->list, &LH_SUSPICIOUS_PATH);
    spin_unlock(&susfs_spin_lock);
    SUSFS_LOGI("pathname: '%s' is successfully added to LH_SUSPICIOUS_PATH.\n", info.name);
    return 0;
}

int susfs_add_sus_mount_type(struct st_susfs_suspicious_mount_type* __user user_info) {
    struct st_susfs_suspicious_mount_type_list *cursor, *temp;
    struct st_susfs_suspicious_mount_type_list *new_list = NULL;
	struct st_susfs_suspicious_mount_type info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_suspicious_mount_type))) {
		SUSFS_LOGE("failed copying from userspace.\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_SUSPICIOUS_MOUNT_TYPE, list) {
        if (!strcmp(info.name, cursor->info.name)) {
            SUSFS_LOGE("mount_type_name: '%s' is already created in LH_SUSPICIOUS_MOUNT_TYPE.\n", info.name);
            return 1;
        }
    }

    new_list = kmalloc(sizeof(struct st_susfs_suspicious_mount_type_list), GFP_KERNEL);
    if (!new_list) {
		SUSFS_LOGE("No enough memory.\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(struct st_susfs_suspicious_mount_type));

    INIT_LIST_HEAD(&new_list->list);
    spin_lock(&susfs_spin_lock);
    list_add_tail(&new_list->list, &LH_SUSPICIOUS_MOUNT_TYPE);
    spin_unlock(&susfs_spin_lock);
    SUSFS_LOGI("mount_type_name: '%s' is successfully added to LH_SUSPICIOUS_MOUNT_TYPE.\n", info.name);
    return 0;
}

int susfs_add_sus_mount_path(struct st_susfs_suspicious_mount_path* __user user_info) {
    struct st_susfs_suspicious_mount_path_list *cursor, *temp;
    struct st_susfs_suspicious_mount_path_list *new_list = NULL;
	struct st_susfs_suspicious_mount_path info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_suspicious_mount_path))) {
		SUSFS_LOGE("failed copying from userspace.\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_SUSPICIOUS_MOUNT_PATH, list) {
        if (!strcmp(info.name, cursor->info.name)) {
            SUSFS_LOGE("mount_path: '%s' is already created in LH_SUSPICIOUS_MOUNT_PATH.\n", info.name);
            return 1;
        }
    }

    new_list = kmalloc(sizeof(struct st_susfs_suspicious_mount_path_list), GFP_KERNEL);
    if (!new_list) {
		SUSFS_LOGE("No enough memory.\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(struct st_susfs_suspicious_mount_path));

    INIT_LIST_HEAD(&new_list->list);
    spin_lock(&susfs_spin_lock);
    list_add_tail(&new_list->list, &LH_SUSPICIOUS_MOUNT_PATH);
    spin_unlock(&susfs_spin_lock);
    SUSFS_LOGI("mount_path: '%s' is successfully added to LH_SUSPICIOUS_MOUNT_PATH.\n", info.name);
    return 0;
}

int susfs_add_sus_kstat(struct st_susfs_suspicious_kstat* __user user_info) {
    struct st_susfs_suspicious_kstat_list *cursor, *temp;
    struct st_susfs_suspicious_kstat_list *new_list = NULL;
	struct st_susfs_suspicious_kstat info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_suspicious_kstat))) {
		SUSFS_LOGE("failed copying from userspace.\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_KSTAT_SPOOFER, list) {
        if (cursor->info.target_ino == info.target_ino) {
            if (strlen(info.target_pathname) > 0) {
                SUSFS_LOGE("target_pathname: '%s' is already created in LH_KSTAT_SPOOFER.\n", info.target_pathname);
            } else {
                SUSFS_LOGE("target_ino: '%lu' is already created in LH_KSTAT_SPOOFER.\n", info.target_ino);
            }
            return 1;
        }
    }

    new_list = kmalloc(sizeof(struct st_susfs_suspicious_kstat_list), GFP_KERNEL);
    if (!new_list) {
		SUSFS_LOGE("No enough memory.\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(struct st_susfs_suspicious_kstat));
	/* Seems the dev number issue is finally solved, the userspace stat we see is already a encoded dev
	 * which is set by new_encode_dev() / huge_encode_dev() function for 64bit system and 
	 * old_encode_dev() for 32bit only system, that's why we need to decode it in kernel as well,
	 * and different kernel may have different function to encode the dev number, be cautious!
	 * Also check your encode_dev() macro in fs/stat.c to determine which one to use 
	 */
#if defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64)
#ifdef CONFIG_MIPS
	new_list->info.spoofed_dev = new_decode_dev(new_list->info.spoofed_dev);
#else
	new_list->info.spoofed_dev = huge_decode_dev(new_list->info.spoofed_dev);
#endif /* CONFIG_MIPS */
#else
	new_list->info.spoofed_dev = old_decode_dev(new_list->info.spoofed_dev);
#endif /* defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64) */
    INIT_LIST_HEAD(&new_list->list);
    spin_lock(&susfs_spin_lock);
    list_add_tail(&new_list->list, &LH_KSTAT_SPOOFER);
    spin_unlock(&susfs_spin_lock);
    if (strlen(new_list->info.target_pathname) > 0) {
        SUSFS_LOGI("target_pathname: '%s' is successfully added to LH_KSTAT_SPOOFER, original dev: %lu\n", new_list->info.target_pathname, new_list->info.spoofed_dev);
    } else {
        SUSFS_LOGI("ino: '%lu' is successfully added to LH_KSTAT_SPOOFER, original dev: %lu\n", new_list->info.target_ino, new_list->info.spoofed_dev);
    }
    return 0;
}

int susfs_update_sus_kstat(struct st_susfs_suspicious_kstat* __user user_info) {
    struct st_susfs_suspicious_kstat_list *cursor, *temp;
	struct st_susfs_suspicious_kstat info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_suspicious_kstat))) {
		SUSFS_LOGE("failed copying from userspace.\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_KSTAT_SPOOFER, list) {
        if (!strcmp(info.target_pathname, cursor->info.target_pathname)) {
            SUSFS_LOGI("updating target_ino for pathname: '%s' in LH_KSTAT_SPOOFER.\n", info.target_pathname);
			cursor->info.target_ino = info.target_ino;
            return 0;
        }
    }

	SUSFS_LOGE("pathname: '%s' is not found in LH_KSTAT_SPOOFER.\n", info.target_pathname);
	return 1;
}

int susfs_add_try_umount(struct st_susfs_try_umount* __user user_info) {
    struct st_susfs_try_umount_list *cursor, *temp;
    struct st_susfs_try_umount_list *new_list = NULL;
	struct st_susfs_try_umount info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_try_umount))) {
		SUSFS_LOGE("failed copying from userspace.\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_TRY_UMOUNT_PATH, list) {
        if (!strcmp(info.name, cursor->info.name)) {
            SUSFS_LOGE("name: '%s' is already created in LH_TRY_UMOUNT_PATH.\n", info.name);
            return 1;
        }
    }

    new_list = kmalloc(sizeof(struct st_susfs_try_umount_list), GFP_KERNEL);
    if (!new_list) {
		SUSFS_LOGE("No enough memory.\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(struct st_susfs_try_umount));

    INIT_LIST_HEAD(&new_list->list);
    spin_lock(&susfs_spin_lock);
    list_add_tail(&new_list->list, &LH_TRY_UMOUNT_PATH);
    spin_unlock(&susfs_spin_lock);
    SUSFS_LOGI("pathname: '%s' is successfully added to LH_TRY_UMOUNT_PATH.\n", new_list->info.name);
    return 0;
}

int susfs_set_uname(struct st_susfs_uname* __user user_info) {
	struct st_susfs_uname info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_uname))) {
		SUSFS_LOGE("failed copying from userspace.\n");
		return 1;
	}

	spin_lock(&susfs_spin_lock);
	strncpy(my_uname.sysname, info.sysname, __NEW_UTS_LEN);
	strncpy(my_uname.nodename, info.nodename, __NEW_UTS_LEN);
	strncpy(my_uname.release, info.release, __NEW_UTS_LEN);
	strncpy(my_uname.version, info.version, __NEW_UTS_LEN);
	strncpy(my_uname.machine, info.machine, __NEW_UTS_LEN);
    spin_unlock(&susfs_spin_lock);
	return 0;
}

int susfs_is_suspicious_path(struct path* file, int* errno_to_be_changed, int syscall_family) {
	size_t size = 4096;
	int res = -1;
	int status = 0;
	char* path = NULL;
	char* ptr = NULL;
	char* end = NULL;
	struct st_susfs_suspicious_path_list *cursor, *temp;

	if (!uid_matches_suspicious_path() || file == NULL) {
		status = 0;
		goto out;
	}

	path = kmalloc(size, GFP_KERNEL);

	if (path == NULL) {
		status = -1;
		goto out;
	}

	ptr = d_path(file, path, size);

	if (IS_ERR(ptr)) {
		status = -1;
		goto out;
	}

	end = mangle_path(path, ptr, " \t\n\\");

	if (!end) {
		status = -1;
		goto out;
	}

	res = end - path;
	path[(size_t) res] = '\0';

    list_for_each_entry_safe(cursor, temp, &LH_SUSPICIOUS_PATH, list) {
        if (!strcmp(cursor->info.name, path)) {
            SUSFS_LOGI("hiding pathname '%s' for UID %i\n", cursor->info.name, current_uid().val);
			if (errno_to_be_changed != NULL) {
				susfs_change_error_no_by_pathname(path, errno_to_be_changed, syscall_family);
			}
			status = 1;
			goto out;
        }
    }

out:
	kfree(path);
	return status;
}

int susfs_suspicious_path(struct filename* name, int* errno_to_be_changed, int syscall_family) {
	int status = 0;
	int ret = 0;
	struct path path;

	if (IS_ERR(name)) {
		return -1;
	}

	if (!uid_matches_suspicious_path() || name == NULL) {
		return 0;
	}

	ret = kern_path(name->name, LOOKUP_FOLLOW, &path);

	if (!ret) {
		status = susfs_is_suspicious_path(&path, errno_to_be_changed, syscall_family);
		path_put(&path);
	}

	return status;
}

int susfs_suspicious_ino_for_filldir64(unsigned long ino) {
    struct st_susfs_suspicious_path_list *cursor, *temp;

	if (!uid_matches_suspicious_path()) return 0;
	list_for_each_entry_safe(cursor, temp, &LH_SUSPICIOUS_PATH, list) {
        if (cursor->info.ino == ino) {
            SUSFS_LOGI("hiding pathname '%s' for UID %i\n", cursor->info.name, current_uid().val);
			return 1;
        }
    }
	return 0;
}

int susfs_is_suspicious_mount(struct vfsmount* mnt, struct path* root) {
	size_t size = 4096;
	int res = -1;
	int status = 0;
	char* path = NULL;
	char* ptr = NULL;
	char* end = NULL;
	struct st_susfs_suspicious_mount_type_list *cursor_mount_type, *temp_mount_type;
	struct st_susfs_suspicious_mount_path_list *cursor_mount_path, *temp_mount_path;
	struct path mnt_path = {
		.dentry = mnt->mnt_root,
		.mnt = mnt
	};

	if (!uid_matches_suspicious_mount()) {
		status = 0;
		goto out;
	}

	list_for_each_entry_safe(cursor_mount_type, temp_mount_type, &LH_SUSPICIOUS_MOUNT_TYPE, list) {
        if (!strcmp(mnt->mnt_root->d_sb->s_type->name, cursor_mount_type->info.name)) {
            SUSFS_LOGI("mount point with suspicious type '%s' won't be shown to process with UID %i\n", mnt->mnt_root->d_sb->s_type->name, current_uid().val);
			status = 1;
			goto out;
        }
    }

	path = kmalloc(size, GFP_KERNEL);

	if (path == NULL) {
		status = -1;
		goto out;
	}

	ptr = __d_path(&mnt_path, root, path, size);

	if (!ptr) {
		status = -1;
		goto out;
	}

	end = mangle_path(path, ptr, " \t\n\\");

	if (!end) {
		status = -1;
		goto out;
	}

	res = end - path;
	path[(size_t) res] = '\0';

	list_for_each_entry_safe(cursor_mount_path, temp_mount_path, &LH_SUSPICIOUS_MOUNT_PATH, list) {
        if (!strcmp(path, cursor_mount_path->info.name)) {
            SUSFS_LOGI("mount point with suspicious path '%s' won't be shown to process with UID %i\n", path, current_uid().val);
			status = 1;
			goto out;
        }
    }

out:
	kfree(path);
	return status;
}

void susfs_suspicious_kstat(unsigned long ino, struct stat* out_stat) {
    struct st_susfs_suspicious_kstat_list *cursor, *temp;

	if (!uid_matches_suspicious_kstat()) return;
	list_for_each_entry_safe(cursor, temp, &LH_KSTAT_SPOOFER, list) {
        if (cursor->info.target_ino == ino && !cursor->info.spoof_in_maps_only) {
            SUSFS_LOGI("spoofing kstat for pathname '%s' for UID %i\n", cursor->info.target_pathname, current_uid().val);
			out_stat->st_ino = cursor->info.spoofed_ino;
#if defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64)
#ifdef CONFIG_MIPS
			out_stat->st_dev = new_encode_dev(cursor->info.spoofed_dev);
#else
			out_stat->st_dev = huge_encode_dev(cursor->info.spoofed_dev);
#endif /* CONFIG_MIPS */
#else
			out_stat->st_dev = old_encode_dev(cursor->info.spoofed_dev);
#endif /* defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64) */
			out_stat->st_atime = cursor->info.spoofed_atime_tv_sec;
			out_stat->st_mtime = cursor->info.spoofed_mtime_tv_sec;
			out_stat->st_ctime = cursor->info.spoofed_ctime_tv_sec;
#ifdef _STRUCT_TIMESPEC
			out_stat->st_atime_nsec = cursor->info.spoofed_atime_tv_nsec;
			out_stat->st_mtime_nsec = cursor->info.spoofed_mtime_tv_nsec;
			out_stat->st_ctime_nsec = cursor->info.spoofed_ctime_tv_nsec;
#endif
			return;
        }
    }
}

int susfs_suspicious_maps(unsigned long target_ino, unsigned long* orig_ino, dev_t* orig_dev, char *tmpname) {
    struct st_susfs_suspicious_kstat_list *cursor, *temp;

	if (!uid_matches_suspicious_kstat()) return 0;
	list_for_each_entry_safe(cursor, temp, &LH_KSTAT_SPOOFER, list) {
        if (cursor->info.target_ino == target_ino) {
            if (!cursor->info.spoof_in_maps_only) {
                if (target_ino != 0) {
                    *orig_ino = cursor->info.spoofed_ino;
                    *orig_dev = cursor->info.spoofed_dev;
                    SUSFS_LOGI("[uid:%u] spoofing ino: '%lu' -> ino: %lu, dev: 0x%x, MAJOR(dev): 0x%x, MINOR(dev): 0x%x, target_pathname: %s\n", current_uid().val, target_ino, *orig_ino, *orig_dev, MAJOR(*orig_dev), MINOR(*orig_dev), cursor->info.target_pathname);
                    return 0;
                }
            } else {
                if (target_ino != 0) {
                    *orig_ino = cursor->info.spoofed_ino;
                    *orig_dev = cursor->info.spoofed_dev;
					strncpy(tmpname, cursor->info.spoofed_pathname, SUSFS_MAX_LEN_PATHNAME);
					SUSFS_LOGI("[uid:%u] spoofing ino: '%lu' -> ino: %lu, dev: 0x%x, MAJOR(dev): 0x%x, MINOR(dev): 0x%x, pathname: %s\n", current_uid().val, target_ino, *orig_ino, *orig_dev, MAJOR(*orig_dev), MINOR(*orig_dev), cursor->info.spoofed_pathname);
                    return 1;
                }
            }
        }
    }
	return 0;
}


static void umount_mnt(struct path *path, int flags) {
	int err = path_umount(path, flags);
	if (err) {
		SUSFS_LOGI("umount %s failed: %d\n", path->dentry->d_iname, err);
	}
}

static bool should_umount(struct path *path)
{
	if (!path) {
		return false;
	}

	if (current->nsproxy->mnt_ns == init_nsproxy.mnt_ns) {
		SUSFS_LOGI("ignore global mnt namespace process: %d\n",
			current_uid().val);
		return false;
	}

	if (path->mnt && path->mnt->mnt_sb && path->mnt->mnt_sb->s_type) {
		const char *fstype = path->mnt->mnt_sb->s_type->name;
		return strcmp(fstype, "overlay") == 0;
	}
	return false;
}

static void try_umount(const char *mnt, bool check_mnt, int flags) {
	struct path path;
	int err = kern_path(mnt, 0, &path);

	if (err) {
		return;
	}

	if (path.dentry != path.mnt->mnt_root) {
		// it is not root mountpoint, maybe umounted by others already.
		return;
	}

	// we are only interest in some specific mounts
	if (check_mnt && !should_umount(&path)) {
		return;
	}
	
	umount_mnt(&path, flags);
}

void susfs_try_umount(uid_t target_uid) {
    struct st_susfs_try_umount_list *cursor, *temp;

	list_for_each_entry_safe(cursor, temp, &LH_TRY_UMOUNT_PATH, list) {
		SUSFS_LOGI("umounting '%s' for uid: %d\n", cursor->info.name, target_uid);
        try_umount(cursor->info.name, false, MNT_DETACH);
    }
}

void susfs_spoof_uname(struct new_utsname* tmp) {
	if (strcmp(my_uname.sysname, "default")) {
		memset(tmp->sysname, 0, __NEW_UTS_LEN);
		strncpy(tmp->sysname, my_uname.sysname, __NEW_UTS_LEN);
	}
	if (strcmp(my_uname.nodename, "default")) {
		memset(tmp->nodename, 0, __NEW_UTS_LEN);
		strncpy(tmp->nodename, my_uname.nodename, __NEW_UTS_LEN);
	}
	if (strcmp(my_uname.release, "default")) {
		memset(tmp->release, 0, __NEW_UTS_LEN);
		strncpy(tmp->release, my_uname.release, __NEW_UTS_LEN);
	}
	if (strcmp(my_uname.version, "default")) {
		memset(tmp->version, 0, __NEW_UTS_LEN);
		strncpy(tmp->version, my_uname.version, __NEW_UTS_LEN);
	}
	if (strcmp(my_uname.machine, "default")) {
		memset(tmp->machine, 0, __NEW_UTS_LEN);
		strncpy(tmp->machine, my_uname.machine, __NEW_UTS_LEN);
	}
	SUSFS_LOGI("spoofed uname to '%s %s %s %s %s' for uid: %d\n", 
				tmp->sysname, tmp->nodename, tmp->release, tmp->version, tmp->machine, current_uid().val);
}

void susfs_set_log(bool enabled) {
	spin_lock(&susfs_spin_lock);
	is_log_enable = enabled;
	spin_unlock(&susfs_spin_lock);
	if (is_log_enable) {
		pr_info("susfs: enable logging to kernel");
	} else if (!is_log_enable) {
		pr_info("susfs: disable logging to kernel");
	}
}

/* For files/directories in /sdcard/ but not in /sdcard/Android/data/, please delete it  
 * by yourself
 */
void susfs_change_error_no_by_pathname(char* const pathname, int* const errno_to_be_changed, int const syscall_family) {
	if (!strncmp(pathname, "/system/", 8)||
		!strncmp(pathname, "/vendor/", 8)) {
		switch(syscall_family) {
			case SYSCALL_FAMILY_ALL_ENOENT:
				*errno_to_be_changed = -ENOENT;
				return;
			case SYSCALL_FAMILY_LINKAT_OLDNAME:
				*errno_to_be_changed = -ENOENT;
				return;
			case SYSCALL_FAMILY_RENAMEAT2_OLDNAME:
				*errno_to_be_changed = -EXDEV;
				return;
			//case SYSCALL_FAMILY_RENAMEAT2_NEWNAME:
			//	if (!strncmp(pathname, "/system/", 8)) {
			//		*errno_to_be_changed = -EROFS;
			//	} else {
			//		*errno_to_be_changed = -EXDEV;
			//	}
			//	return;
			default:
				*errno_to_be_changed = -EROFS;
				return;
		}
	} else if (!strncmp(pathname, "/storage/emulated/0/Android/data/", 33)) {
		switch(syscall_family) {
			case SYSCALL_FAMILY_ALL_ENOENT:
				*errno_to_be_changed = -ENOENT;
				return;
			case SYSCALL_FAMILY_MKNOD:
				*errno_to_be_changed = -EACCES;
				return;
			case SYSCALL_FAMILY_MKDIRAT:
				*errno_to_be_changed = -EACCES;
				return;
			case SYSCALL_FAMILY_RMDIR:
				*errno_to_be_changed = -ENOENT;
				return;
			case SYSCALL_FAMILY_UNLINKAT:
				*errno_to_be_changed = -ENOENT;
				return;
			case SYSCALL_FAMILY_SYMLINKAT_NEWNAME:
				*errno_to_be_changed = -EACCES;
				return;
			case SYSCALL_FAMILY_LINKAT_OLDNAME:
				*errno_to_be_changed = -ENOENT;
				return;
			case SYSCALL_FAMILY_LINKAT_NEWNAME:
				*errno_to_be_changed = -EXDEV;
				return;
			case SYSCALL_FAMILY_RENAMEAT2_OLDNAME:
				*errno_to_be_changed = -EXDEV;
				return;
			case SYSCALL_FAMILY_RENAMEAT2_NEWNAME:
				*errno_to_be_changed = -EXDEV;
				return;
			default:
				*errno_to_be_changed = -ENOENT;
				return;
		}
	} else if (!strncmp(pathname, "/dev/", 5)) {
		switch(syscall_family) {
			case SYSCALL_FAMILY_ALL_ENOENT:
				*errno_to_be_changed = -ENOENT;
				return;
			case SYSCALL_FAMILY_MKNOD:
				*errno_to_be_changed = -EACCES;
				return;
			case SYSCALL_FAMILY_MKDIRAT:
				*errno_to_be_changed = -EACCES;
				return;
			case SYSCALL_FAMILY_RMDIR:
				*errno_to_be_changed = -ENOENT;
				return;
			case SYSCALL_FAMILY_UNLINKAT:
				*errno_to_be_changed = -ENOENT;
				return;
			case SYSCALL_FAMILY_SYMLINKAT_NEWNAME:
				*errno_to_be_changed = -EACCES;
				return;
			case SYSCALL_FAMILY_LINKAT_OLDNAME:
				*errno_to_be_changed = -ENOENT;
				return;
			case SYSCALL_FAMILY_LINKAT_NEWNAME:
				*errno_to_be_changed = -EXDEV;
				return;
			case SYSCALL_FAMILY_RENAMEAT2_OLDNAME:
				*errno_to_be_changed = -EXDEV;
				return;
			case SYSCALL_FAMILY_RENAMEAT2_NEWNAME:
				*errno_to_be_changed = -EXDEV;
				return;
			default:
				*errno_to_be_changed = -ENOENT;
				return;
		}
	}
}

static void susfs_my_uname_init(void) {
	memset(&my_uname, 0, sizeof(struct st_susfs_uname));
	strncpy(my_uname.sysname, "default", __NEW_UTS_LEN);
	strncpy(my_uname.nodename, "default", __NEW_UTS_LEN);
	strncpy(my_uname.release, "default", __NEW_UTS_LEN);
	strncpy(my_uname.version, "default", __NEW_UTS_LEN);
	strncpy(my_uname.machine, "default", __NEW_UTS_LEN);
}

void __init susfs_init(void) {
    spin_lock_init(&susfs_spin_lock);
	susfs_my_uname_init();
}

/* No module exit is needed becuase it should never be a loadable kernel module */
//void __init susfs_exit(void)
