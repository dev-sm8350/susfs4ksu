#include <linux/version.h>
#include <linux/cred.h>
#include <linux/fs.h>
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
#include <linux/statfs.h>
#include <linux/susfs.h>
#ifdef CONFIG_KSU_SUSFS_SUS_SU
#include <linux/sus_su.h>
#endif
#include "pnode.h"

spinlock_t susfs_spin_lock;

extern bool susfs_is_current_ksu_domain(void);

#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
bool is_log_enable __read_mostly = true;
#define SUSFS_LOGI(fmt, ...) if (is_log_enable) pr_info("susfs:[%u][%d][%s] " fmt, current_uid().val, current->pid, __func__, ##__VA_ARGS__)
#define SUSFS_LOGE(fmt, ...) if (is_log_enable) pr_err("susfs:[%u][%d][%s]" fmt, current_uid().val, current->pid, __func__, ##__VA_ARGS__)
#else
#define SUSFS_LOGI(fmt, ...) 
#define SUSFS_LOGE(fmt, ...) 
#endif

#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
extern void try_umount(const char *mnt, bool check_mnt, int flags);
#endif

/* sus_path */
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
DEFINE_HASHTABLE(SUS_PATH_HLIST, 10);
static int susfs_update_sus_path_inode(char *target_pathname) {
	struct path p;
	struct inode *inode = NULL;

	if (kern_path(target_pathname, LOOKUP_FOLLOW, &p)) {
		SUSFS_LOGE("Failed opening file '%s'\n", target_pathname);
		return 1;
	}

	// We don't allow path of which filesystem type is "tmpfs", because its inode->i_ino is starting from 1 again,
	// which will cause wrong comparison in function susfs_sus_ino_for_filldir64()
	if (strcmp(p.mnt->mnt_sb->s_type->name, "tmpfs") == 0) {
		SUSFS_LOGE("target_pathname: '%s' cannot be added since its filesystem is 'tmpfs'\n", target_pathname);
		path_put(&p);
		return 1;
	}

	inode = d_inode(p.dentry);
	if (!inode) {
		SUSFS_LOGE("inode is NULL\n");
		path_put(&p);
		return 1;
	}

	spin_lock(&inode->i_lock);
	inode->i_state |= INODE_STATE_SUS_PATH;
	spin_unlock(&inode->i_lock);

	path_put(&p);
	return 0;
}

int susfs_add_sus_path(struct st_susfs_sus_path* __user user_info) {
	struct st_susfs_sus_path info;
	struct st_susfs_sus_path_hlist *new_entry, *tmp_entry;
	struct hlist_node *tmp_node;
	int bkt;
	bool update_hlist = false;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	spin_lock(&susfs_spin_lock);
	hash_for_each_safe(SUS_PATH_HLIST, bkt, tmp_node, tmp_entry, node) {
	if (!strcmp(tmp_entry->target_pathname, info.target_pathname)) {
			hash_del(&tmp_entry->node);
			kfree(tmp_entry);
			update_hlist = true;
			break;
		}
	}
	spin_unlock(&susfs_spin_lock);

	new_entry = kmalloc(sizeof(struct st_susfs_sus_path_hlist), GFP_KERNEL);
	if (!new_entry) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	new_entry->target_ino = info.target_ino;
	strncpy(new_entry->target_pathname, info.target_pathname, SUSFS_MAX_LEN_PATHNAME-1);
	if (susfs_update_sus_path_inode(new_entry->target_pathname)) {
		kfree(new_entry);
		return 1;
	}
	spin_lock(&susfs_spin_lock);
	hash_add(SUS_PATH_HLIST, &new_entry->node, info.target_ino);
	if (update_hlist) {
		SUSFS_LOGI("target_ino: '%lu', target_pathname: '%s' is successfully updated to SUS_PATH_HLIST\n",
				new_entry->target_ino, new_entry->target_pathname);	
	} else {
		SUSFS_LOGI("target_ino: '%lu', target_pathname: '%s' is successfully added to SUS_PATH_HLIST\n",
				new_entry->target_ino, new_entry->target_pathname);
	}
	spin_unlock(&susfs_spin_lock);
	return 0;
}

int susfs_sus_ino_for_filldir64(unsigned long ino) {
	struct st_susfs_sus_path_hlist *entry;

	hash_for_each_possible(SUS_PATH_HLIST, entry, node, ino) {
		if (entry->target_ino == ino)
			return 1;
	}
	return 0;
}
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_PATH

/* sus_mount */
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
LIST_HEAD(LH_SUS_MOUNT);
static void susfs_update_sus_mount_inode(char *target_pathname) {
	struct path p;
	struct inode *inode = NULL;
	int err = 0;

	err = kern_path(target_pathname, LOOKUP_FOLLOW, &p);
	if (err) {
		SUSFS_LOGE("Failed opening file '%s'\n", target_pathname);
		return;
	}

	inode = d_inode(p.dentry);
	if (!inode) {
		path_put(&p);
		SUSFS_LOGE("inode is NULL\n");
		return;
	}

	spin_lock(&inode->i_lock);
	inode->i_state |= INODE_STATE_SUS_MOUNT;
	spin_unlock(&inode->i_lock);

	path_put(&p);
}

int susfs_add_sus_mount(struct st_susfs_sus_mount* __user user_info) {
	struct st_susfs_sus_mount_list *cursor = NULL, *temp = NULL;
	struct st_susfs_sus_mount_list *new_list = NULL;
	struct st_susfs_sus_mount info;

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
			susfs_update_sus_mount_inode(cursor->info.target_pathname);
			SUSFS_LOGI("target_pathname: '%s', target_dev: '%lu', is successfully updated to LH_SUS_MOUNT\n",
						cursor->info.target_pathname, cursor->info.target_dev);
			spin_unlock(&susfs_spin_lock);
			return 0;
		}
	}

	new_list = kmalloc(sizeof(struct st_susfs_sus_mount_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(info));
	susfs_update_sus_mount_inode(new_list->info.target_pathname);

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_SUS_MOUNT);
	SUSFS_LOGI("target_pathname: '%s', target_dev: '%lu', is successfully added to LH_SUS_MOUNT\n",
				new_list->info.target_pathname, new_list->info.target_dev);
	spin_unlock(&susfs_spin_lock);
	return 0;
}

#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT
int susfs_auto_add_sus_bind_mount(const char *pathname, struct path *path_target) {
	struct inode *inode;
	// Only source mount path starting with '/data/adb/' will be hidden
	if (strncmp(pathname, "/data/adb/", 10)) {
		SUSFS_LOGE("skip setting SUS_MOUNT inode state for source bind mount path '%s'\n", pathname);
		return 1;
	}
	inode = path_target->dentry->d_inode;
	if (!inode) return 1;
	spin_lock(&inode->i_lock);
	inode->i_state |= INODE_STATE_SUS_MOUNT;
	SUSFS_LOGI("set SUS_MOUNT inode state for source bind mount path '%s'\n", pathname);
	spin_unlock(&inode->i_lock);
	return 0;
}
#endif // #ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT

#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
void susfs_auto_add_sus_ksu_default_mount(const char __user *to_pathname) {
	char pathname[SUSFS_MAX_LEN_PATHNAME];
	struct path path;
	struct inode *inode;

	// Here we need to re-retrieve the struct path as we want the new struct path, not the old one
	if (strncpy_from_user(pathname, to_pathname, SUSFS_MAX_LEN_PATHNAME-1) < 0)
		return;
	//SUSFS_LOGI("pathname: '%s'\n", pathname);
	if ((!strncmp(pathname, "/data/adb/modules", 17) ||
		 !strncmp(pathname, "/debug_ramdisk", 14) ||
		 !strncmp(pathname, "/system", 7) ||
		 !strncmp(pathname, "/system_ext", 11) ||
		 !strncmp(pathname, "/vendor", 7) ||
		 !strncmp(pathname, "/product", 8)) &&
		 !kern_path(pathname, LOOKUP_FOLLOW, &path)) {
		goto set_inode_sus_mount;
	}
	return;
set_inode_sus_mount:
	inode = path.dentry->d_inode;
	if (!inode) return;
	spin_lock(&inode->i_lock);
	inode->i_state |= INODE_STATE_SUS_MOUNT;
	SUSFS_LOGI("set SUS_MOUNT inode state for default KSU mount path '%s'\n", pathname);
	spin_unlock(&inode->i_lock);
	path_put(&path);
}
#endif // #ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_MOUNT

/* sus_kstat */
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
DEFINE_HASHTABLE(SUS_KSTAT_HLIST, 10);
static int susfs_update_sus_kstat_inode(char *target_pathname) {
	struct path p;
	struct inode *inode = NULL;
	int err = 0;

	err = kern_path(target_pathname, LOOKUP_FOLLOW, &p);
	if (err) {
		SUSFS_LOGE("Failed opening file '%s'\n", target_pathname);
		return 1;
	}

	// We don't allow path of which filesystem type is "tmpfs", because its inode->i_ino is starting from 1 again,
	// which will cause wrong comparison in function susfs_sus_ino_for_filldir64()
	if (strcmp(p.mnt->mnt_sb->s_type->name, "tmpfs") == 0) {
		SUSFS_LOGE("target_pathname: '%s' cannot be added since its filesystem is 'tmpfs'\n", target_pathname);
		path_put(&p);
		return 1;
	}

	inode = d_inode(p.dentry);
	if (!inode) {
		path_put(&p);
		SUSFS_LOGE("inode is NULL\n");
		return 1;
	}

	spin_lock(&inode->i_lock);
	inode->i_state |= INODE_STATE_SUS_KSTAT;
	spin_unlock(&inode->i_lock);

	path_put(&p);
	return 0;
}

int susfs_add_sus_kstat(struct st_susfs_sus_kstat* __user user_info) {
	struct st_susfs_sus_kstat info;
	struct st_susfs_sus_kstat_hlist *new_entry, *tmp_entry;
	struct hlist_node *tmp_node;
	int bkt;
	bool update_hlist = false;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	if (strlen(info.target_pathname) == 0) {
		SUSFS_LOGE("target_pathname is an empty string\n");
		return 1;
	}

	spin_lock(&susfs_spin_lock);
	hash_for_each_safe(SUS_KSTAT_HLIST, bkt, tmp_node, tmp_entry, node) {
		if (!strcmp(tmp_entry->info.target_pathname, info.target_pathname)) {
			hash_del(&tmp_entry->node);
			kfree(tmp_entry);
			update_hlist = true;
			break;
		}
	}
	spin_unlock(&susfs_spin_lock);

	new_entry = kmalloc(sizeof(struct st_susfs_sus_kstat_hlist), GFP_KERNEL);
	if (!new_entry) {
		SUSFS_LOGE("no enough memory\n");
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

	new_entry->target_ino = info.target_ino;
	memcpy(&new_entry->info, &info, sizeof(info));
	// only if the target is added statically needs to have flag INODE_STATE_SUS_KSTAT set here,
	// otherwise the flag INODE_STATE_SUS_KSTAT should be set in function susfs_update_sus_kstat()
	if (new_entry->info.is_statically && susfs_update_sus_kstat_inode(new_entry->info.target_pathname)) {
		kfree(new_entry);
		return 1;
	}

	spin_lock(&susfs_spin_lock);
	hash_add(SUS_KSTAT_HLIST, &new_entry->node, info.target_ino);
	if (update_hlist) {
		SUSFS_LOGI("is_statically: '%d', target_ino: '%lu', target_pathname: '%s', spoofed_ino: '%lu', spoofed_dev: '%lu', spoofed_nlink: '%u', spoofed_size: '%u', spoofed_atime_tv_sec: '%ld', spoofed_mtime_tv_sec: '%ld', spoofed_ctime_tv_sec: '%ld', spoofed_atime_tv_nsec: '%ld', spoofed_mtime_tv_nsec: '%ld', spoofed_ctime_tv_nsec: '%ld', spoofed_blksize: '%lu', spoofed_blocks: '%llu', is successfully added to SUS_KSTAT_HLIST\n",
				new_entry->info.is_statically, new_entry->info.target_ino, new_entry->info.target_pathname,
				new_entry->info.spoofed_ino, new_entry->info.spoofed_dev,
				new_entry->info.spoofed_nlink, new_entry->info.spoofed_size,
				new_entry->info.spoofed_atime_tv_sec, new_entry->info.spoofed_mtime_tv_sec, new_entry->info.spoofed_ctime_tv_sec,
				new_entry->info.spoofed_atime_tv_nsec, new_entry->info.spoofed_mtime_tv_nsec, new_entry->info.spoofed_ctime_tv_nsec,
				new_entry->info.spoofed_blksize, new_entry->info.spoofed_blocks);
	} else {
		SUSFS_LOGI("is_statically: '%d', target_ino: '%lu', target_pathname: '%s', spoofed_ino: '%lu', spoofed_dev: '%lu', spoofed_nlink: '%u', spoofed_size: '%u', spoofed_atime_tv_sec: '%ld', spoofed_mtime_tv_sec: '%ld', spoofed_ctime_tv_sec: '%ld', spoofed_atime_tv_nsec: '%ld', spoofed_mtime_tv_nsec: '%ld', spoofed_ctime_tv_nsec: '%ld', spoofed_blksize: '%lu', spoofed_blocks: '%llu', is successfully updated to SUS_KSTAT_HLIST\n",
				new_entry->info.is_statically, new_entry->info.target_ino, new_entry->info.target_pathname,
				new_entry->info.spoofed_ino, new_entry->info.spoofed_dev,
				new_entry->info.spoofed_nlink, new_entry->info.spoofed_size,
				new_entry->info.spoofed_atime_tv_sec, new_entry->info.spoofed_mtime_tv_sec, new_entry->info.spoofed_ctime_tv_sec,
				new_entry->info.spoofed_atime_tv_nsec, new_entry->info.spoofed_mtime_tv_nsec, new_entry->info.spoofed_ctime_tv_nsec,
				new_entry->info.spoofed_blksize, new_entry->info.spoofed_blocks);
	}
	spin_unlock(&susfs_spin_lock);
	return 0;
}

int susfs_update_sus_kstat(struct st_susfs_sus_kstat* __user user_info) {
	struct st_susfs_sus_kstat info;
	struct st_susfs_sus_kstat_hlist *new_entry, *tmp_entry;
	struct hlist_node *tmp_node;
	int bkt;
	int err = 0;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	spin_lock(&susfs_spin_lock);
	hash_for_each_safe(SUS_KSTAT_HLIST, bkt, tmp_node, tmp_entry, node) {
		if (!strcmp(tmp_entry->info.target_pathname, info.target_pathname)) {
			if (susfs_update_sus_kstat_inode(tmp_entry->info.target_pathname)) {
				err = 1;
				goto out_spin_unlock;
			}
			new_entry = kmalloc(sizeof(struct st_susfs_sus_kstat_hlist), GFP_KERNEL);
			if (!new_entry) {
				SUSFS_LOGE("no enough memory\n");
				err = 1;
				goto out_spin_unlock;
			}
			memcpy(&new_entry->info, &tmp_entry->info, sizeof(tmp_entry->info));
			SUSFS_LOGI("updating target_ino from '%lu' to '%lu' for pathname: '%s' in SUS_KSTAT_HLIST\n",
							new_entry->info.target_ino, info.target_ino, info.target_pathname);
			new_entry->target_ino = info.target_ino;
			new_entry->info.target_ino = info.target_ino;
			if (info.spoofed_size > 0) {
				SUSFS_LOGI("updating spoofed_size from '%lld' to '%lld' for pathname: '%s' in SUS_KSTAT_HLIST\n",
								new_entry->info.spoofed_size, info.spoofed_size, info.target_pathname);
				new_entry->info.spoofed_size = info.spoofed_size;
			}
			if (info.spoofed_blocks > 0) {
				SUSFS_LOGI("updating spoofed_blocks from '%llu' to '%llu' for pathname: '%s' in SUS_KSTAT_HLIST\n",
								new_entry->info.spoofed_blocks, info.spoofed_blocks, info.target_pathname);
				new_entry->info.spoofed_blocks = info.spoofed_blocks;
			}
			hash_del(&tmp_entry->node);
			kfree(tmp_entry);
			hash_add(SUS_KSTAT_HLIST, &new_entry->node, info.target_ino);
			goto out_spin_unlock;
		}
	}
out_spin_unlock:
	spin_unlock(&susfs_spin_lock);
	return err;
}

void susfs_sus_ino_for_generic_fillattr(unsigned long ino, struct kstat *stat) {
	struct st_susfs_sus_kstat_hlist *entry;

	hash_for_each_possible(SUS_KSTAT_HLIST, entry, node, ino) {
		if (entry->target_ino == ino) {
			stat->dev = entry->info.spoofed_dev;
			stat->ino = entry->info.spoofed_ino;
			stat->nlink = entry->info.spoofed_nlink;
			stat->size = entry->info.spoofed_size;
			stat->atime.tv_sec = entry->info.spoofed_atime_tv_sec;
			stat->atime.tv_nsec = entry->info.spoofed_atime_tv_nsec;
			stat->mtime.tv_sec = entry->info.spoofed_mtime_tv_sec;
			stat->mtime.tv_nsec = entry->info.spoofed_mtime_tv_nsec;
			stat->ctime.tv_sec = entry->info.spoofed_ctime_tv_sec;
			stat->ctime.tv_nsec = entry->info.spoofed_ctime_tv_nsec;
			stat->blocks = entry->info.spoofed_blocks;
			stat->blksize = entry->info.spoofed_blksize;
			return;
		}
	}
}

void susfs_sus_ino_for_show_map_vma(unsigned long ino, dev_t *out_dev, unsigned long *out_ino) {
	struct st_susfs_sus_kstat_hlist *entry;

	hash_for_each_possible(SUS_KSTAT_HLIST, entry, node, ino) {
		if (entry->target_ino == ino) {
			*out_dev = entry->info.spoofed_dev;
			*out_ino = entry->info.spoofed_ino;
			return;
		}
	}
}
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_KSTAT

/* try_umount */
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
LIST_HEAD(LH_TRY_UMOUNT_PATH);
int susfs_add_try_umount(struct st_susfs_try_umount* __user user_info) {
	struct st_susfs_try_umount_list *cursor = NULL, *temp = NULL;
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
	struct st_susfs_try_umount_list *cursor = NULL, *temp = NULL;

	list_for_each_entry_safe(cursor, temp, &LH_TRY_UMOUNT_PATH, list) {
		SUSFS_LOGI("umounting '%s' for uid: %d\n", cursor->info.target_pathname, target_uid);
		if (cursor->info.mnt_mode == TRY_UMOUNT_DEFAULT) {
			try_umount(cursor->info.target_pathname, false, 0);
		} else if (cursor->info.mnt_mode == TRY_UMOUNT_DETACH) {
			try_umount(cursor->info.target_pathname, false, MNT_DETACH);
		} else {
			SUSFS_LOGE("failed umounting '%s' for uid: %d, mnt_mode '%d' not supported\n",
							cursor->info.target_pathname, target_uid, cursor->info.mnt_mode);
		}
	}
}

#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT
void susfs_auto_add_try_umount_for_bind_mount(struct path *path) {
	struct st_susfs_try_umount_list *cursor = NULL, *temp = NULL;
	struct st_susfs_try_umount_list *new_list = NULL;
	char *pathname = NULL, *dpath = NULL;

	pathname = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!pathname) {
		SUSFS_LOGE("no enough memory\n");
		return;
	}

	dpath = d_path(path, pathname, PAGE_SIZE);
	if (!dpath) {
		SUSFS_LOGE("dpath is NULL\n");
		goto out_free_pathname;
	}

	list_for_each_entry_safe(cursor, temp, &LH_TRY_UMOUNT_PATH, list) {
		if (unlikely(!strcmp(dpath, cursor->info.target_pathname))) {
			SUSFS_LOGE("target_pathname: '%s' is already created in LH_TRY_UMOUNT_PATH\n", dpath);
			goto out_free_pathname;
		}
	}

	new_list = kmalloc(sizeof(struct st_susfs_try_umount_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		goto out_free_pathname;
	}

	strncpy(new_list->info.target_pathname, dpath, SUSFS_MAX_LEN_PATHNAME-1);
	new_list->info.mnt_mode = TRY_UMOUNT_DETACH;

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_TRY_UMOUNT_PATH);
	spin_unlock(&susfs_spin_lock);
	SUSFS_LOGI("target_pathname: '%s', mnt_mode: %d, is successfully added to LH_TRY_UMOUNT_PATH\n", new_list->info.target_pathname, new_list->info.mnt_mode);
out_free_pathname:
	kfree(pathname);
}
#endif // #ifdef CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT
#endif // #ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT

/* spoof_uname */
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
spinlock_t susfs_uname_spin_lock;
struct st_susfs_uname my_uname;
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
	spin_unlock(&susfs_uname_spin_lock);
	SUSFS_LOGI("setting spoofed release: '%s', version: '%s'\n",
				my_uname.release, my_uname.version);
	return 0;
}

int susfs_spoof_uname(struct new_utsname* tmp) {
	if (unlikely(my_uname.release[0] == '\0' || spin_is_locked(&susfs_uname_spin_lock)))
		return 1;
	strncpy(tmp->sysname, utsname()->sysname, __NEW_UTS_LEN);
	strncpy(tmp->nodename, utsname()->nodename, __NEW_UTS_LEN);
	strncpy(tmp->release, my_uname.release, __NEW_UTS_LEN);
	strncpy(tmp->version, my_uname.version, __NEW_UTS_LEN);
	strncpy(tmp->machine, utsname()->machine, __NEW_UTS_LEN);
	strncpy(tmp->domainname, utsname()->domainname, __NEW_UTS_LEN);
	return 0;
}
#endif // #ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME

/* set_log */
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
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
#endif // #ifdef CONFIG_KSU_SUSFS_ENABLE_LOG

/* spoof_bootconfig */
#ifdef CONFIG_KSU_SUSFS_SPOOF_BOOTCONFIG
char *fake_boot_config = NULL;
int susfs_set_bootconfig(char* __user user_fake_boot_config) {
	int res;

	if (!fake_boot_config) {
		// 4096 is enough I guess
		fake_boot_config = kmalloc(SUSFS_FAKE_BOOT_CONFIG_SIZE, GFP_KERNEL);
		if (!fake_boot_config) {
			SUSFS_LOGE("no enough memory\n");
			return -ENOMEM;
		}
	}

	spin_lock(&susfs_spin_lock);
	memset(fake_boot_config, 0, SUSFS_FAKE_BOOT_CONFIG_SIZE);
	res = strncpy_from_user(fake_boot_config, user_fake_boot_config, SUSFS_FAKE_BOOT_CONFIG_SIZE-1);
	spin_unlock(&susfs_spin_lock);

	if (res > 0) {
		SUSFS_LOGI("fake_boot_config is set, length of string: %u\n", strlen(fake_boot_config));
		return 0;
	}
	SUSFS_LOGI("failed setting fake_boot_config\n");
	return res;
}

int susfs_spoof_bootconfig(struct seq_file *m) {
	if (fake_boot_config != NULL) {
		seq_puts(m, fake_boot_config);
		return 0;
	}
	return 1;
}
#endif

/* open_redirect */
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
DEFINE_HASHTABLE(OPEN_REDIRECT_HLIST, 10);
static int susfs_update_open_redirect_inode(struct st_susfs_open_redirect_hlist *new_entry) {
	struct path path_target;
	struct inode *inode_target;
	int err = 0;

	err = kern_path(new_entry->target_pathname, LOOKUP_FOLLOW, &path_target);
	if (err) {
		SUSFS_LOGE("Failed opening file '%s'\n", new_entry->target_pathname);
		return err;
	}

	inode_target = d_inode(path_target.dentry);
	if (!inode_target) {
		SUSFS_LOGE("inode_target is NULL\n");
		err = 1;
		goto out_path_put_target;
	}

	spin_lock(&inode_target->i_lock);
	inode_target->i_state |= INODE_STATE_OPEN_REDIRECT;
	spin_unlock(&inode_target->i_lock);

out_path_put_target:
	path_put(&path_target);
	return err;
}

int susfs_add_open_redirect(struct st_susfs_open_redirect* __user user_info) {
	struct st_susfs_open_redirect info;
	struct st_susfs_open_redirect_hlist *new_entry, *tmp_entry;
	struct hlist_node *tmp_node;
	int bkt;
	bool update_hlist = false;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	spin_lock(&susfs_spin_lock);
	hash_for_each_safe(OPEN_REDIRECT_HLIST, bkt, tmp_node, tmp_entry, node) {
		if (!strcmp(tmp_entry->target_pathname, info.target_pathname)) {
			hash_del(&tmp_entry->node);
			kfree(tmp_entry);
			update_hlist = true;
			break;
		}
	}
	spin_unlock(&susfs_spin_lock);

	new_entry = kmalloc(sizeof(struct st_susfs_open_redirect_hlist), GFP_KERNEL);
	if (!new_entry) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	new_entry->target_ino = info.target_ino;
	strncpy(new_entry->target_pathname, info.target_pathname, SUSFS_MAX_LEN_PATHNAME-1);
	strncpy(new_entry->redirected_pathname, info.redirected_pathname, SUSFS_MAX_LEN_PATHNAME-1);
	if (susfs_update_open_redirect_inode(new_entry)) {
		SUSFS_LOGE("failed adding path '%s' to OPEN_REDIRECT_HLIST\n", new_entry->target_pathname);
		kfree(new_entry);
		return 1;
	}

	spin_lock(&susfs_spin_lock);
	hash_add(OPEN_REDIRECT_HLIST, &new_entry->node, info.target_ino);
	if (update_hlist) {
		SUSFS_LOGI("target_ino: '%lu', target_pathname: '%s', redirected_pathname: '%s', is successfully updated to OPEN_REDIRECT_HLIST\n",
				new_entry->target_ino, new_entry->target_pathname, new_entry->redirected_pathname);	
	} else {
		SUSFS_LOGI("target_ino: '%lu', target_pathname: '%s' redirected_pathname: '%s', is successfully added to OPEN_REDIRECT_HLIST\n",
				new_entry->target_ino, new_entry->target_pathname, new_entry->redirected_pathname);
	}
	spin_unlock(&susfs_spin_lock);
	return 0;
}

struct filename* susfs_get_redirected_path(unsigned long ino) {
	struct st_susfs_open_redirect_hlist *entry;

	hash_for_each_possible(OPEN_REDIRECT_HLIST, entry, node, ino) {
		if (entry->target_ino == ino) {
			SUSFS_LOGI("Redirect for ino: %lu\n", ino);
			return getname_kernel(entry->redirected_pathname);
		}
	}
	return ERR_PTR(-ENOENT);
}
#endif // #ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT

/* sus_su */
#ifdef CONFIG_KSU_SUSFS_SUS_SU
bool susfs_is_sus_su_hooks_enabled __read_mostly = false;
extern void ksu_susfs_enable_sus_su(void);
extern void ksu_susfs_disable_sus_su(void);
int susfs_sus_su(struct st_sus_su* __user user_info) {
	struct st_sus_su info;

	if (copy_from_user(&info, user_info, sizeof(struct st_sus_su))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	if (info.mode == SUS_SU_WITH_OVERLAY) {
		sus_su_fifo_init(&info.maj_dev_num, info.drv_path);
		ksu_susfs_enable_sus_su();
		if (info.maj_dev_num < 0) {
			SUSFS_LOGI("failed to get proper info.maj_dev_num: %d\n", info.maj_dev_num);
			return 1;
		}
		SUSFS_LOGI("core kprobe hooks for ksu are disabled!\n");
		SUSFS_LOGI("sus_su driver '%s' is enabled!\n", info.drv_path);
		if (copy_to_user(user_info, &info, sizeof(info)))
			SUSFS_LOGE("copy_to_user() failed\n");
		return 0;
	} else if (info.mode == SUS_SU_WITH_HOOKS) {
		ksu_susfs_enable_sus_su();
		susfs_is_sus_su_hooks_enabled = true;
		SUSFS_LOGI("core kprobe hooks for ksu are disabled!\n");
		SUSFS_LOGI("non-kprobe hook sus_su is enabled!\n");
		SUSFS_LOGI("sus_su mode: SUS_SU_WITH_HOOKS\n");
		return 0;
	} else if (info.mode == 0) {
		susfs_is_sus_su_hooks_enabled = false;
		ksu_susfs_disable_sus_su();
		sus_su_fifo_exit(&info.maj_dev_num, info.drv_path);
		if (info.maj_dev_num != -1) {
			SUSFS_LOGI("failed to set proper info.maj_dev_num to '-1'\n");
			return 1;
		}
		SUSFS_LOGI("core kprobe hooks for ksu are enabled! sus_su driver '%s' is disabled!\n", info.drv_path);
		if (copy_to_user(user_info, &info, sizeof(info)))
			SUSFS_LOGE("copy_to_user() failed\n");
		return 0;
	}
	return 1;
}
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_SU

/* susfs_init */
void susfs_init(void) {
	spin_lock_init(&susfs_spin_lock);
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
	spin_lock_init(&susfs_uname_spin_lock);
	susfs_my_uname_init();
#endif
	SUSFS_LOGI("susfs is initialized!\n");
}

/* No module exit is needed becuase it should never be a loadable kernel module */
//void __init susfs_exit(void)
