#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/prctl.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <android/log.h>
#include <ctype.h>
#include <fcntl.h>

/******************
 ** Define Macro **
 ******************/
#define TAG "ksu_susfs"
#define KERNEL_SU_OPTION 0xDEADBEEF

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

#define SUSFS_MAX_LEN_PATHNAME 256
#define SUSFS_MAX_LEN_MFD_NAME 248
#define SUSFS_MAX_LEN_MOUNT_TYPE_NAME 32

#ifndef TIME_HAVE_NANOSEC
#define TIME_HAVE_NANOSEC
#endif

#ifndef __NEW_UTS_LEN
#define __NEW_UTS_LEN 64
#endif

/* VM flags from linux kernel */
#define VM_NONE		0x00000000
#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008
/* mprotect() hardcodes VM_MAYREAD >> 4 == VM_READ, and so for r/w/x bits. */
#define VM_MAYREAD	0x00000010	/* limits for mprotect() etc */
#define VM_MAYWRITE	0x00000020
#define VM_MAYEXEC	0x00000040
#define VM_MAYSHARE	0x00000080

#define log(fmt, msg...) printf(TAG ":" fmt, ##msg);

/*******************
 ** Define Struct **
 *******************/
struct st_susfs_sus_path {
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long           target_ino;
};

struct st_susfs_sus_mount {
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
};

struct st_susfs_sus_kstat {
	unsigned long           target_ino; // the ino after bind mounted or overlayed
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	char                    spoofed_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long           spoofed_ino;
	unsigned long           spoofed_dev;
	unsigned int            spoofed_nlink;
	long                    spoofed_atime_tv_sec;
	long                    spoofed_mtime_tv_sec;
	long                    spoofed_ctime_tv_sec;
#ifdef TIME_HAVE_NANOSEC
	long                    spoofed_atime_tv_nsec;
	long                    spoofed_mtime_tv_nsec;
	long                    spoofed_ctime_tv_nsec;
#endif
};

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

struct st_susfs_try_umount {
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	int                     mnt_mode;
};

struct st_susfs_sus_proc_fd_link {
	char                    target_link_name[SUSFS_MAX_LEN_PATHNAME];
	char                    spoofed_link_name[SUSFS_MAX_LEN_PATHNAME];
};

struct st_susfs_sus_memfd {
	char                    target_pathname[SUSFS_MAX_LEN_MFD_NAME];
};

struct st_susfs_uname {
	char                    sysname[__NEW_UTS_LEN+1];
	char                    nodename[__NEW_UTS_LEN+1];
	char                    release[__NEW_UTS_LEN+1];
	char                    version[__NEW_UTS_LEN+1];
	char                    machine[__NEW_UTS_LEN+1];
};

/**********************
 ** Define Functions **
 **********************/
void pre_check() {
	if (getuid() != 0) {
		log("[-] Must run as root\n");
		exit(1);
	}
}

int isNumeric(char* str) {
	// Check if the string is empty
	if (str[0] == '\0') {
		return 0;
	}

	// Check each character in the string
	for (int i = 0; str[i] != '\0'; i++) {
		// If any character is not a digit, return false
		if (!isdigit(str[i])) {
			return 0;
		}
	}

	// All characters are digits, return true
	return 1;
}

int get_file_stat(char *pathname, struct stat* sb) {
	if (stat(pathname, sb) != 0) {
		return 1;
	}
	return 0;
}

void copy_stat_to_sus_kstat(struct st_susfs_sus_kstat* info, struct stat* sb) {
	info->spoofed_ino = sb->st_ino;
	info->spoofed_dev = sb->st_dev;
	info->spoofed_nlink = sb->st_nlink;
	info->spoofed_atime_tv_sec = sb->st_atime;
	info->spoofed_mtime_tv_sec = sb->st_mtime;
	info->spoofed_ctime_tv_sec = sb->st_ctime;
	info->spoofed_atime_tv_nsec = sb->st_atime_nsec;
	info->spoofed_mtime_tv_nsec = sb->st_mtime_nsec;
	info->spoofed_ctime_tv_nsec = sb->st_ctime_nsec;
}

void copy_stat_to_sus_maps(struct st_susfs_sus_maps* info, struct stat* sb) {
	info->spoofed_ino = sb->st_ino;
	info->spoofed_dev = sb->st_dev;
}

int create_file(const char* filename) {
	FILE* file = fopen(filename, "w+");

	if (file != NULL) {
		printf("File '%s' created successfully.\n", filename);
		fclose(file);
	} else {
		printf("Failed to create file '%s'.\n", filename);
		return 1;
	}
	return 0;
}

static void print_help(void) {
	log(" usage: %s <CMD> [CMD options]\n", TAG);
	log("    <CMD>:\n");
	log("        add_sus_path </path/of/file_or_directory>\n");
	log("         |--> Added path will be hidden from several syscalls\n");
	log("\n");
	log("        add_sus_mount <mounted_path>\n");
	log("         |--> Added mounted path will be hidden from /proc/self/[mounts|mountinfo|mountstats]\n");
	log("\n");
	log("        add_sus_kstat_statically </path/of/file_or_directory> <ino> <dev> <nlink>\\\n");
	log("                                 <atime> <atime_nsec> <mtime> <mtime_nsec> <ctime> <ctime_nsec>\n");
	log("         |--> Add the desired path for spoofing user defined [ino,dev,atime,atime_nsec,mtime,mtime_nsec,ctime,ctime_nsec]\n");
	log("         |--> Use 'stat' tool to find the format of ino -> %%i, dev -> %%d, nlink -> %%h atime -> %%X, mtime -> %%Y, ctime -> %%Z\n");
	log("         |--> e.g., %s add_sus_kstat_statically '/system/addon.d' '1234' '1234' '2'\\\n", TAG);
	log("                       '1712592355' '0' '1712592355' '0' '1712592355' '0' '1712592355' '0'\n");
	log("         |--> Or pass 'default' to use its original value:\n");
	log("         |--> e.g., %s add_sus_kstat_statically '/system/addon.d' 'default' 'default' 'default'\\\n", TAG);
	log("                       '1712592355' 'default' '1712592355' 'default' '1712592355' 'default'\n");
	log("\n");
	log("        add_sus_kstat </path/of/file_or_directory>\n");
	log("         |--> Add the desired path before it gets bind mounted or overlayed, this is used for storing original stat info in kernel memory\n");
	log("         |--> This command must be completed with <update_sus_kstat> later after the added path is bind mounted or overlayed\n");
	log("\n");
	log("        update_sus_kstat </path/of/file_or_directory>\n");
	log("         |--> Add the desired path you have added before via <add_sus_kstat> to complete the kstat spoofing procedure\n");
	log("\n");
	log("        add_sus_maps </path/of/file_or_directory>\n");
	log("         |--> Matched ino in /proc/self/[maps|smaps] will be spoofed for the user defined [ino] and [dev] ONLY!\n");
	log("\n");
	log("        update_sus_maps </path/of/file_or_directory>\n");
	log("         |--> Add the desired path you have added before via <add_sus_maps> to complete the [ino] and [dev] spoofing in maps\n");
	log("\n");
	log("        add_sus_maps_statically <compare_mode> <args_for_mode>\n");
	log("         |--> compare_mode: 1 => target_ino is 'non-zero', all entries match target_ino will be spoofed with user defined entry\n");
	log("               |--> <target_ino>\n");
	log("               |--> <spoofed_pathname>\n");
	log("               |--> <spoofed_ino>\n");
	log("               |--> <spoofed_dev>\n");
	log("               |--> <spoofed_pgoff>\n");
	log("               |--> <spoofed_prot>\n");
	log("         |--> compare_mode: 2 => target_ino is 'non-zero', all entries match [target_ino,target_addr_size,target_pgoff,target_prot,is_isolated_entry] will be spoofed with user defined entry\n");
	log("               |--> <target_ino>: in decimal\n");
	log("               |--> <target_addr_size>: in decimal\n");
	log("               |--> <target_pgoff>: in decimal\n");
	log("               |--> <target_prot>: in string, must be length of 4, and include only characters 'rwxps-', e.g.: 'r--s'\n");
	log("               |--> <spoofed_pathname>: in string, can be passed as 'default' or 'empty'\n");
	log("               |--> <spoofed_ino>: in decimal, can be passed as 'default'\n");
	log("               |--> <spoofed_dev>: in decimal, can be passed as 'default'\n");
	log("               |--> <spoofed_pgoff>: in decimal, can be passed as 'default'\n");
	log("               |--> <spoofed_prot>: in string, must be length of 4, and include only characters 'rwxps-', e.g.: 'r--s', can be passed as 'default'\n");
	log("               |--> <is_isolated_entry>: 0 -> not isolated entry, 1 -> isolated entry\n");
	log("         |--> compare_mode: 3 => target_ino is 'zero', all entries match [prev_target_ino,next_target_ino] will be spoofed with user defined entry\n");
	log("               |--> Note: one of <prev_target_ino> and <next_target_ino> must be > 0, if both are > 0, then both will be compared\n");
	log("               |--> <prev_target_ino>: in decimal, must be >= 0, if 0, then it will not be compared\n");
	log("               |--> <next_target_ino>: in decimal, must be >= 0, if 0, then it will not be compared\n");
	log("               |--> <spoofed_pathname>: in string, can be passed as 'default' or 'empty'\n");
	log("               |--> <spoofed_ino>: in decimal, can be passed as 'default'\n");
	log("               |--> <spoofed_dev>: in decimal, can be passed as 'default'\n");
	log("               |--> <spoofed_pgoff>: in decimal, can be passed as 'default'\n");
	log("               |--> <spoofed_prot>: in string, must be length of 4, and include only characters 'rwxps-', e.g.: 'r--s', can be passed as 'default'\n");
	log("         |--> compare_mode: 4 => all entries match [is_file,target_addr_size,target_prot,target_pgoff,target_dev] will be spoofed with user defined entry\n");
	log("               |--> <is_file>: '0' or '1', 0 -> NOT a file, 0 -> IS a file\n");
	log("               |--> <target_dev>: in decimal, must be >= 0\n");
	log("               |--> <target_pgoff>: in decimal, must be >= 0\n");
	log("               |--> <target_prot>: in string, must be length of 4, and include only characters 'rwxps-', e.g.: 'r--s', can be passed as 'default'\n");
	log("               |--> <target_addr_size>: in decimal, must be > 0\n");
	log("               |--> <spoofed_pathname>: in string, can be passed as 'default' or 'empty'\n");
	log("               |--> <spoofed_ino>: in decimal, can be passed as 'default'\n");
	log("               |--> <spoofed_dev>: in decimal, can be passed as 'default'\n");
	log("               |--> <spoofed_pgoff>: in decimal, can be passed as 'default'\n");
	log("               |--> <spoofed_prot>: in string, must be length of 4, and include only characters 'rwxps-', e.g.: 'r--s', can be passed as 'default'\n");
	log("         |--> 'default' args will be spoofed with the original value\n");
	log("         |--> 'empty' for <spoofed_pathname> will be spoofed with the empty pathname\n");
	log("\n");
	log("        add_try_umount </path/of/file_or_directory> <mode>\n");
	log("         |--> Added path will be umounted from KSU for all UIDs that are NOT su allowed, and profile template configured with umount\n");
	log("         |--> <mode>: 0 -> umount with no flags, 1 -> umount with MNT_DETACH\n");
	log("         |--> NOTE: susfs umount takes precedence of ksu umount\n");
	log("\n");
	log("        add_sus_proc_fd_link </original/symlinked/path/in/proc/fd/xxx> </spoofed/symlinked/path>\n");
	log("         |--> Added symlinked path will be spoofed in /proc/self/fd/[xx] only\n");
	log("         |--> e.g., add_sus_proc_fd_link /dev/binder /dev/null\n");
	log("         |-->       So if /proc/self/fd/10 is a symlink to /dev/binder, then it will be shown as /dev/null instead\n");
	log("\n");
	log("        add_sus_memfd <target_pathname>\n");
	log("         |--> NOTE: This feature will be effective on all process\n");
	log("         |--> NOTE: Remeber to prepend 'memfd:' to <memfd_name>\n");
	log("         |--> e.g., add_sus_memfd 'memfd:/jit-cache'\n");
	log("\n");
	log("        set_uname <sysname> <nodename> <release> <version> <machine>\n");
	log("         |--> Spoof uname for all processes, set string to 'default' to imply the function to use original string\n");
	log("         |--> e.g., set_uname 'default' 'default' '4.9.337-g3291538446b7' 'default' 'default' \n");
	log("\n");
	log("        enable_log <0|1>\n");
	log("         |--> 0: disable susfs log in kernel, 1: enable susfs log in kernel\n");
	log("\n");
}

/*******************
 ** Main Function **
 *******************/
int main(int argc, char *argv[]) {
	int error = 1;

	pre_check();
	// add_sus_path
	if (argc == 3 && !strcmp(argv[1], "add_sus_path")) {
		struct st_susfs_sus_path info;
		struct stat sb;

		if (get_file_stat(argv[2], &sb)) {
			log("%s not found, skip adding its ino\n", info.target_pathname);
			return 1;
		}
		strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME-1);
		info.target_ino = sb.st_ino;
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_SUS_PATH, &info, NULL, &error);
		return error;
	// add_sus_mount
	} else if (argc == 3 && !strcmp(argv[1], "add_sus_mount")) {
		struct st_susfs_sus_mount info;
		struct stat sb;

		strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME-1);
		if (get_file_stat(argv[2], &sb)) {
			log("[-] Failed to get stat from path: '%s'\n", argv[2]);
			return 1;
		}
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_SUS_MOUNT, &info, NULL, &error);
		return error;
	// add_sus_kstat_statically
	} else if (argc == 12 && !strcmp(argv[1], "add_sus_kstat_statically")) {
		struct st_susfs_sus_kstat info;
		struct stat sb;
		char* endptr;
		unsigned long ino, dev, atime_nsec, mtime_nsec, ctime_nsec;
		unsigned int nlink;
		long atime, mtime, ctime;

		if (get_file_stat(argv[2], &sb)) {
			log("[-] Failed to get stat from path: '%s'\n", argv[2]);
			return 1;
		}

		if (strcmp(argv[3], "default")) {
			ino = strtoul(argv[3], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			info.target_ino = sb.st_ino;
			sb.st_ino = ino;
		} else {
			info.target_ino = sb.st_ino;
		}

		if (strcmp(argv[4], "default")) {
			dev = strtoul(argv[4], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			sb.st_dev = dev;
		}
		if (strcmp(argv[5], "default")) {
			nlink = strtoul(argv[5], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			sb.st_nlink = nlink;
		}
		if (strcmp(argv[6], "default")) {
			atime = strtol(argv[6], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			sb.st_atime = atime;
		}
		if (strcmp(argv[7], "default")) {
			atime_nsec = strtoul(argv[7], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			sb.st_atimensec = atime_nsec;
		}
		if (strcmp(argv[8], "default")) {
			mtime = strtol(argv[8], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			sb.st_mtime = mtime;
		}
		if (strcmp(argv[9], "default")) {
			mtime_nsec = strtoul(argv[9], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			sb.st_mtimensec = mtime_nsec;
		}
		if (strcmp(argv[10], "default")) {
			ctime = strtol(argv[10], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			sb.st_ctime = ctime;
		}
		if (strcmp(argv[11], "default")) {
			ctime_nsec = strtoul(argv[11], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			sb.st_ctimensec = ctime_nsec;
		}
		strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME-1);
		copy_stat_to_sus_kstat(&info, &sb);
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_SUS_KSTAT_STATICALLY, &info, NULL, &error);
		return error;
	// add_sus_kstat
	} else if (argc == 3 && !strcmp(argv[1], "add_sus_kstat")) {
		struct st_susfs_sus_kstat info;
		struct stat sb;

		if (get_file_stat(argv[2], &sb)) {
			log("[-] Failed to get stat from path: '%s'\n", argv[2]);
			return 1;
		}
		strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME-1);
		info.target_ino = sb.st_ino;
		copy_stat_to_sus_kstat(&info, &sb);
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_SUS_KSTAT, &info, NULL, &error);
		return error;
	// update_sus_kstat
	} else if (argc == 3 && !strcmp(argv[1], "update_sus_kstat")) {
		struct st_susfs_sus_kstat info;
		struct stat sb;

		if (get_file_stat(argv[2], &sb)) {
			log("[-] Failed to get stat from path: '%s'\n", argv[2]);
			return 1;
		}
		strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME-1);
		info.target_ino = sb.st_ino;
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_UPDATE_SUS_KSTAT, &info, NULL, &error);
		return error;
	// add_sus_maps
	} else if (argc == 3 && !strcmp(argv[1], "add_sus_maps")) {
		struct st_susfs_sus_maps info;
		struct stat sb;

		if (get_file_stat(argv[2], &sb)) {
			log("[-] Failed to get stat from path: '%s'\n", argv[2]);
			return 1;
		}
		memset(&info, 0, sizeof(struct st_susfs_sus_maps));
		info.is_statically = false;
		info.target_ino = sb.st_ino;
		copy_stat_to_sus_maps(&info, &sb);
		strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME-1);
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_SUS_MAPS, &info, NULL, &error);
		return error;
	// update_sus_maps
	} else if (argc == 3 && !strcmp(argv[1], "update_sus_maps")) {
		struct st_susfs_sus_maps info;
		struct stat sb = {0};

		if (get_file_stat(argv[2], &sb)) {
			log("[-] Failed to get stat from path: '%s'\n", argv[2]);
			return 1;
		}
		info.target_ino = sb.st_ino;
		strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME-1);
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_UPDATE_SUS_MAPS, &info, NULL, &error);
		return error;
	// add_sus_maps_statically
	} else if (argc > 3 && !strcmp(argv[1], "add_sus_maps_statically")) {
		struct st_susfs_sus_maps info;
		char* endptr;

		memset(&info, 0, sizeof(struct st_susfs_sus_maps));
		info.is_statically = true;
		info.compare_mode = strtoul(argv[2], &endptr, 10);
		if (*endptr != '\0' || info.compare_mode > 4 || info.compare_mode < 1) {
			log("[-] compare_mode must be [1|2|3|4]\n");
			return 1;
		}
		// compare_mode == 1
		if (info.compare_mode == 1 && argc == 9) {
			// target_ino
			info.target_ino = strtoul(argv[3], &endptr, 10);
			if (*endptr != '\0') {
				log("[-] target_ino must be a digit\n");
				return 1;
			}
			// spoofed_pathname
			if (strcmp(argv[4], "default")) {
				if (strcmp(argv[4], "empty")) {
					strncpy(info.spoofed_pathname, argv[4], SUSFS_MAX_LEN_PATHNAME-1);
				}
				info.need_to_spoof_pathname = true;
			}
			// spoofed_ino
			if (strcmp(argv[5], "default")) {
				info.spoofed_ino = strtoul(argv[5], &endptr, 10);
				if (*endptr != '\0') {
					log("[-] spoofed_ino must be a digit or 'default'\n");
					return 1;
				}
				info.need_to_spoof_ino = true;
			}
			// spoofed_dev
			if (strcmp(argv[6], "default")) {
				info.spoofed_dev = strtoul(argv[6], &endptr, 10);
				if (*endptr != '\0') {
					log("[-] spoofed_dev must be a digit or 'default'\n");
					return 1;
				}
				info.need_to_spoof_dev = true;
			}
			// spoofed_pgoff
			if (strcmp(argv[7], "default")) {
				info.spoofed_pgoff = strtoul(argv[7], &endptr, 10);
				if (*endptr != '\0') {
					log("[-] spoofed_pgoff must be a digit or 'default'\n");
					return 1;
				}
				info.need_to_spoof_pgoff = true;
			}
			// spoofed_prot
			if (strcmp(argv[8], "default")) {
				if (strlen(argv[8]) != 4 ||
					((argv[8][0] != 'r' && argv[8][0] != '-') ||
					(argv[8][1] != 'w' && argv[8][1] != '-') ||
					(argv[8][2] != 'x' && argv[8][2] != '-') ||
					(argv[8][3] != 'p' && argv[8][3] != 's'))) 
				{
					log("[-] spoofed_prot must match length of 'rwxp', and include only 'rwxps-' charaters\n");
					return 1;
				}
				if (argv[8][0] == 'r') info.spoofed_prot |= VM_READ; 
				if (argv[8][1] == 'w') info.spoofed_prot |= VM_WRITE; 
				if (argv[8][2] == 'x') info.spoofed_prot |= VM_EXEC; 
				if (argv[8][3] == 's') info.spoofed_prot |= VM_MAYSHARE; 
				info.need_to_spoof_prot = true;
			}
		// compare_mode == 2
		} else if (info.compare_mode == 2 && argc == 13) {
			// target_ino
			info.target_ino = strtoul(argv[3], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			// target_addr_size
			info.target_addr_size = strtoul(argv[4], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			// target_pgoff
			info.target_pgoff = strtoul(argv[5], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			// target_prot
			if (strlen(argv[6]) != 4 ||
				((argv[6][0] != 'r' && argv[6][0] != '-') ||
				(argv[6][1] != 'w' && argv[6][1] != '-') ||
				(argv[6][2] != 'x' && argv[6][2] != '-') ||
				(argv[6][3] != 'p' && argv[6][3] != 's'))) 
			{
				print_help();
				return 1;
			}
			if (argv[6][0] == 'r') info.target_prot |= VM_READ; 
			if (argv[6][1] == 'w') info.target_prot |= VM_WRITE; 
			if (argv[6][2] == 'x') info.target_prot |= VM_EXEC; 
			if (argv[6][3] == 's') info.target_prot |= VM_MAYSHARE; 
			// spoofed_pathname
			if (strcmp(argv[7], "default")) { 
				if (strcmp(argv[7], "empty")) {
					strncpy(info.spoofed_pathname, argv[7], SUSFS_MAX_LEN_PATHNAME-1);
				}
				info.need_to_spoof_pathname = true;
			}
			// spoofed_ino
			if (strcmp(argv[8], "default")) {
				info.spoofed_ino = strtoul(argv[8], &endptr, 10);
				if (*endptr != '\0') {
					print_help();
					return 1;
				}
				info.need_to_spoof_ino = true;
			}
			// spoofed_dev
			if (strcmp(argv[9], "default")) {
				info.spoofed_dev = strtoul(argv[9], &endptr, 10);
				if (*endptr != '\0') {
					print_help();
					return 1;
				}
				info.need_to_spoof_dev = true;
            }
			// spoofed_pgoff
			if (strcmp(argv[10], "default")) {
				info.spoofed_pgoff = strtoul(argv[10], &endptr, 10);
				if (*endptr != '\0') {
					print_help();
					return 1;
				}
				info.need_to_spoof_pgoff = true;
			}
			// spoofed_prot
			if (strcmp(argv[11], "default")) {
				if (strlen(argv[11]) != 4 ||
					((argv[11][0] != 'r' && argv[11][0] != '-') ||
					(argv[11][1] != 'w' && argv[11][1] != '-') ||
					(argv[11][2] != 'x' && argv[11][2] != '-') ||
					(argv[11][3] != 'p' && argv[11][3] != 's'))) 
				{
					print_help();
					return 1;
				}
				if (argv[11][0] == 'r') info.spoofed_prot |= VM_READ; 
				if (argv[11][1] == 'w') info.spoofed_prot |= VM_WRITE; 
				if (argv[11][2] == 'x') info.spoofed_prot |= VM_EXEC; 
				if (argv[11][3] == 's') info.spoofed_prot |= VM_MAYSHARE; 
				info.need_to_spoof_prot = true;
			}
			// is_isolated_entry
			if (strcmp(argv[12], "0") && strcmp(argv[12], "1")) {
				print_help();
				return 1;
			}
			if (!strcmp(argv[12], "0")) {
				info.is_isolated_entry = false;
			} else {
				info.is_isolated_entry = true;
			}
		// compare_mode == 3
		} else if (info.compare_mode == 3 && argc == 10) {
			// prev_target_ino
			info.prev_target_ino = strtoul(argv[3], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			// next_target_ino
			info.next_target_ino = strtoul(argv[4], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			if (info.prev_target_ino == 0 && info.next_target_ino == 0) {
				log("[-] prev_target_ino and next_target_ino cannot be 0 at the same time, one of them must be > 0\n");
				return 1;
			}
			// spoofed_pathname
			if (strcmp(argv[5], "default")) { 
				if (strcmp(argv[5], "empty")) {
					strncpy(info.spoofed_pathname, argv[5], SUSFS_MAX_LEN_PATHNAME-1);
				}
				info.need_to_spoof_pathname = true;
			}
			// spoofed_ino
			if (strcmp(argv[6], "default")) {
				info.spoofed_ino = strtoul(argv[6], &endptr, 10);
				if (*endptr != '\0') {
					print_help();
					return 1;
				}
				info.need_to_spoof_ino = true;
			}
			// spoofed_dev
			if (strcmp(argv[7], "default")) {
				info.spoofed_dev = strtoul(argv[7], &endptr, 10);
				if (*endptr != '\0') {
					print_help();
					return 1;
				}
				info.need_to_spoof_dev = true;
			}
			// spoofed_pgoff
			if (strcmp(argv[8], "default")) {
				info.spoofed_pgoff = strtoul(argv[8], &endptr, 10);
				if (*endptr != '\0') {
					print_help();
					return 1;
				}
				info.need_to_spoof_pgoff = true;
			}
			// spoofed_prot
			if (strcmp(argv[9], "default")) {
				if (strlen(argv[9]) != 4 ||
					((argv[9][0] != 'r' && argv[9][0] != '-') ||
					(argv[9][1] != 'w' && argv[9][1] != '-') ||
					(argv[9][2] != 'x' && argv[9][2] != '-') ||
					(argv[9][3] != 'p' && argv[9][3] != 's'))) 
				{
					print_help();
					return 1;
				}
				if (argv[9][0] == 'r') info.spoofed_prot |= VM_READ; 
				if (argv[9][1] == 'w') info.spoofed_prot |= VM_WRITE; 
				if (argv[9][2] == 'x') info.spoofed_prot |= VM_EXEC; 
				if (argv[9][3] == 's') info.spoofed_prot |= VM_MAYSHARE; 
				info.need_to_spoof_prot = true;
			}
		} else if (info.compare_mode == 4 && argc == 13) {
			// is_file
			if (strcmp(argv[3], "0") && strcmp(argv[3], "1")) {
				print_help();
				return 1;
			}
			info.is_file = strtoul(argv[3], &endptr, 10);
			// target_dev
			info.target_dev = strtoul(argv[4], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			// target_pgoff
			info.target_pgoff = strtoul(argv[5], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			// target_prot
			if (strlen(argv[6]) != 4 ||
				((argv[6][0] != 'r' && argv[6][0] != '-') ||
				(argv[6][1] != 'w' && argv[6][1] != '-') ||
				(argv[6][2] != 'x' && argv[6][2] != '-') ||
				(argv[6][3] != 'p' && argv[6][3] != 's'))) 
			{
				print_help();
				return 1;
			}
			if (argv[6][0] == 'r') info.target_prot |= VM_READ; 
			if (argv[6][1] == 'w') info.target_prot |= VM_WRITE; 
			if (argv[6][2] == 'x') info.target_prot |= VM_EXEC; 
			if (argv[6][3] == 's') info.target_prot |= VM_MAYSHARE; 
			// target_addr_size
			info.target_addr_size = strtoul(argv[7], &endptr, 10);
			if (*endptr != '\0') {
				print_help();
				return 1;
			}
			// spoofed_pathname
			if (strcmp(argv[8], "default")) { 
				if (strcmp(argv[8], "empty")) {
					strncpy(info.spoofed_pathname, argv[8], SUSFS_MAX_LEN_PATHNAME-1);
				}
				info.need_to_spoof_pathname = true;
			}
			// spoofed_ino
			if (strcmp(argv[9], "default")) {
				info.spoofed_ino = strtoul(argv[9], &endptr, 10);
				if (*endptr != '\0') {
					print_help();
					return 1;
				}
				info.need_to_spoof_ino = true;
			}
			// spoofed_dev
			if (strcmp(argv[10], "default")) {
				info.spoofed_dev = strtoul(argv[10], &endptr, 10);
				if (*endptr != '\0') {
					print_help();
					return 1;
				}
				info.need_to_spoof_dev = true;
			}
			// spoofed_pgoff
			if (strcmp(argv[11], "default")) {
				info.spoofed_pgoff = strtoul(argv[11], &endptr, 10);
				if (*endptr != '\0') {
					print_help();
					return 1;
				}
				info.need_to_spoof_pgoff = true;
			}
			// spoofed_prot
			if (strcmp(argv[12], "default")) {
				if (strlen(argv[12]) != 4 ||
					((argv[12][0] != 'r' && argv[12][0] != '-') ||
					(argv[12][1] != 'w' && argv[12][1] != '-') ||
					(argv[12][2] != 'x' && argv[12][2] != '-') ||
					(argv[12][3] != 'p' && argv[12][3] != 's'))) 
				{
					print_help();
					return 1;
				}
				if (argv[12][0] == 'r') info.spoofed_prot |= VM_READ; 
				if (argv[12][1] == 'w') info.spoofed_prot |= VM_WRITE; 
				if (argv[12][2] == 'x') info.spoofed_prot |= VM_EXEC; 
				if (argv[12][3] == 's') info.spoofed_prot |= VM_MAYSHARE; 
				info.need_to_spoof_prot = true;
			}
		} else {
			print_help();
			return 1;
		}
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_SUS_MAPS_STATICALLY, &info, NULL, &error);
		return error;
	// add_try_umount
	} else if (argc == 4 && !strcmp(argv[1], "add_try_umount")) {
		struct st_susfs_try_umount info;
		char* endptr;
		char abs_path[PATH_MAX], *p_abs_path;
		
		strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME-1);
		p_abs_path = realpath(info.target_pathname, abs_path);
		if (p_abs_path == NULL) {
			perror("realpath");
			return 1;
		}
		if (!strcmp(p_abs_path, "/system") ||
			!strcmp(p_abs_path, "/vendor") ||
			!strcmp(p_abs_path, "/product") ||
			!strcmp(p_abs_path, "/data/adb/modules") ||
			!strcmp(p_abs_path, "/debug_ramdisk") ||
			!strcmp(p_abs_path, "/sbin")) {
			printf("[-] %s cannot be added to try_umount, because it will be umounted by ksu lastly\n", p_abs_path);
			return 1;
		}
		if (strcmp(argv[3], "0") && strcmp(argv[3], "1")) {
			print_help();
			return 1;
		}
		info.mnt_mode = strtol(argv[3], &endptr, 10);
		if (*endptr != '\0') {
			print_help();
			return 1;
		}
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_TRY_UMOUNT, &info, NULL, &error);
		return error;
	// add_sus_proc_fd_link
	} else if (argc == 4 && !strcmp(argv[1], "add_sus_proc_fd_link")) {
		struct st_susfs_sus_proc_fd_link info;
		
		strncpy(info.target_link_name, argv[2], SUSFS_MAX_LEN_PATHNAME-1);
		strncpy(info.spoofed_link_name, argv[3], SUSFS_MAX_LEN_PATHNAME-1);
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_SUS_PROC_FD_LINK, &info, NULL, &error);
		return error;
	// add_sus_memfd
	} else if (argc == 3 && !strcmp(argv[1], "add_sus_memfd")) {
		struct st_susfs_sus_memfd info;
	
		memset(&info, 0, sizeof(struct st_susfs_sus_memfd));
		strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_MFD_NAME-1);
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_SUS_MEMFD, &info, NULL, &error);
		return error;
	// set_uname
	} else if (argc == 7 && !strcmp(argv[1], "set_uname")) {
		struct st_susfs_uname info;
		
		strncpy(info.sysname, argv[2], __NEW_UTS_LEN);
		strncpy(info.nodename, argv[3], __NEW_UTS_LEN);
		strncpy(info.release, argv[4], __NEW_UTS_LEN);
		strncpy(info.version, argv[5], __NEW_UTS_LEN);
		strncpy(info.machine, argv[6], __NEW_UTS_LEN);
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_SET_UNAME, &info, NULL, &error);
		return error;
	// enable_log
	} else if (argc == 3 && !strcmp(argv[1], "enable_log")) {
		if (strcmp(argv[2], "0") && strcmp(argv[2], "1")) {
			print_help();
			return 1;
		}
		prctl(KERNEL_SU_OPTION, CMD_SUSFS_ENABLE_LOG, atoi(argv[2]), NULL, &error);
		return error;
	} else {
		print_help();
	}
out:
	return 0;
}
