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

LIST_HEAD(LH_SUS_PATH);
LIST_HEAD(LH_KSTAT_SPOOFER);
LIST_HEAD(LH_SUS_MOUNT);
LIST_HEAD(LH_MAPS_SPOOFER);
LIST_HEAD(LH_SUS_PROC_FD_LINK);
LIST_HEAD(LH_TRY_UMOUNT_PATH);
struct st_susfs_uname my_uname;

spinlock_t susfs_spin_lock;
bool is_log_enable = true;
#define SUSFS_LOGI(fmt, ...) if (is_log_enable) pr_info("susfs: " fmt, ##__VA_ARGS__)
#define SUSFS_LOGE(fmt, ...) if (is_log_enable) pr_err("susfs: " fmt, ##__VA_ARGS__)


int susfs_add_sus_path(struct st_susfs_sus_path* __user user_info) {
    struct st_susfs_sus_path_list *cursor, *temp;
    struct st_susfs_sus_path_list *new_list = NULL;
	struct st_susfs_sus_path info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_sus_path))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_SUS_PATH, list) {
        if (!strcmp(info.target_pathname, cursor->info.target_pathname)) {
            SUSFS_LOGE("target_pathname: '%s' is already created in LH_SUS_PATH\n", info.target_pathname);
            return 1;
        }
    }

    new_list = kmalloc(sizeof(struct st_susfs_sus_path_list), GFP_KERNEL);
    if (!new_list) {
		SUSFS_LOGE("No enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(struct st_susfs_sus_path));

    INIT_LIST_HEAD(&new_list->list);
    spin_lock(&susfs_spin_lock);
    list_add_tail(&new_list->list, &LH_SUS_PATH);
    spin_unlock(&susfs_spin_lock);
    SUSFS_LOGI("target_pathname: '%s' is successfully added to LH_SUS_PATH\n", info.target_pathname);
    return 0;
}

int susfs_add_sus_mount(struct st_susfs_sus_mount* __user user_info) {
    struct st_susfs_sus_mount_list *cursor, *temp;
    struct st_susfs_sus_mount_list *new_list = NULL;
	struct st_susfs_sus_mount info;
	unsigned long encoded_target_dev;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_sus_mount))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}
#if defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64)
#ifdef CONFIG_MIPS
	encoded_target_dev = new_decode_dev(info.target_dev);
#else
	encoded_target_dev = huge_decode_dev(info.target_dev);
#endif /* CONFIG_MIPS */
#else /* defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64) */
	encoded_target_dev = old_decode_dev(info.target_dev);
#endif /* defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64) */

    list_for_each_entry_safe(cursor, temp, &LH_SUS_MOUNT, list) {
        if (cursor->info.target_dev == encoded_target_dev &&
		    !strcmp(cursor->info.target_pathname, info.target_pathname)) {
		//if (info.target_dev == cursor->info.target_dev) {
            SUSFS_LOGE("target_dev: '0x%x', target_pathname: '%s' is already created in LH_SUS_MOUNT\n", cursor->info.target_dev, cursor->info.target_pathname);
            return 1;
        }
    }

    new_list = kmalloc(sizeof(struct st_susfs_sus_mount_list), GFP_KERNEL);
    if (!new_list) {
		SUSFS_LOGE("No enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(struct st_susfs_sus_mount));
	new_list->info.target_dev = encoded_target_dev;

    INIT_LIST_HEAD(&new_list->list);
    spin_lock(&susfs_spin_lock);
    list_add_tail(&new_list->list, &LH_SUS_MOUNT);
    spin_unlock(&susfs_spin_lock);
    SUSFS_LOGI("target_dev: '0x%x', target_pathname: '%s', is successfully added to LH_SUS_MOUNT\n", new_list->info.target_dev, new_list->info.target_pathname);
    return 0;
}

int susfs_add_sus_kstat(struct st_susfs_sus_kstat* __user user_info) {
    struct st_susfs_sus_kstat_list *cursor, *temp;
    struct st_susfs_sus_kstat_list *new_list = NULL;
	struct st_susfs_sus_kstat info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_sus_kstat))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_KSTAT_SPOOFER, list) {
        if (cursor->info.target_ino == info.target_ino) {
            if (info.target_pathname[0] != '\0') {
                SUSFS_LOGE("target_pathname: '%s' is already created in LH_KSTAT_SPOOFER\n", info.target_pathname);
            } else {
                SUSFS_LOGE("target_ino: '%lu' is already created in LH_KSTAT_SPOOFER\n", info.target_ino);
            }
            return 1;
        }
    }

    new_list = kmalloc(sizeof(struct st_susfs_sus_kstat_list), GFP_KERNEL);
    if (!new_list) {
		SUSFS_LOGE("No enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(struct st_susfs_sus_kstat));
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
	SUSFS_LOGI("target_ino: '%lu', target_pathname: '%s', spoofed_pathname: '%s', spoofed_ino: '%lu', spoofed_dev: '%lu', spoofed_nlink: '%u', spoofed_atime_tv_sec: '%ld', spoofed_mtime_tv_sec: '%ld', spoofed_ctime_tv_sec: '%ld', spoofed_atime_tv_nsec: '%ld', spoofed_mtime_tv_nsec: '%ld', spoofed_ctime_tv_nsec: '%ld', is successfully added to LH_KSTAT_SPOOFER\n",
		new_list->info.target_ino , new_list->info.target_pathname, new_list->info.spoofed_pathname,
		new_list->info.spoofed_ino, new_list->info.spoofed_dev, new_list->info.spoofed_nlink,
		new_list->info.spoofed_atime_tv_sec, new_list->info.spoofed_mtime_tv_sec, new_list->info.spoofed_ctime_tv_sec,
		new_list->info.spoofed_atime_tv_nsec, new_list->info.spoofed_mtime_tv_nsec, new_list->info.spoofed_ctime_tv_nsec);
    return 0;
}

int susfs_update_sus_kstat(struct st_susfs_sus_kstat* __user user_info) {
    struct st_susfs_sus_kstat_list *cursor, *temp;
	struct st_susfs_sus_kstat info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_sus_kstat))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_KSTAT_SPOOFER, list) {
        if (!strcmp(info.target_pathname, cursor->info.target_pathname)) {
            SUSFS_LOGI("updating target_ino from '%lu' to '%lu' for pathname: '%s' in LH_KSTAT_SPOOFER\n", cursor->info.target_ino, info.target_ino, info.target_pathname);
			cursor->info.target_ino = info.target_ino;
            return 0;
        }
    }

	SUSFS_LOGE("target_pathname: '%s' is not found in LH_KSTAT_SPOOFER\n", info.target_pathname);
	return 1;
}

int susfs_add_sus_maps(struct st_susfs_sus_maps* __user user_info) {
    struct st_susfs_sus_maps_list *cursor, *temp;
    struct st_susfs_sus_maps_list *new_list = NULL;
	struct st_susfs_sus_maps info;
	
	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_sus_maps))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

#if defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64)
#ifdef CONFIG_MIPS
	info.target_dev = new_decode_dev(info.target_dev);
#else
	info.target_dev = huge_decode_dev(info.target_dev);
#endif /* CONFIG_MIPS */
#else
	info.target_dev = old_decode_dev(info.target_dev);
#endif /* defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64) */

    list_for_each_entry_safe(cursor, temp, &LH_MAPS_SPOOFER, list) {
		if (cursor->info.is_statically == info.is_statically && !info.is_statically) {
			if (cursor->info.target_ino == info.target_ino) {
				SUSFS_LOGE("is_statically: '%d', target_ino: '%lu', is already created in LH_MAPS_SPOOFER\n",
				info.is_statically, info.target_ino);
				return 1;
			}
		} else if (cursor->info.is_statically == info.is_statically && info.is_statically) {
			if (cursor->info.compare_mode == info.compare_mode && info.compare_mode == 1) {
				if (cursor->info.target_ino == info.target_ino) {
					SUSFS_LOGE("is_statically: '%d', compare_mode: '%d', target_ino: '%lu', is already created in LH_MAPS_SPOOFER\n",
					info.is_statically, info.compare_mode, info.target_ino);
					return 1;
				}
			} else if (cursor->info.compare_mode == info.compare_mode && info.compare_mode == 2) {
				if (cursor->info.target_ino == info.target_ino &&
					cursor->info.is_isolated_entry == info.is_isolated_entry &&
				    cursor->info.target_pgoff == info.target_pgoff &&
					cursor->info.target_prot == info.target_prot) {
					SUSFS_LOGE("is_statically: '%d', compare_mode: '%d', target_ino: '%lu', is_isolated_entry: '%d', target_pgoff: '0x%x', target_prot: '0x%x', is already created in LH_MAPS_SPOOFER\n",
					info.is_statically, info.compare_mode, info.target_ino,
					info.is_isolated_entry, info.target_pgoff, info.target_prot);
					return 1;
				}
			} else if (cursor->info.compare_mode == info.compare_mode && info.compare_mode == 3) {
				if (info.target_ino == 0 &&
					cursor->info.prev_target_ino == info.prev_target_ino &&
				    cursor->info.next_target_ino == info.next_target_ino) {
					SUSFS_LOGE("is_statically: '%d', compare_mode: '%d', target_ino: '%lu', prev_target_ino: '%lu', next_target_ino: '%lu', is already created in LH_MAPS_SPOOFER\n",
					info.is_statically, info.compare_mode, info.target_ino,
					info.prev_target_ino, info.next_target_ino);
					return 1;
				}
			} else if (cursor->info.compare_mode == info.compare_mode && info.compare_mode == 4) {
				if (cursor->info.is_file == info.is_file &&
					cursor->info.target_dev == info.target_dev &&
				    cursor->info.target_pgoff == info.target_pgoff &&
				    cursor->info.target_prot == info.target_prot &&
				    cursor->info.target_addr_size == info.target_addr_size) {
					SUSFS_LOGE("is_statically: '%d', compare_mode: '%d', is_file: '%d', target_dev: '0x%x', target_pgoff: '0x%x', target_prot: '0x%x', target_addr_size: '0x%x', is already created in LH_MAPS_SPOOFER\n",
					info.is_statically, info.compare_mode, info.is_file,
					info.target_dev, info.target_pgoff, info.target_prot,
					info.target_addr_size);
					return 1;
				}
			}
		}
    }

    new_list = kmalloc(sizeof(struct st_susfs_sus_maps_list), GFP_KERNEL);
    if (!new_list) {
		SUSFS_LOGE("No enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(struct st_susfs_sus_maps));
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
    list_add_tail(&new_list->list, &LH_MAPS_SPOOFER);
    spin_unlock(&susfs_spin_lock);

	SUSFS_LOGI("is_statically: '%d', compare_mode: '%d', is_isolated_entry: '%d', is_file: '%d', prev_target_ino: '%lu', next_target_ino: '%lu', target_ino: '%lu', target_dev: '0x%x', target_pgoff: '0x%x', target_prot: '0x%x', target_addr_size: '0x%x', spoofed_pathname: '%s', spoofed_ino: '%lu', spoofed_dev: '0x%x', spoofed_pgoff: '0x%x', spoofed_prot: '0x%x', is successfully added to LH_MAPS_SPOOFER\n",
	new_list->info.is_statically, new_list->info.compare_mode, new_list->info.is_isolated_entry,
	new_list->info.is_file, new_list->info.prev_target_ino, new_list->info.next_target_ino,
	new_list->info.target_ino, new_list->info.target_dev, new_list->info.target_pgoff,
	new_list->info.target_prot, new_list->info.target_addr_size, new_list->info.spoofed_pathname,
	new_list->info.spoofed_ino, new_list->info.spoofed_dev, new_list->info.spoofed_pgoff,
	new_list->info.spoofed_prot);

    return 0;
}

int susfs_update_sus_maps(struct st_susfs_sus_maps* __user user_info) {
    struct st_susfs_sus_maps_list *cursor, *temp;
	struct st_susfs_sus_maps info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_sus_maps))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_MAPS_SPOOFER, list) {
		if (cursor->info.is_statically == info.is_statically && !info.is_statically) {
			if (!strcmp(info.target_pathname, cursor->info.target_pathname)) {
				SUSFS_LOGI("updating target_ino from '%lu' to '%lu' for pathname: '%s' in LH_MAPS_SPOOFER\n", cursor->info.target_ino, info.target_ino, info.target_pathname);
				cursor->info.target_ino = info.target_ino;
				return 0;
			}
		}
    }

	SUSFS_LOGE("target_pathname: '%s' is not found in LH_MAPS_SPOOFER\n", info.target_pathname);
	return 1;
}

int susfs_add_sus_proc_fd_link(struct st_susfs_sus_proc_fd_link* __user user_info) {
    struct st_susfs_sus_proc_fd_link_list *cursor, *temp;
    struct st_susfs_sus_proc_fd_link_list *new_list = NULL;
	struct st_susfs_sus_proc_fd_link info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_sus_proc_fd_link))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_SUS_PROC_FD_LINK, list) {
        if (!strcmp(info.target_link_name, cursor->info.target_link_name)) {
            SUSFS_LOGE("target_link_name: '%s' is already created in LH_SUS_PROC_FD_LINK\n", info.target_link_name);
            return 1;
        }
    }

    new_list = kmalloc(sizeof(struct st_susfs_sus_proc_fd_link_list), GFP_KERNEL);
    if (!new_list) {
		SUSFS_LOGE("No enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(struct st_susfs_sus_proc_fd_link));

    INIT_LIST_HEAD(&new_list->list);
    spin_lock(&susfs_spin_lock);
    list_add_tail(&new_list->list, &LH_SUS_PROC_FD_LINK);
    spin_unlock(&susfs_spin_lock);
    SUSFS_LOGI("target_link_name: '%s', spoofed_link_name: '%s', is successfully added to LH_SUS_PROC_FD_LINK\n",
				new_list->info.target_link_name, new_list->info.spoofed_link_name);
    return 0;
}

int susfs_add_try_umount(struct st_susfs_try_umount* __user user_info) {
    struct st_susfs_try_umount_list *cursor, *temp;
    struct st_susfs_try_umount_list *new_list = NULL;
	struct st_susfs_try_umount info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_try_umount))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

    list_for_each_entry_safe(cursor, temp, &LH_TRY_UMOUNT_PATH, list) {
        if (!strcmp(info.target_pathname, cursor->info.target_pathname)) {
            SUSFS_LOGE("target_pathname: '%s' is already created in LH_TRY_UMOUNT_PATH\n", info.target_pathname);
            return 1;
        }
    }

    new_list = kmalloc(sizeof(struct st_susfs_try_umount_list), GFP_KERNEL);
    if (!new_list) {
		SUSFS_LOGE("No enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(struct st_susfs_try_umount));

    INIT_LIST_HEAD(&new_list->list);
    spin_lock(&susfs_spin_lock);
    list_add_tail(&new_list->list, &LH_TRY_UMOUNT_PATH);
    spin_unlock(&susfs_spin_lock);
    SUSFS_LOGI("target_pathname: '%s', mnt_mode: %d, is successfully added to LH_TRY_UMOUNT_PATH\n", new_list->info.target_pathname, new_list->info.mnt_mode);
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
	SUSFS_LOGI("Setting sysname: '%s', nodename: '%s', release: '%s', version: '%s', machine: '%s'\n",
				my_uname.sysname, my_uname.nodename, my_uname.release, my_uname.version, my_uname.machine);
    spin_unlock(&susfs_spin_lock);
	return 0;
}

int susfs_sus_path_by_path(struct path* file, int* errno_to_be_changed, int syscall_family) {
	size_t size = 4096;
	int res = -1;
	int status = 0;
	char* path = NULL;
	char* ptr = NULL;
	char* end = NULL;
	struct st_susfs_sus_path_list *cursor, *temp;

	if (!uid_matches_suspicious_path() || file == NULL) {
		status = 0;
		goto out;
	}

	path = kmalloc(size, GFP_KERNEL);

	if (path == NULL) {
		status = -1;
		return status;
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

    list_for_each_entry_safe(cursor, temp, &LH_SUS_PATH, list) {
        if (!strcmp(cursor->info.target_pathname, path)) {
            SUSFS_LOGI("hiding target_pathname '%s' for UID %i\n", cursor->info.target_pathname, current_uid().val);
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

int susfs_sus_path_by_filename(struct filename* name, int* errno_to_be_changed, int syscall_family) {
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
		status = susfs_sus_path_by_path(&path, errno_to_be_changed, syscall_family);
		path_put(&path);
	}

	return status;
}

int susfs_sus_ino_for_filldir64(unsigned long ino) {
    struct st_susfs_sus_path_list *cursor, *temp;

	if (!uid_matches_suspicious_path()) return 0;
	list_for_each_entry_safe(cursor, temp, &LH_SUS_PATH, list) {
        if (cursor->info.ino == ino) {
            SUSFS_LOGI("hiding target_pathname '%s' for UID %i\n", cursor->info.target_pathname, current_uid().val);
			return 1;
        }
    }
	return 0;
}

int susfs_sus_mount(struct vfsmount* mnt, struct path* root) {
	size_t size = 4096;
	int res = -1;
	int status = 0;
	char* path = NULL;
	char* ptr = NULL;
	char* end = NULL;
	struct path mnt_path = {
		.dentry = mnt->mnt_root,
		.mnt = mnt
	};
	struct st_susfs_sus_mount_list *cursor, *temp;

	//if (!uid_matches_suspicious_mount()) return status;

	path = kmalloc(size, GFP_KERNEL);

	if (path == NULL) {
		status = -1;
		return status;
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

	list_for_each_entry_safe(cursor, temp, &LH_SUS_MOUNT, list) {
        if (!strcmp(path, cursor->info.target_pathname)) {
			SUSFS_LOGI("target_dev: '%lu', target_pathname '%s' won't be shown to process with UID %i\n",
						cursor->info.target_dev, cursor->info.target_pathname, current_uid().val);
			status = 1;
			goto out;
        }
	}

out:
	kfree(path);
	return status;
}

void susfs_sus_kstat(unsigned long ino, struct stat* out_stat) {
    struct st_susfs_sus_kstat_list *cursor, *temp;

	if (!uid_matches_suspicious_kstat()) return;
	list_for_each_entry_safe(cursor, temp, &LH_KSTAT_SPOOFER, list) {
		if (cursor->info.target_ino == ino) {
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
			out_stat->st_nlink = cursor->info.spoofed_nlink;
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

/* for non statically, it only compare with target_ino, and spoof only the ino, dev to the matched entry
 * for staticially, it compares depending on the mode user chooses
 * compare mode:
 *  1 -> target_ino is 'non-zero', all entries match with target_ino will be spoofed with user defined entry
 *  2 -> target_ino is 'non-zero', all entries match with [target_ino,target_prot,target_pgoff,is_isolated_entry] will be spoofed with user defined entry
 *  3 -> target_ino is 'zero', which is not file, all entries match with [prev_target_ino,next_target_ino] will be spoofed with user defined entry
 *  4 -> target_ino is 'zero' or 'non-zero', all entries match with [is_file,target_addr_size,target_prot,target_pgoff,target_dev] will be spoofed with user defined entry
 */
int susfs_sus_maps(unsigned long target_ino, unsigned long target_address_size, unsigned long* orig_ino, dev_t* orig_dev, vm_flags_t* flags, unsigned long long* pgoff, struct vm_area_struct* vma, char* tmpname) {
    struct st_susfs_sus_maps_list *cursor, *temp;
	struct inode *tmp_inode, *tmp_inode_prev, *tmp_inode_next;

	//if (!uid_matches_suspicious_maps()) return 0;
	list_for_each_entry_safe(cursor, temp, &LH_MAPS_SPOOFER, list) {
		// if it is NOT statically
		if (!cursor->info.is_statically) {
			if (target_ino != 0 && cursor->info.target_ino == target_ino) {
				*orig_ino = cursor->info.spoofed_ino;
				*orig_dev = cursor->info.spoofed_dev;
				SUSFS_LOGI("spoofing maps -> is_statically: '%d', compare_mode: '%d', is_file: '%d', is_isolated_entry: '%d', prev_target_ino: '%lu', next_target_ino: '%lu', target_ino: '%lu', target_dev: '0x%x', target_pgoff: '0x%x', target_prot: '0x%x', target_addr_size: '0x%x', spoofed_pathname: '%s', spoofed_ino: '%lu', spoofed_dev: '0x%x', spoofed_pgoff: '0x%x', spoofed_prot: '0x%x'\n",
				cursor->info.is_statically, cursor->info.compare_mode, cursor->info.is_file,
				cursor->info.is_isolated_entry, cursor->info.prev_target_ino, cursor->info.next_target_ino,
				cursor->info.target_ino, cursor->info.target_dev, cursor->info.target_pgoff,
				cursor->info.target_prot, cursor->info.target_addr_size, cursor->info.spoofed_pathname,
				cursor->info.spoofed_ino, cursor->info.spoofed_dev, cursor->info.spoofed_pgoff,
				cursor->info.spoofed_prot);
				return 1;
			}
		// if it is statically, then compare with compare_mode
		} else if (cursor->info.compare_mode > 0) {
			switch(cursor->info.compare_mode) {
				case 1:
					if (target_ino != 0 && cursor->info.target_ino == target_ino) {
						goto do_spoof;
					}
					break;
				case 2:
					if (target_ino != 0 && cursor->info.target_ino == target_ino &&
						((cursor->info.target_prot & VM_READ) == (*flags & VM_READ)) &&
						((cursor->info.target_prot & VM_WRITE) == (*flags & VM_WRITE)) &&
						((cursor->info.target_prot & VM_EXEC) == (*flags & VM_EXEC)) &&
						((cursor->info.target_prot & VM_MAYSHARE) == (*flags & VM_MAYSHARE)) &&
						cursor->info.target_pgoff == *pgoff) {
						// if is NOT isolated_entry, check for vma->vm_next and vma->vm_prev to see if they have the same inode
						if (!cursor->info.is_isolated_entry) {
							if (vma && vma->vm_next && vma->vm_next->vm_file) {
								tmp_inode = file_inode(vma->vm_next->vm_file);
								if (tmp_inode->i_ino == cursor->info.target_ino ||
								    tmp_inode->i_ino == (cursor->info.target_ino+1) ||
									tmp_inode->i_ino == (cursor->info.target_ino-1)) {
									goto do_spoof;
								}
							}
							if (vma && vma->vm_prev && vma->vm_prev->vm_file) {
								tmp_inode = file_inode(vma->vm_prev->vm_file);
								if (tmp_inode->i_ino == cursor->info.target_ino ||
								    tmp_inode->i_ino == (cursor->info.target_ino+1) ||
									tmp_inode->i_ino == (cursor->info.target_ino-1)) {
									goto do_spoof;
								}
							}
						// if it is isolated_entry
						} else {
							if (vma && vma->vm_next && vma->vm_next->vm_file) {
								tmp_inode = file_inode(vma->vm_next->vm_file);
								if (tmp_inode->i_ino == cursor->info.target_ino) {
									continue;
								}
							}
							if (vma && vma->vm_prev && vma->vm_prev->vm_file) {
								tmp_inode = file_inode(vma->vm_prev->vm_file);
								if (tmp_inode->i_ino == cursor->info.target_ino) {
									continue;
								}
							}
							// both prev and next don't have the same indoe as current entry, we can spoof now
							goto do_spoof;
						}
					}
					break;
				case 3:
					// if current vma is a file, it is not our target
					if (vma->vm_file) continue;
					// compare next target ino only
					if (cursor->info.prev_target_ino == 0 && cursor->info.next_target_ino > 0) {
						if (vma->vm_next && vma->vm_next->vm_file) {
							tmp_inode_next = file_inode(vma->vm_next->vm_file);
							if (tmp_inode_next->i_ino == cursor->info.next_target_ino) {
								goto do_spoof;
							}
						}
					// compare prev target ino only
					} else if (cursor->info.prev_target_ino > 0 && cursor->info.next_target_ino == 0) {
						if (vma->vm_prev && vma->vm_prev->vm_file) {
							tmp_inode_prev = file_inode(vma->vm_prev->vm_file);
							if (tmp_inode_prev->i_ino == cursor->info.prev_target_ino) {
								goto do_spoof;
							}
						}
					// compare both prev ino and next ino
					} else if (cursor->info.prev_target_ino > 0 && cursor->info.next_target_ino > 0) {
						if (vma->vm_prev && vma->vm_prev->vm_file &&
							vma->vm_next && vma->vm_next->vm_file) {
							tmp_inode_prev = file_inode(vma->vm_prev->vm_file);
							tmp_inode_next = file_inode(vma->vm_next->vm_file);
							if (tmp_inode_prev->i_ino == cursor->info.prev_target_ino &&
							    tmp_inode_next->i_ino == cursor->info.next_target_ino) {
								goto do_spoof;
							}
						}
					}
					break;
				case 4:
					if ((cursor->info.is_file && vma->vm_file)||(!cursor->info.is_file && !vma->vm_file)) {
						if (cursor->info.target_dev == *orig_dev &&
							cursor->info.target_pgoff == *pgoff &&
							((cursor->info.target_prot & VM_READ) == (*flags & VM_READ) &&
							 (cursor->info.target_prot & VM_WRITE) == (*flags & VM_WRITE) &&
							 (cursor->info.target_prot & VM_EXEC) == (*flags & VM_EXEC) &&
							 (cursor->info.target_prot & VM_MAYSHARE) == (*flags & VM_MAYSHARE)) &&
							cursor->info.target_addr_size == target_address_size) {
							goto do_spoof;
						}
					}
					break;
				default:
					break;
			}
		}
		continue;
do_spoof:
		if (cursor->info.need_to_spoof_pathname) {
			strncpy(tmpname, cursor->info.spoofed_pathname, SUSFS_MAX_LEN_PATHNAME);
		}
		if (cursor->info.need_to_spoof_ino) {
			*orig_ino = cursor->info.spoofed_ino;
		}
		if (cursor->info.need_to_spoof_dev) {
			*orig_dev = cursor->info.spoofed_dev;
		}
		if (cursor->info.need_to_spoof_prot) {
			if (cursor->info.spoofed_prot & VM_READ) *flags |= VM_READ;
			else *flags = ((*flags | VM_READ) ^ VM_READ);
			if (cursor->info.spoofed_prot & VM_WRITE) *flags |= VM_WRITE;
			else *flags = ((*flags | VM_WRITE) ^ VM_WRITE);
			if (cursor->info.spoofed_prot & VM_EXEC) *flags |= VM_EXEC;
			else *flags = ((*flags | VM_EXEC) ^ VM_EXEC);
			if (cursor->info.spoofed_prot & VM_MAYSHARE) *flags |= VM_MAYSHARE;
			else *flags = ((*flags | VM_MAYSHARE) ^ VM_MAYSHARE);
		}
		if (cursor->info.need_to_spoof_pgoff) {
			*pgoff = cursor->info.spoofed_pgoff;
		}
		SUSFS_LOGI("spoofing maps -> is_statically: '%d', compare_mode: '%d', is_file: '%d', is_isolated_entry: '%d', prev_target_ino: '%lu', next_target_ino: '%lu', target_ino: '%lu', target_dev: '0x%x', target_pgoff: '0x%x', target_prot: '0x%x', target_addr_size: '0x%x', spoofed_pathname: '%s', spoofed_ino: '%lu', spoofed_dev: '0x%x', spoofed_pgoff: '0x%x', spoofed_prot: '0x%x'\n",
		cursor->info.is_statically, cursor->info.compare_mode, cursor->info.is_file,
		cursor->info.is_isolated_entry, cursor->info.prev_target_ino, cursor->info.next_target_ino,
		cursor->info.target_ino, cursor->info.target_dev, cursor->info.target_pgoff,
		cursor->info.target_prot, cursor->info.target_addr_size, cursor->info.spoofed_pathname,
		cursor->info.spoofed_ino, cursor->info.spoofed_dev, cursor->info.spoofed_pgoff,
		cursor->info.spoofed_prot);
		return 2;
    }
	return 0;
}

void susfs_sus_proc_fd_link(char *pathname, int len) {
	struct st_susfs_sus_proc_fd_link_list *cursor, *temp;

	if (!uid_matches_suspicious_proc_fd_link()) {
		return;
	}

	list_for_each_entry_safe(cursor, temp, &LH_SUS_PROC_FD_LINK, list) {
        if (!strcmp(pathname, cursor->info.target_link_name)) {
			if (strlen(cursor->info.spoofed_link_name) >= len) {
				SUSFS_LOGE("[uid:%u] Cannot spoof fd link: '%s' -> '%s', as spoofed_link_name size is bigger than %d\n", current_uid().val, pathname, cursor->info.spoofed_link_name, len);
				return;
			}
            SUSFS_LOGI("[uid:%u] spoofing fd link: '%s' -> '%s'\n", current_uid().val, pathname, cursor->info.spoofed_link_name);
			strcpy(pathname, cursor->info.spoofed_link_name);
			return;
        }
    }
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
		SUSFS_LOGI("umounting '%s' for uid: %d\n", cursor->info.target_pathname, target_uid);
		if (cursor->info.mnt_mode == 0) {
        	try_umount(cursor->info.target_pathname, false, 0);
		} else if (cursor->info.mnt_mode == 1) {
        	try_umount(cursor->info.target_pathname, false, MNT_DETACH);
		}
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
}

void susfs_set_log(bool enabled) {
	spin_lock(&susfs_spin_lock);
	is_log_enable = enabled;
	spin_unlock(&susfs_spin_lock);
	if (is_log_enable) {
		pr_info("susfs: enable logging to kernel");
	} else {
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
	} else if (!strncmp(pathname, "/data/", 6)) {
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
				*errno_to_be_changed = -ENOENT;
				return;
			case SYSCALL_FAMILY_RENAMEAT2_OLDNAME:
				*errno_to_be_changed = -ENOENT;
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
