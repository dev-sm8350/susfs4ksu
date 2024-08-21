#include <linux/version.h>
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/printk.h>
#include <linux/namei.h>
#include <linux/list.h>
#include <linux/init_task.h>
#include <linux/spinlock.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/fdtable.h>
#include <linux/mnt_namespace.h>
#include <linux/statfs.h>
#include "internal.h"
#include "mount.h"
#include <linux/susfs.h>
#ifdef CONFIG_KSU_SUSFS_SUS_SU
#include <linux/sus_su.h>
#endif

LIST_HEAD(LH_SUS_PATH);
LIST_HEAD(LH_SUS_KSTAT_SPOOFER);
LIST_HEAD(LH_SUS_KSTATFS_SPOOFER);
LIST_HEAD(LH_SUS_MOUNT);
LIST_HEAD(LH_SUS_MAPS_SPOOFER);
LIST_HEAD(LH_SUS_PROC_FD_LINK);
LIST_HEAD(LH_SUS_MEMFD);
LIST_HEAD(LH_TRY_UMOUNT_PATH);

struct st_susfs_uname my_uname;

spinlock_t susfs_spin_lock;
spinlock_t susfs_uname_spin_lock;

bool is_log_enable __read_mostly = true;
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
#define SUSFS_LOGI(fmt, ...) if (is_log_enable) pr_info("susfs:[%u][%u][%s] " fmt, current_uid().val, current->pid, __func__, ##__VA_ARGS__)
#define SUSFS_LOGE(fmt, ...) if (is_log_enable) pr_err("susfs:[%u][%u][%s]" fmt, current_uid().val, current->pid, __func__, ##__VA_ARGS__)
#else
#define SUSFS_LOGI(fmt, ...) 
#define SUSFS_LOGE(fmt, ...) 
#endif

/* sus_path */
static void susfs_sus_path_inode_err_retval(struct inode *inode, char *pathname) {
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
	inode->sus_path_err_retval.other_syscalls = -ENOENT;
	inode->sus_path_err_retval.mknodat = -ENOENT;
	inode->sus_path_err_retval.mkdirat = -ENOENT;
	inode->sus_path_err_retval.rmdir = -ENOENT;
	inode->sus_path_err_retval.unlinkat = -ENOENT;
	inode->sus_path_err_retval.symlinkat_newname = -ENOENT;
	inode->sus_path_err_retval.linkat_oldname = -ENOENT;
	inode->sus_path_err_retval.linkat_newname = -ENOENT;
	inode->sus_path_err_retval.renameat2_oldname = -ENOENT;
	inode->sus_path_err_retval.renameat2_newname = -ENOENT;

	if (!strncmp(pathname, "/system/", 8)||
		!strncmp(pathname, "/vendor/", 8)) {
		inode->sus_path_err_retval.other_syscalls = -ENOENT;
		inode->sus_path_err_retval.mknodat = -EROFS;
		inode->sus_path_err_retval.mkdirat = -EROFS;
		inode->sus_path_err_retval.rmdir = -EROFS;
		inode->sus_path_err_retval.unlinkat = -EROFS;
		inode->sus_path_err_retval.symlinkat_newname = -EROFS;
		inode->sus_path_err_retval.linkat_oldname = -ENOENT;
		inode->sus_path_err_retval.linkat_newname = -ENOENT;
		inode->sus_path_err_retval.renameat2_oldname = -EXDEV;
		inode->sus_path_err_retval.renameat2_newname = -EXDEV;
	} else if (!strncmp(pathname, "/storage/emulated/0/Android/data/", 33)||
			   !strncmp(pathname, "/sdcard/Android/data/", 21)) {
		inode->sus_path_err_retval.other_syscalls = -ENOENT;
		inode->sus_path_err_retval.mknodat = -EACCES;
		inode->sus_path_err_retval.mkdirat = -EACCES;
		inode->sus_path_err_retval.rmdir = -ENOENT;
		inode->sus_path_err_retval.unlinkat = -ENOENT;
		inode->sus_path_err_retval.symlinkat_newname = -EACCES;
		inode->sus_path_err_retval.linkat_oldname = -ENOENT;
		inode->sus_path_err_retval.linkat_newname = -ENOENT;
		inode->sus_path_err_retval.renameat2_oldname = -EXDEV;
		inode->sus_path_err_retval.renameat2_newname = -EXDEV;
	} else if (!strncmp(pathname, "/dev/", 5)) {
		inode->sus_path_err_retval.other_syscalls = -ENOENT;
		inode->sus_path_err_retval.mknodat = -EACCES;
		inode->sus_path_err_retval.mkdirat = -EACCES;
		inode->sus_path_err_retval.rmdir = -ENOENT;
		inode->sus_path_err_retval.unlinkat = -ENOENT;
		inode->sus_path_err_retval.symlinkat_newname = -EACCES;
		inode->sus_path_err_retval.linkat_oldname = -ENOENT;
		inode->sus_path_err_retval.linkat_newname = -ENOENT;
		inode->sus_path_err_retval.renameat2_oldname = -EXDEV;
		inode->sus_path_err_retval.renameat2_newname = -EXDEV;
	} else if (!strncmp(pathname, "/data/", 6)) {
		inode->sus_path_err_retval.other_syscalls = -ENOENT;
		inode->sus_path_err_retval.mknodat = -EACCES;
		inode->sus_path_err_retval.mkdirat = -EACCES;
		inode->sus_path_err_retval.rmdir = -ENOENT;
		inode->sus_path_err_retval.unlinkat = -ENOENT;
		inode->sus_path_err_retval.symlinkat_newname = -EACCES;
		inode->sus_path_err_retval.linkat_oldname = -ENOENT;
		inode->sus_path_err_retval.linkat_newname = -ENOENT;
		inode->sus_path_err_retval.renameat2_oldname = -ENOENT;
		inode->sus_path_err_retval.renameat2_newname = -ENOENT;
	}
#endif
}

static int susfs_update_sus_path_inode(struct st_susfs_sus_path* info) {
	struct file *file;
	struct inode *inode;
	mm_segment_t old_fs;
	int err = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	file = filp_open(info->target_pathname, O_RDONLY, 0);
	if (IS_ERR(file)) {
		SUSFS_LOGE("Failed opening file '%s'\n", info->target_pathname);
		err = 1;
		goto out_set_fs;
	}
	inode = file->f_inode;
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
	inode->is_sus_path = true;
#endif
	filp_close(file, NULL);

out_set_fs:
	set_fs(old_fs);
	susfs_sus_path_inode_err_retval(inode, info->target_pathname);
	return err;
}

int susfs_add_sus_path(struct st_susfs_sus_path* __user user_info) {
	struct st_susfs_sus_path_list *cursor, *temp;
	struct st_susfs_sus_path_list *new_list = NULL;
	struct st_susfs_sus_path info;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	list_for_each_entry_safe(cursor, temp, &LH_SUS_PATH, list) {
		if (unlikely(!strcmp(cursor->info.target_pathname, info.target_pathname))) {
			spin_lock(&susfs_spin_lock);
			memcpy(&cursor->info, &info, sizeof(info));
			spin_unlock(&susfs_spin_lock);
			if (susfs_update_sus_path_inode(&cursor->info))
				return 1;
			SUSFS_LOGI("target_ino: '%lu', target_pathname: '%s' is successfully updated to LH_SUS_PATH\n",
						cursor->info.target_ino, cursor->info.target_pathname);
			return 0;
		}
	}

	new_list = kmalloc(sizeof(struct st_susfs_sus_path_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(info));

	if (susfs_update_sus_path_inode(&new_list->info)) {
		kfree(new_list);
		return 1;
	}

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_SUS_PATH);
	spin_unlock(&susfs_spin_lock);
	SUSFS_LOGI("target_ino: '%lu', target_pathname: '%s' is successfully added to LH_SUS_PATH\n",
						new_list->info.target_ino, new_list->info.target_pathname);
	return 0;
}

int susfs_sus_ino_for_filldir64(unsigned long ino) {
	struct st_susfs_sus_path_list *cursor, *temp;

	list_for_each_entry_safe(cursor, temp, &LH_SUS_PATH, list) {
		if (cursor->info.target_ino == ino)
			return 1;
	}
	return 0;
}

int susfs_is_sus_path_list_empty(void) {
	return list_empty(&LH_SUS_PATH);
}

/* sus_mount */
int susfs_add_sus_mount(struct st_susfs_sus_mount* __user user_info) {
	struct st_susfs_sus_mount_list *cursor, *temp;
	struct st_susfs_sus_mount_list *new_list = NULL;
	struct st_susfs_sus_mount info;
	int list_count = 0;

	if (copy_from_user(&info, user_info, sizeof(info))) {
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

	list_for_each_entry_safe(cursor, temp, &LH_SUS_MOUNT, list) {
		if (unlikely(!strcmp(cursor->info.target_pathname, info.target_pathname))) {
			spin_lock(&susfs_spin_lock);
			memcpy(&cursor->info, &info, sizeof(info));
			spin_unlock(&susfs_spin_lock);
			SUSFS_LOGI("target_pathname: '%s', target_dev: '%lu', is successfully updated to LH_SUS_MOUNT\n",
						cursor->info.target_pathname, cursor->info.target_dev);
			return 0;
		}
		list_count++;
	}

	if (list_count == SUSFS_MAX_SUS_MNTS) {
		SUSFS_LOGE("LH_SUS_MOUNT has reached the list limit of %d\n", SUSFS_MAX_SUS_MNTS);
		return 1;
	}

	new_list = kmalloc(sizeof(struct st_susfs_sus_mount_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(info));

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_SUS_MOUNT);
	spin_unlock(&susfs_spin_lock);
	SUSFS_LOGI("target_pathname: '%s', target_dev: '%lu', is successfully added to LH_SUS_MOUNT\n",
				new_list->info.target_pathname, new_list->info.target_dev);
	return 0;
}

/* This implementation may not work for newer kernel like 5.4+,
 * looks like it is caused by mount namespace iteration, need some more investigations.
 * Don't enable this feature if it causes bootloop or stucking at boot logo for you,
 * or use the patch version 1.3.8 instead.
 */
void susfs_sus_mount(struct mnt_namespace *ns) {
	struct st_susfs_sus_mount_list *sus_mount_cursor, *sus_mount_temp;
	struct mount *mnt_cursor, *mnt_temp;
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT_MNT_ID_REORDER
	struct mount *first_mnt_entry;
	int first_entry_mnt_id;
	int first_entry_mnt_master_group_id = 1;
	int last_mnt_master_group_id = 0;
#endif

	if (unlikely(!ns))
		return;
	get_mnt_ns(ns);

#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT_MNT_ID_REORDER
	first_mnt_entry = list_first_entry(&ns->list, struct mount, mnt_list);
	first_entry_mnt_id = first_mnt_entry->mnt_id;
#endif
	list_for_each_entry_safe(mnt_cursor, mnt_temp, &ns->list, mnt_list) {
		mntget(&mnt_cursor->mnt);
		list_for_each_entry_safe(sus_mount_cursor, sus_mount_temp, &LH_SUS_MOUNT, list) {
			if (unlikely(mnt_cursor->mnt.mnt_sb->s_dev == sus_mount_cursor->info.target_dev)) {
				//SUSFS_LOGI("hiding '%s' from proc mounts\n",
				//			sus_mount_cursor->info.target_pathname);
				mnt_cursor->is_sus_mount = true;
				goto out_continue;
			}
		}
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT_MNT_ID_REORDER
		mnt_cursor->fake_mnt_id = first_entry_mnt_id++;
		if (mnt_cursor->mnt_master) {
			if (likely(last_mnt_master_group_id != mnt_cursor->mnt_master->mnt_group_id)) {
				mnt_cursor->fake_mnt_master_group_id = first_entry_mnt_master_group_id++;
			} else {
				mnt_cursor->fake_mnt_master_group_id = first_entry_mnt_master_group_id;
			}
			last_mnt_master_group_id = mnt_cursor->fake_mnt_master_group_id;
		}
#endif
out_continue:
		mntput(&mnt_cursor->mnt);
	}
	put_mnt_ns(ns);
}

/* sus_kstat */
static int susfs_update_sus_kstat_inode(struct st_susfs_sus_kstat* info) {
	struct file *file;
	struct inode *inode;
	mm_segment_t old_fs;
	int err = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	file = filp_open(info->target_pathname, O_RDONLY, 0);
	if (IS_ERR(file)) {
		SUSFS_LOGE("Failed opening file '%s'\n", info->target_pathname);
		err = 1;
		goto out_set_fs;
	}
	
	inode = file->f_inode;
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
	inode->is_sus_kstat = true;
	inode->sus_kstat.ino = info->spoofed_ino;
	inode->sus_kstat.dev = info->spoofed_dev;
	inode->sus_kstat.mode = inode->i_mode;
	inode->sus_kstat.nlink = info->spoofed_nlink;
	inode->sus_kstat.uid = inode->i_uid;
	inode->sus_kstat.gid = inode->i_gid;
	inode->sus_kstat.rdev = inode->i_rdev;
	inode->sus_kstat.size = info->spoofed_size;
	inode->sus_kstat.atime.tv_sec = info->spoofed_atime_tv_sec;
	inode->sus_kstat.atime.tv_nsec = info->spoofed_atime_tv_nsec;
	inode->sus_kstat.mtime.tv_sec = info->spoofed_mtime_tv_sec;
	inode->sus_kstat.mtime.tv_nsec = info->spoofed_mtime_tv_nsec;
	inode->sus_kstat.ctime.tv_sec = info->spoofed_ctime_tv_sec;
	inode->sus_kstat.ctime.tv_nsec = info->spoofed_ctime_tv_nsec;
	inode->sus_kstat.blksize = info->spoofed_blksize;
	inode->sus_kstat.blocks = info->spoofed_blocks;
#endif
	filp_close(file, NULL);
out_set_fs:
	set_fs(old_fs);
	return err;
}

int susfs_add_sus_kstat(struct st_susfs_sus_kstat* __user user_info) {
	struct st_susfs_sus_kstat_list *cursor, *temp;
	struct st_susfs_sus_kstat_list *new_list = NULL;
	struct st_susfs_sus_kstat info;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

#if defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64)
#ifdef CONFIG_MIPS
	info.spoofed_dev = new_decode_dev(info.spoofed_dev);
#else
	info.spoofed_dev = huge_decode_dev(info.spoofed_dev);
#endif /* CONFIG_MIPS */
#else
	info.spoofed_dev = old_decode_dev(info.spoofed_dev);
#endif /* defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64) */

	list_for_each_entry_safe(cursor, temp, &LH_SUS_KSTAT_SPOOFER, list) {
		if (unlikely(!strcmp(cursor->info.target_pathname, info.target_pathname))) {
			spin_lock(&susfs_spin_lock);
			memcpy(&cursor->info, &info, sizeof(info));
			spin_unlock(&susfs_spin_lock);
			if (susfs_update_sus_kstat_inode(&cursor->info))
				return 1;
			SUSFS_LOGI("is_statically: '%d', target_ino: '%lu', target_pathname: '%s', spoofed_ino: '%lu', spoofed_dev: '%lu', spoofed_nlink: '%u', spoofed_size: '%u', spoofed_atime_tv_sec: '%ld', spoofed_mtime_tv_sec: '%ld', spoofed_ctime_tv_sec: '%ld', spoofed_atime_tv_nsec: '%ld', spoofed_mtime_tv_nsec: '%ld', spoofed_ctime_tv_nsec: '%ld', spoofed_blksize: '%u', spoofed_blocks: '%lu', is successfully updated to LH_SUS_KSTAT_SPOOFER\n",
				cursor->info.is_statically, cursor->info.target_ino, cursor->info.target_pathname,
				cursor->info.spoofed_ino, cursor->info.spoofed_dev,
				cursor->info.spoofed_nlink, cursor->info.spoofed_size,
				cursor->info.spoofed_atime_tv_sec, cursor->info.spoofed_mtime_tv_sec, cursor->info.spoofed_ctime_tv_sec,
				cursor->info.spoofed_atime_tv_nsec, cursor->info.spoofed_mtime_tv_nsec, cursor->info.spoofed_ctime_tv_nsec,
				cursor->info.spoofed_blksize, cursor->info.spoofed_blocks);
			return 0;
		}
	}

	new_list = kmalloc(sizeof(struct st_susfs_sus_kstat_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(info));

	if (new_list->info.target_pathname[0] != '\0') {
		if (susfs_update_sus_kstat_inode(&new_list->info)) {
			kfree(new_list);
			return 1;
		}
	}

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_SUS_KSTAT_SPOOFER);
	spin_unlock(&susfs_spin_lock);

	SUSFS_LOGI("is_statically: '%d', target_ino: '%lu', target_pathname: '%s', spoofed_ino: '%lu', spoofed_dev: '%lu', spoofed_nlink: '%u', spoofed_size: '%u', spoofed_atime_tv_sec: '%ld', spoofed_mtime_tv_sec: '%ld', spoofed_ctime_tv_sec: '%ld', spoofed_atime_tv_nsec: '%ld', spoofed_mtime_tv_nsec: '%ld', spoofed_ctime_tv_nsec: '%ld', spoofed_blksize: '%u', spoofed_blocks: '%lu', is successfully added to LH_SUS_KSTAT_SPOOFER\n",
		new_list->info.is_statically, new_list->info.target_ino, new_list->info.target_pathname,
		new_list->info.spoofed_ino, new_list->info.spoofed_dev,
		new_list->info.spoofed_nlink, new_list->info.spoofed_size,
		new_list->info.spoofed_atime_tv_sec, new_list->info.spoofed_mtime_tv_sec, new_list->info.spoofed_ctime_tv_sec,
		new_list->info.spoofed_atime_tv_nsec, new_list->info.spoofed_mtime_tv_nsec, new_list->info.spoofed_ctime_tv_nsec,
		new_list->info.spoofed_blksize, new_list->info.spoofed_blocks);
	return 0;
}

int susfs_update_sus_kstat(struct st_susfs_sus_kstat* __user user_info) {
	struct st_susfs_sus_kstat_list *cursor, *temp;
	struct st_susfs_sus_kstat info;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	spin_lock(&susfs_spin_lock);
	list_for_each_entry_safe(cursor, temp, &LH_SUS_KSTAT_SPOOFER, list) {
		if (unlikely(!strcmp(info.target_pathname, cursor->info.target_pathname))) {
			SUSFS_LOGI("updating target_ino from '%lu' to '%lu' for pathname: '%s' in LH_SUS_KSTAT_SPOOFER\n", cursor->info.target_ino, info.target_ino, info.target_pathname);
			cursor->info.target_ino = info.target_ino;
			if (cursor->info.target_pathname[0] != '\0') {
				if (susfs_update_sus_kstat_inode(&cursor->info)) {
					list_del(&cursor->list);
					kfree(cursor);
					spin_unlock(&susfs_spin_lock);
					return 1;
				}
			}
			spin_unlock(&susfs_spin_lock);
			return 0;
		}
	}
	spin_unlock(&susfs_spin_lock);
	SUSFS_LOGE("target_pathname: '%s' is not found in LH_SUS_KSTAT_SPOOFER\n", info.target_pathname);
	return 1;
}

/* sus_kstatfs */
static int susfs_update_kstatfs_inode(struct st_susfs_sus_kstatfs* info) {
	struct file *target_file, *spoofed_file;
	struct inode *target_inode;
	struct kstatfs spoofed_buf;
	mm_segment_t old_fs;
	int err = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	target_file = filp_open(info->target_pathname, O_RDONLY, 0);
	if (IS_ERR(target_file)) {
		SUSFS_LOGE("Failed opening target_file '%s'\n", info->target_pathname);
		err = 1;
		goto out_set_fs;
	}
	
	spoofed_file = filp_open(info->spoofed_pathname, O_RDONLY, 0);
	if (IS_ERR(spoofed_file)) {
		SUSFS_LOGE("Failed opening spoofed_file '%s'\n", info->spoofed_pathname);
		err = 1;
		goto out_close_target_file;
	}

	if (vfs_statfs(&spoofed_file->f_path, &spoofed_buf)) {
		SUSFS_LOGE("Failed opening spoofed_file '%s'\n", info->spoofed_pathname);
		err = 1;
		goto out_close_spoofed_file;
	}

	target_inode = target_file->f_inode;
	target_inode->is_sus_kstatfs = true;
	memcpy(&target_inode->sus_kstatfs, &spoofed_buf, sizeof(spoofed_buf));

out_close_spoofed_file:
	filp_close(spoofed_file, NULL);
out_close_target_file:
	filp_close(target_file, NULL);
out_set_fs:
	set_fs(old_fs);
	return err;
}

int susfs_add_sus_kstatfs(struct st_susfs_sus_kstatfs* __user user_info) {
	struct st_susfs_sus_kstatfs_list *cursor, *temp;
	struct st_susfs_sus_kstatfs_list *new_list = NULL;
	struct st_susfs_sus_kstatfs info;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	list_for_each_entry_safe(cursor, temp, &LH_SUS_KSTATFS_SPOOFER, list) {
		if (unlikely(!strcmp(cursor->info.target_pathname, info.target_pathname))) {
			spin_lock(&susfs_spin_lock);
			memcpy(&cursor->info, &info, sizeof(info));
			spin_unlock(&susfs_spin_lock);
			if (susfs_update_kstatfs_inode(&cursor->info))
				return 1;
			SUSFS_LOGI("target_ino: '%lu', target_pathname: '%s', spoofed_pathname: '%s', is successfully updated to LH_SUS_KSTATFS_SPOOFER\n",
						cursor->info.target_ino, cursor->info.target_pathname, cursor->info.spoofed_pathname);
			return 0;
		}
	}

	new_list = kmalloc(sizeof(struct st_susfs_sus_kstatfs_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(info));

	if (susfs_update_kstatfs_inode(&new_list->info)) {
		kfree(new_list);
		return 1;
	}

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_SUS_KSTATFS_SPOOFER);
	spin_unlock(&susfs_spin_lock);

	SUSFS_LOGI("target_ino: '%lu', target_pathname: '%s', spoofed_pathname: '%s', is successfully added to LH_SUS_KSTATFS_SPOOFER\n",
		new_list->info.target_ino, new_list->info.target_pathname, new_list->info.spoofed_pathname);
	return 0;
}

/* sus_maps */
int susfs_add_sus_maps(struct st_susfs_sus_maps* __user user_info) {
	struct st_susfs_sus_maps_list *cursor, *temp;
	struct st_susfs_sus_maps_list *new_list = NULL;
	struct st_susfs_sus_maps info;
	int list_count = 0;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

#if defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64)
#ifdef CONFIG_MIPS
	info.target_dev = new_decode_dev(info.target_dev);
	info.spoofed_dev = new_decode_dev(info.spoofed_dev);
#else
	info.target_dev = huge_decode_dev(info.target_dev);
	info.spoofed_dev = huge_decode_dev(info.spoofed_dev);
#endif /* CONFIG_MIPS */
#else
	info.target_dev = old_decode_dev(info.target_dev);
	info.spoofed_dev = old_decode_dev(info.spoofed_dev);
#endif /* defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64) */

	list_for_each_entry_safe(cursor, temp, &LH_SUS_MAPS_SPOOFER, list) {
		if (cursor->info.is_statically == info.is_statically && !info.is_statically) {
			if (cursor->info.target_ino == info.target_ino) {
				SUSFS_LOGE("is_statically: '%d', target_ino: '%lu', is already created in LH_SUS_MAPS_SPOOFER\n",
				info.is_statically, info.target_ino);
				return 1;
			}
		} else if (cursor->info.is_statically == info.is_statically && info.is_statically) {
			if (cursor->info.compare_mode == info.compare_mode && info.compare_mode == 1) {
				if (cursor->info.target_ino == info.target_ino) {
					SUSFS_LOGE("is_statically: '%d', compare_mode: '%d', target_ino: '%lu', is already created in LH_SUS_MAPS_SPOOFER\n",
					info.is_statically, info.compare_mode, info.target_ino);
					return 1;
				}
			} else if (cursor->info.compare_mode == info.compare_mode && info.compare_mode == 2) {
				if (cursor->info.target_ino == info.target_ino &&
					cursor->info.is_isolated_entry == info.is_isolated_entry &&
					cursor->info.target_addr_size == info.target_addr_size &&
				    cursor->info.target_pgoff == info.target_pgoff &&
					cursor->info.target_prot == info.target_prot) {
					SUSFS_LOGE("is_statically: '%d', compare_mode: '%d', target_ino: '%lu', is_isolated_entry: '%d', target_pgoff: '0x%x', target_prot: '0x%x', is already created in LH_SUS_MAPS_SPOOFER\n",
					info.is_statically, info.compare_mode, info.target_ino,
					info.is_isolated_entry, info.target_pgoff, info.target_prot);
					return 1;
				}
			} else if (cursor->info.compare_mode == info.compare_mode && info.compare_mode == 3) {
				if (info.target_ino == 0 &&
					cursor->info.prev_target_ino == info.prev_target_ino &&
				    cursor->info.next_target_ino == info.next_target_ino) {
					SUSFS_LOGE("is_statically: '%d', compare_mode: '%d', target_ino: '%lu', prev_target_ino: '%lu', next_target_ino: '%lu', is already created in LH_SUS_MAPS_SPOOFER\n",
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
					SUSFS_LOGE("is_statically: '%d', compare_mode: '%d', is_file: '%d', target_dev: '0x%x', target_pgoff: '0x%x', target_prot: '0x%x', target_addr_size: '0x%x', is already created in LH_SUS_MAPS_SPOOFER\n",
					info.is_statically, info.compare_mode, info.is_file,
					info.target_dev, info.target_pgoff, info.target_prot,
					info.target_addr_size);
					return 1;
				}
			}
		}
		list_count++;
	}

	if (list_count == SUSFS_MAX_SUS_MAPS) {
		SUSFS_LOGE("LH_SUS_MOUNT has reached the list limit of %d\n", SUSFS_MAX_SUS_MAPS);
		return 1;
	}

	new_list = kmalloc(sizeof(struct st_susfs_sus_maps_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(info));

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_SUS_MAPS_SPOOFER);
	spin_unlock(&susfs_spin_lock);

	SUSFS_LOGI("is_statically: '%d', compare_mode: '%d', is_isolated_entry: '%d', is_file: '%d', prev_target_ino: '%lu', next_target_ino: '%lu', target_ino: '%lu', target_dev: '0x%x', target_pgoff: '0x%x', target_prot: '0x%x', target_addr_size: '0x%x', spoofed_pathname: '%s', spoofed_ino: '%lu', spoofed_dev: '0x%x', spoofed_pgoff: '0x%x', spoofed_prot: '0x%x', is successfully added to LH_SUS_MAPS_SPOOFER\n",
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

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	list_for_each_entry_safe(cursor, temp, &LH_SUS_MAPS_SPOOFER, list) {
		if (cursor->info.is_statically == info.is_statically && !info.is_statically) {
			if (unlikely(!strcmp(info.target_pathname, cursor->info.target_pathname))) {
				SUSFS_LOGI("updating target_ino from '%lu' to '%lu' for pathname: '%s' in LH_SUS_MAPS_SPOOFER\n", cursor->info.target_ino, info.target_ino, info.target_pathname);
				cursor->info.target_ino = info.target_ino;
				return 0;
			}
		}
	}

	SUSFS_LOGE("target_pathname: '%s' is not found in LH_SUS_MAPS_SPOOFER\n", info.target_pathname);
	return 1;
}

/* for non statically, it only compare with target_ino, and spoof only the ino, dev to the matched entry
 * for staticially, it compares depending on the mode user chooses
 * compare mode:
 *  1 -> target_ino is 'non-zero', all entries match with target_ino will be spoofed with user defined entry
 *  2 -> target_ino is 'non-zero', all entries match with [target_ino,target_addr_size,target_prot,target_pgoff,is_isolated_entry] will be spoofed with user defined entry
 *  3 -> target_ino is 'zero', which is not file, all entries match with [prev_target_ino,next_target_ino] will be spoofed with user defined entry
 *  4 -> target_ino is 'zero' or 'non-zero', all entries match with [is_file,target_addr_size,target_prot,target_pgoff,target_dev] will be spoofed with user defined entry
 */
int susfs_sus_maps(unsigned long target_ino, unsigned long target_addr_size, unsigned long* orig_ino, dev_t* orig_dev, vm_flags_t* flags, unsigned long long* pgoff, struct vm_area_struct* vma, char* out_name) {
	struct st_susfs_sus_maps_list *cursor, *temp;
	struct inode *tmp_inode, *tmp_inode_prev, *tmp_inode_next;

	list_for_each_entry_safe(cursor, temp, &LH_SUS_MAPS_SPOOFER, list) {
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
						  cursor->info.target_addr_size == target_addr_size &&
						  cursor->info.target_pgoff == *pgoff) {
						// if is NOT isolated_entry, check for vma->vm_next and vma->vm_prev to see if they have the same inode
						if (!cursor->info.is_isolated_entry) {
							if (vma && vma->vm_next) {
								if (vma->vm_next->vm_file) {
									tmp_inode = file_inode(vma->vm_next->vm_file);
									if (tmp_inode->i_ino == cursor->info.target_ino)
										goto do_spoof;
								}
							}
							if (vma && vma->vm_prev) {
								if (vma->vm_prev->vm_file) {
									tmp_inode = file_inode(vma->vm_prev->vm_file);
									if (tmp_inode->i_ino == cursor->info.target_ino)
										goto do_spoof;
								}
							}
							continue;
						// if it is isolated_entry
						} else {
							if (vma && vma->vm_next) {
								if (vma->vm_next->vm_file) {
									tmp_inode = file_inode(vma->vm_next->vm_file);
									if (tmp_inode->i_ino == cursor->info.target_ino) {
										continue;
									}
								}
							}
							if (vma && vma->vm_prev) {
								if (vma->vm_prev->vm_file) {
									tmp_inode = file_inode(vma->vm_prev->vm_file);
									if (tmp_inode->i_ino == cursor->info.target_ino) {
										continue;
									}
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
							  cursor->info.target_addr_size == target_addr_size) {
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
			strncpy(out_name, cursor->info.spoofed_pathname, SUSFS_MAX_LEN_PATHNAME-1);
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

int susfs_is_sus_maps_list_empty(void) {
	return list_empty(&LH_SUS_MAPS_SPOOFER);
}

/* sus_map_files */
/* @ This function only does the following:
 *   1. Spoof the symlink name of a target_ino listed in /proc/self/map_files
 * 
 * @Note
 * - It has limitation as there is no way to check which
 *   vma address it belongs by passing dentry* only, so it just
 *   checks for matched dentry* and its target_ino in sus_maps list,
 *   then spoof the symlink name of the target_ino defined by user.
 * - Also user cannot see the effects in map_files from other root session,
 *   because it uses current->mm to compare the dentry, the only way to test
 *   is to check within its own pid.
 * - So the BEST practise here is:
 *     Do NOT spoof the map entries which share the same name to different name
 *     seperately unless the other spoofed name is empty of which spoofed_ino is 0,
 *     otherwise there will be inconsistent entries between maps and map_files.
 */
void susfs_sus_map_files_readlink(unsigned long target_ino, char* pathname) {
	struct st_susfs_sus_maps_list *cursor, *temp;

	if (!pathname)
		return;

	list_for_each_entry_safe(cursor, temp, &LH_SUS_MAPS_SPOOFER, list) {
		// We are only interested in statically and target_ino > 0
		if (cursor->info.is_statically && cursor->info.compare_mode > 0 &&
			target_ino > 0 && cursor->info.target_ino == target_ino)
		{
			if (cursor->info.need_to_spoof_pathname) {
				SUSFS_LOGI("spoofing symlink name of ino '%lu' to '%s' in map_files\n",
						target_ino, cursor->info.spoofed_pathname);
				// Don't need to check buffer size as 'pathname' is allocated with 'PAGE_SIZE'
				// which is way bigger than SUSFS_MAX_LEN_PATHNAME
				strcpy(pathname, cursor->info.spoofed_pathname);
				return;
			}
		}
	}
	return;
}

/* @ This function mainly does the following:
 *   1. Remove the user write access for spoofed symlink name in /proc/self/map_files
 *      - Normally anon memory shows read-write access while file shows read access only,
 *        so this is used when spoofing anon memory to files
 *   2. Prevent the dentry from being seen in /proc/self/map_files when spoofing a file to anon memory
 *      - In normal situation only addresses of non-anon files will appear in /proc/self/map_files
 * 
 * @Note
 * - anon files are supposed to be not shown in /proc/self/map_files and 
 *   spoofing from memfd name to non-memfd name should not have write
 *   permission on that target dentry
 */
int susfs_sus_map_files_instantiate(struct vm_area_struct* vma) {
	struct inode *inode = file_inode(vma->vm_file);
	unsigned long target_ino = inode->i_ino;
	dev_t target_dev = inode->i_sb->s_dev;
	unsigned long long target_pgoff = ((loff_t)vma->vm_pgoff) << PAGE_SHIFT;
	unsigned long target_addr_size = vma->vm_end - vma->vm_start;
	vm_flags_t target_flags = vma->vm_flags;
	struct st_susfs_sus_maps_list *cursor, *temp;
	struct inode *tmp_inode, *tmp_inode_prev, *tmp_inode_next;

	list_for_each_entry_safe(cursor, temp, &LH_SUS_MAPS_SPOOFER, list) {
		// We are only interested in statically
		if (!cursor->info.is_statically) {
			continue;
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
						((cursor->info.target_prot & VM_READ) == (target_flags & VM_READ)) &&
						((cursor->info.target_prot & VM_WRITE) == (target_flags & VM_WRITE)) &&
						((cursor->info.target_prot & VM_EXEC) == (target_flags & VM_EXEC)) &&
						((cursor->info.target_prot & VM_MAYSHARE) == (target_flags & VM_MAYSHARE)) &&
						  cursor->info.target_addr_size == target_addr_size &&
						  cursor->info.target_pgoff == target_pgoff) {
						// if is NOT isolated_entry, check for vma->vm_next and vma->vm_prev to see if they have the same inode
						if (!cursor->info.is_isolated_entry) {
							if (vma && vma->vm_next) {
								if (vma->vm_next->vm_file) {
									tmp_inode = file_inode(vma->vm_next->vm_file);
									if (tmp_inode->i_ino == cursor->info.target_ino)
										goto do_spoof;
								}
							}
							if (vma && vma->vm_prev) {
								if (vma->vm_prev->vm_file) {
									tmp_inode = file_inode(vma->vm_prev->vm_file);
									if (tmp_inode->i_ino == cursor->info.target_ino)
										goto do_spoof;
								}
							}
							continue;
						// if it is isolated_entry
						} else {
							if (vma && vma->vm_next) {
								if (vma->vm_next->vm_file) {
									tmp_inode = file_inode(vma->vm_next->vm_file);
									if (tmp_inode->i_ino == cursor->info.target_ino) {
										continue;
									}
								}
							}
							if (vma && vma->vm_prev) {
								if (vma->vm_prev->vm_file) {
									tmp_inode = file_inode(vma->vm_prev->vm_file);
									if (tmp_inode->i_ino == cursor->info.target_ino) {
										continue;
									}
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
						if (cursor->info.target_dev == target_dev &&
							cursor->info.target_pgoff == target_pgoff &&
							((cursor->info.target_prot & VM_READ) == (target_flags & VM_READ) &&
							 (cursor->info.target_prot & VM_WRITE) == (target_flags & VM_WRITE) &&
							 (cursor->info.target_prot & VM_EXEC) == (target_flags & VM_EXEC) &&
							 (cursor->info.target_prot & VM_MAYSHARE) == (target_flags & VM_MAYSHARE)) &&
							  cursor->info.target_addr_size == target_addr_size) {
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
		if (!(cursor->info.spoofed_ino == 0 ||
			(MAJOR(cursor->info.spoofed_dev) == 0 &&
			(MINOR(cursor->info.spoofed_dev) == 0 || MINOR(cursor->info.spoofed_dev) == 1))))
		{
			SUSFS_LOGI("remove user write permission of spoofed symlink '%s' in map_files\n", cursor->info.spoofed_pathname);
			return 1;
		} else {
			SUSFS_LOGI("drop dentry of target_ino '%lu' with spoofed_ino '%lu' in map_files\n",
						cursor->info.target_ino, cursor->info.spoofed_ino);
			return 2;
		}
		return 0;
	}
	return 0;
}

/* sus_proc_fd_link */
int susfs_add_sus_proc_fd_link(struct st_susfs_sus_proc_fd_link* __user user_info) {
	struct st_susfs_sus_proc_fd_link_list *cursor, *temp;
	struct st_susfs_sus_proc_fd_link_list *new_list = NULL;
	struct st_susfs_sus_proc_fd_link info;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	list_for_each_entry_safe(cursor, temp, &LH_SUS_PROC_FD_LINK, list) {
		if (unlikely(!strcmp(cursor->info.target_link_name, info.target_link_name))) {
			spin_lock(&susfs_spin_lock);
			memcpy(&cursor->info, &info, sizeof(info));
			spin_unlock(&susfs_spin_lock);
			SUSFS_LOGI("target_link_name: '%s', spoofed_link_name: '%s', is successfully updated to LH_SUS_PROC_FD_LINK\n",
				cursor->info.target_link_name, cursor->info.spoofed_link_name);
			return 0;
		}
	}

	new_list = kmalloc(sizeof(struct st_susfs_sus_proc_fd_link_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(info));

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_SUS_PROC_FD_LINK);
	spin_unlock(&susfs_spin_lock);
	SUSFS_LOGI("target_link_name: '%s', spoofed_link_name: '%s', is successfully added to LH_SUS_PROC_FD_LINK\n",
				new_list->info.target_link_name, new_list->info.spoofed_link_name);
	return 0;
}

int susfs_sus_proc_fd_link(char *pathname, int len) {
	struct st_susfs_sus_proc_fd_link_list *cursor, *temp;

	list_for_each_entry_safe(cursor, temp, &LH_SUS_PROC_FD_LINK, list) {
		if (unlikely(!strcmp(pathname, cursor->info.target_link_name))) {
			SUSFS_LOGI("[uid:%u] spoofing fd link: '%s' -> '%s'\n", current_uid().val, pathname, cursor->info.spoofed_link_name);
			memset(pathname, 0, len);
			strcpy(pathname, cursor->info.spoofed_link_name);
			return 1;
		}
	}
	return 0;
}

int susfs_is_sus_proc_fd_link_list_empty(void) {
	return list_empty(&LH_SUS_PROC_FD_LINK);
}

/* sus_memfd */
int susfs_add_sus_memfd(struct st_susfs_sus_memfd* __user user_info) {
	struct st_susfs_sus_memfd_list *cursor, *temp;
	struct st_susfs_sus_memfd_list *new_list = NULL;
	struct st_susfs_sus_memfd info;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	list_for_each_entry_safe(cursor, temp, &LH_SUS_MEMFD, list) {
		if (unlikely(!strcmp(info.target_pathname, cursor->info.target_pathname))) {
			SUSFS_LOGE("target_pathname: '%s' is already created in LH_SUS_MEMFD\n", info.target_pathname);
			return 1;
		}
	}

	new_list = kmalloc(sizeof(struct st_susfs_sus_memfd_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(info));

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_SUS_MEMFD);
	spin_unlock(&susfs_spin_lock);
	SUSFS_LOGI("target_pathname: '%s', is successfully added to LH_SUS_MEMFD\n",
				new_list->info.target_pathname);
	return 0;
}

int susfs_sus_memfd(char *memfd_name) {
	struct st_susfs_sus_memfd_list *cursor, *temp;

	list_for_each_entry_safe(cursor, temp, &LH_SUS_MEMFD, list) {
		if (unlikely(!strcmp(memfd_name, cursor->info.target_pathname))) {
			SUSFS_LOGI("prevent memfd_name: '%s' from being created\n", memfd_name);
			return 1;
		}
	}
    return 0;
}

/* try_umount */
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

int susfs_add_try_umount(struct st_susfs_try_umount* __user user_info) {
	struct st_susfs_try_umount_list *cursor, *temp;
	struct st_susfs_try_umount_list *new_list = NULL;
	struct st_susfs_try_umount info;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	list_for_each_entry_safe(cursor, temp, &LH_TRY_UMOUNT_PATH, list) {
		if (unlikely(!strcmp(info.target_pathname, cursor->info.target_pathname))) {
			SUSFS_LOGE("target_pathname: '%s' is already created in LH_TRY_UMOUNT_PATH\n", info.target_pathname);
			return 1;
		}
	}

	new_list = kmalloc(sizeof(struct st_susfs_try_umount_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(info));

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_TRY_UMOUNT_PATH);
	spin_unlock(&susfs_spin_lock);
	SUSFS_LOGI("target_pathname: '%s', mnt_mode: %d, is successfully added to LH_TRY_UMOUNT_PATH\n", new_list->info.target_pathname, new_list->info.mnt_mode);
	return 0;
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

/* spoof_uname */
static void susfs_my_uname_init(void) {
	memset(&my_uname, 0, sizeof(my_uname));
}

int susfs_set_uname(struct st_susfs_uname* __user user_info) {
	struct st_susfs_uname info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_uname))) {
		SUSFS_LOGE("failed copying from userspace.\n");
		return 1;
	}

	spin_lock(&susfs_uname_spin_lock);
	if (!strcmp(info.sysname, "default")) {
		strncpy(my_uname.sysname, utsname()->sysname, __NEW_UTS_LEN);
	} else {
		strncpy(my_uname.sysname, info.sysname, __NEW_UTS_LEN);
	}
	if (!strcmp(info.nodename, "default")) {
		strncpy(my_uname.nodename, utsname()->nodename, __NEW_UTS_LEN);
	} else {
		strncpy(my_uname.nodename, info.nodename, __NEW_UTS_LEN);
	}
	if (!strcmp(info.release, "default")) {
		strncpy(my_uname.release, utsname()->release, __NEW_UTS_LEN);
	} else {
		strncpy(my_uname.release, info.release, __NEW_UTS_LEN);
	}
	if (!strcmp(info.version, "default")) {
		strncpy(my_uname.version, utsname()->version, __NEW_UTS_LEN);
	} else {
		strncpy(my_uname.version, info.version, __NEW_UTS_LEN);
	}
	if (!strcmp(info.machine, "default")) {
		strncpy(my_uname.machine, utsname()->machine, __NEW_UTS_LEN);
	} else {
		strncpy(my_uname.machine, info.machine, __NEW_UTS_LEN);
	}
	if (!strcmp(info.domainname, "default")) {
		strncpy(my_uname.domainname, utsname()->domainname, __NEW_UTS_LEN);
	} else {
		strncpy(my_uname.domainname, info.domainname, __NEW_UTS_LEN);
	}
	spin_unlock(&susfs_uname_spin_lock);
	SUSFS_LOGI("setting sysname: '%s', nodename: '%s', release: '%s', version: '%s', machine: '%s', domainname: '%s'\n",
				my_uname.sysname, my_uname.nodename, my_uname.release, my_uname.version, my_uname.machine, my_uname.domainname);
	return 0;
}

int susfs_spoof_uname(struct new_utsname* tmp) {
	if (unlikely(my_uname.sysname[0] == '\0' || spin_is_locked(&susfs_uname_spin_lock)))
		return 1;
	memcpy(tmp, &my_uname, sizeof(my_uname));
	return 0;
}

/* set_log */
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

/* sus_su */
#ifdef CONFIG_KSU_SUSFS_SUS_SU
int susfs_sus_su(struct st_sus_su* __user user_info) {
	struct st_sus_su info;

	if (copy_from_user(&info, user_info, sizeof(struct st_sus_su))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	if (info.enabled) {
		sus_su_fifo_init(&info.maj_dev_num, info.drv_path);
		ksu_susfs_disable_sus_su();
		if (info.maj_dev_num < 0) {
			SUSFS_LOGI("failed to get proper info.maj_dev_num: %d\n", info.maj_dev_num);
			return 1;
		}
		SUSFS_LOGI("core kprobe hooks for ksu are disabled!\n");
		SUSFS_LOGI("sus_su driver '%s' is enabled!\n", info.drv_path);
		copy_to_user(user_info, &info, sizeof(info));
		return 0;
	} else {
		ksu_susfs_enable_sus_su();
		sus_su_fifo_exit(&info.maj_dev_num, info.drv_path);
		if (info.maj_dev_num != -1) {
			SUSFS_LOGI("failed to set proper info.maj_dev_num to '-1'\n");
			return 1;
		}
		SUSFS_LOGI("core kprobe hooks for ksu are enabled! sus_su driver '%s' is disabled!\n", info.drv_path);
		copy_to_user(user_info, &info, sizeof(info));
		return 0;
	}
	return 1;
}
#endif

/* susfs_init */
void __init susfs_init(void) {
	spin_lock_init(&susfs_spin_lock);
	spin_lock_init(&susfs_uname_spin_lock);
	susfs_my_uname_init();
}

/* No module exit is needed becuase it should never be a loadable kernel module */
//void __init susfs_exit(void)


/*
// For debugging use only 
static int susfs_get_cur_fd_counts() {
	struct fdtable *files_table;
    int fd_count = 0;

	files_table = files_fdtable(current->files);
	for (i = 0; i < files_table->max_fds; i++) {
        if (files_table->fd[i] != NULL) {
            fd_count++;
        }
    }
	return fd_count;
}

static int susfs_pre_add_sus_mount(const char *target_pathname) {
	struct st_susfs_sus_mount_list *cursor, *temp;
	struct st_susfs_sus_mount_list *new_list = NULL;
	int list_count = 0;

	list_for_each_entry_safe(cursor, temp, &LH_SUS_MOUNT, list) {
		if (unlikely(!strcmp(cursor->info.target_pathname, target_pathname))) {
			SUSFS_LOGE("target_pathname: '%s' is already created in LH_SUS_MOUNT\n", cursor->info.target_pathname);
			return 1;
		}
		list_count++;
	}

	if (list_count == SUSFS_MAX_SUS_MNTS) {
		SUSFS_LOGE("LH_SUS_MOUNT has reached the list limit of %d\n", SUSFS_MAX_SUS_MNTS);
		return 1;
	}

	new_list = kmalloc(sizeof(struct st_susfs_sus_mount_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	strncpy(new_list->info.target_pathname, target_pathname, SUSFS_MAX_LEN_PATHNAME-1);

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_SUS_MOUNT);
	spin_unlock(&susfs_spin_lock);
	SUSFS_LOGI("target_pathname: '%s', is successfully added to LH_SUS_MOUNT\n", new_list->info.target_pathname);
	return 0;
}

void susfs_d_path(struct vfsmount *vfs_mnt, struct dentry *vfs_dentry) {
	const struct path mnt_path = {
		.mnt = vfs_mnt,
		.dentry = vfs_dentry,
	};
	char *path = NULL;
	char *p_path = NULL;
	char *end = NULL;
	int res = 0;

	mntget(mnt_path.mnt);
	dget(mnt_path.dentry);

	path = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!path) {
		SUSFS_LOGE("no enough memory\n");
		return;
	}

	//path_get(mnt_path);

	p_path = d_path(&mnt_path, path, PAGE_SIZE);
	if (IS_ERR(p_path)) {
		SUSFS_LOGE("d_path() failed\n");
		goto out_free_path;
	}
	end = mangle_path(path, p_path, " \t\n\\");
	if (!end) {
		goto out_free_path;
	}
	res = end - path;
	path[(size_t) res] = '\0';

	SUSFS_LOGI("checking path: '%s'\n", path);

out_free_path:
	kfree(path);
	dput(mnt_path.dentry);
	mntput(mnt_path.mnt);
}
*/