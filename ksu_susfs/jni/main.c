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
#define CMD_SUSFS_ADD_SUSPICIOUS_PATH 0x55555
#define CMD_SUSFS_ADD_MOUNT_TYPE 0x55556
#define CMD_SUSFS_ADD_MOUNT_PATH 0x55557
#define CMD_SUSFS_ADD_SUSPICIOUS_KSTAT 0x55558
#define CMD_SUSFS_UPDATE_SUSPICIOUS_KSTAT 0x55559
#define CMD_SUSFS_ADD_TRY_UMOUNT 0x5555a
#define CMD_SUSFS_ADD_UNAME 0x5555b
#define CMD_SUSFS_ADD_SUSPICIOUS_KSTAT_STATICALLY 0x5555c

#define SUSFS_MAX_LEN_PATHNAME 128
#define SUSFS_MAX_LEN_MOUNT_TYPE_NAME 32

#define HIDE_IN_MAPS_SPOOF_STAT_ONLY 0
#define HIDE_IN_MAPS_HIDE_ENTRYS 1

#ifndef TIME_HAVE_NANOSEC
#define TIME_HAVE_NANOSEC
#endif

#ifndef __NEW_UTS_LEN
#define __NEW_UTS_LEN 64
#endif

#define log(fmt, msg...) printf(TAG ":" fmt, ##msg);

/*******************
 ** Define Struct **
 *******************/
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
    dev_t                  spoofed_dev;
    long                   spoofed_atime_tv_sec;
    long                   spoofed_mtime_tv_sec;
    long                   spoofed_ctime_tv_sec;
#ifdef TIME_HAVE_NANOSEC
    long                   spoofed_atime_tv_nsec;
    long                   spoofed_mtime_tv_nsec;
    long                   spoofed_ctime_tv_nsec;
#endif
};

struct st_susfs_try_umount {
    char                   name[SUSFS_MAX_LEN_PATHNAME];
    //bool                 check_mnt;
    //int                  flags;
};

struct st_susfs_uname {
    char                   sysname[__NEW_UTS_LEN+1];
    char                   nodename[__NEW_UTS_LEN+1];
    char                   release[__NEW_UTS_LEN+1];
    char                   version[__NEW_UTS_LEN+1];
    char                   machine[__NEW_UTS_LEN+1];
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

void copy_stat_to_suspicious_kstat(struct st_susfs_suspicious_kstat* info, struct stat* sb) {
    info->spoofed_ino = sb->st_ino;
    info->spoofed_dev = sb->st_dev;
    info->spoofed_atime_tv_sec = sb->st_atime;
    info->spoofed_mtime_tv_sec = sb->st_mtime;
    info->spoofed_ctime_tv_sec = sb->st_ctime;
#ifdef TIME_HAVE_NANOSEC
    info->spoofed_atime_tv_nsec = sb->st_atime_nsec;
    info->spoofed_mtime_tv_nsec = sb->st_mtime_nsec;
    info->spoofed_ctime_tv_nsec = sb->st_ctime_nsec;
#endif
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
    log("        add_suspicious_path </path/of/file_or_directory>\n");
    log("         |--> Added path will be hidden from different syscalls\n");
    log("        add_mount_type <mount_type_name>\n");
    log("         |--> Added mount type will be hidden from /proc/self/[mounts|mountinfo|mountstats]\n");
    log("        add_mount_path </path/of/file_or_directory>\n");
    log("         |--> Added mounted path will be hidden from /proc/self/[mounts|mountinfo|mountstats]\n");
    log("        add_suspicious_kstat_statically </path/of/file_or_directory> <hide_in_maps_or_not> <ino> <dev> \\\n");
    log("                                      <atime> <atime_nsec> <mtime> <mtime_nsec> <ctime> <ctime_nsec>\n");
    log("         |--> Add the desired path for spoofing a custom ino, dev, atime, atime_nsec, mtime, mtime_nsec, ctime, ctime_nsec\n");
    log("         |--> Matched ino can also be hidden in /proc/self/[maps|smaps] by setting <hide_in_maps_or_not> to 1\n");
    log("         |--> Use 'stat' tool to find the format of ino -> %%i, dev -> %%d, atime -> %%X, mtime -> %%Y, ctime -> %%Z\n");
    log("         |--> e.g., %s add_suspicious_kstat_statically '/system/addon.d' '1234' '1234' \\\n", TAG);
    log("                       '1712592355' '0' '1712592355' '0' '1712592355' '0' '1712592355' '0'\n");
    log("         |--> Or pass 'default' to use its original value:\n");
    log("         |--> e.g., %s add_suspicious_kstat_statically '/system/addon.d' 'default' 'default' \\\n", TAG);
    log("                       '1712592355' 'default' '1712592355' 'default' '1712592355' 'default'\n");
    log("        add_suspicious_kstat </path/of/file_or_directory> <hide_in_maps_or_not>\n");
    log("         |--> Add the desired path before it gets bind mounted or overlayed, this is used for storing original stat info in kernel memory\n");
    log("         |--> Matched ino can also be hidden in /proc/self/[maps|smaps] by setting <hide_in_maps_or_not> to 1\n");
    log("         |--> This command must be completed with <update_suspicious_kstat> later after the added path is bind mounted or overlayed\n");
    log("        update_suspicious_kstat </path/of/file_or_directory>\n");
    log("         |--> Add the desired path you have added before via <add_suspicious_kstat> to complete the kstat spoofing procedure\n");
    log("        add_try_umount </path/of/file_or_directory>\n");
    log("         |--> Added path will be umount from kernel for all UIDs that are NOT su allowed, and profile template configured with umount\n");
    log("        add_uname <sysname> <nodename> <release> <version> <machine>\n");
    log("         |--> Spoof uname for all processes, set string to 'default' imply the function to use original string\n");
    log("         |--> e.g., add_uname 'default' 'default' '4.9.337-g3291538446b7' 'default' 'default' \n");
    log("\n");
    log("    [CMD options]:\n");
    log("        mount_type_name: [overlay|you_name_it_as_I_dont_know]\n");
    log("        hide_in_maps_or_not: [0|1] (using this option only may not help you to bypass dections)\n");
    log("                              0 -> no hide, spoof the ino and dev only\n");
    log("                              1 -> hide the whole entry in maps\n");
}

/*******************
 ** Main Function **
 *******************/
int main(int argc, char *argv[]) {
    int error;

    pre_check();

    if (argc == 3 && !strcmp(argv[1], "add_suspicious_path")) {
        struct st_susfs_suspicious_path info;
        struct stat sb;
        strncpy(info.name, argv[2], SUSFS_MAX_LEN_PATHNAME);
        if (!get_file_stat(argv[2], &sb)) {
            log("%s not found, skip adding its ino\n", info.name);
            info.ino = sb.st_ino;
        } else {
            info.ino = 0;
        }
        prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_SUSPICIOUS_PATH, &info, NULL, &error);
        return error;
    } else if (argc == 3 && !strcmp(argv[1], "add_mount_type")) {
        struct st_susfs_suspicious_mount_type info;
        strncpy(info.name, argv[2], SUSFS_MAX_LEN_MOUNT_TYPE_NAME);
        prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_MOUNT_TYPE, &info, NULL, &error);
        return error;
    } else if (argc == 3 && !strcmp(argv[1], "add_mount_path")) {
        struct st_susfs_suspicious_mount_path info;
        struct stat sb;
        strncpy(info.name, argv[2], SUSFS_MAX_LEN_PATHNAME);
        if (!get_file_stat(argv[2], &sb)) {
            info.ino = sb.st_ino;
        } else {
            info.ino = 0;
        }
        prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_MOUNT_PATH, &info, NULL, &error);
        return error;
    } else if (argc == 12 && !strcmp(argv[1], "add_suspicious_kstat_statically")) {
        struct st_susfs_suspicious_kstat info;
        struct stat sb;
        int hide_in_maps;
        char* endptr;

        unsigned long ino, dev, atime_nsec, mtime_nsec, ctime_nsec;
        long atime, mtime, ctime;

        if (get_file_stat(argv[2], &sb)) {
            log("[-] Failed to get stat from path: '%s'\n", argv[2]);
            return 1;
        }
        if (!isNumeric(argv[3])) {
            print_help();
            return 1;
        }
        hide_in_maps = atoi(argv[3]);
        if (hide_in_maps < 0 && hide_in_maps > 1) {
            print_help();
            return 1;
        }
        if (strcmp(argv[4], "default")) {
            ino = strtoul(argv[4], &endptr, 10);
            if (*endptr != '\0') {
                print_help();
                return 1;
            }
            info.target_ino = sb.st_ino;
            sb.st_ino = ino;
        } else {
            info.target_ino = sb.st_ino;
        }
        if (strcmp(argv[5], "default")) {
            dev = strtoul(argv[5], &endptr, 10);
            if (*endptr != '\0') {
                print_help();
                return 1;
            }
            sb.st_dev = dev;
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
        strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME);
        info.hide_in_maps = hide_in_maps;
        copy_stat_to_suspicious_kstat(&info, &sb);
        prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_SUSPICIOUS_KSTAT_STATICALLY, &info, NULL, &error);
        return error;
    } else if (argc == 4 && !strcmp(argv[1], "add_suspicious_kstat")) {
        struct st_susfs_suspicious_kstat info;
        struct stat sb;
        int hide_in_maps;
        if (get_file_stat(argv[2], &sb)) {
            log("[-] Failed to get stat from path: '%s'\n", argv[2]);
            return 1;
        }
        if (!isNumeric(argv[3])) {
            print_help();
            return 1;
        }
        hide_in_maps = atoi(argv[3]);
        if (hide_in_maps < 0 && hide_in_maps > 1) {
            print_help();
            return 1;
        }
        strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME);
        info.hide_in_maps = hide_in_maps;
        info.target_ino = 0;
        copy_stat_to_suspicious_kstat(&info, &sb);
        prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_SUSPICIOUS_KSTAT, &info, NULL, &error);
        return error;
    } else if (argc == 3 && !strcmp(argv[1], "update_suspicious_kstat")) {
        struct st_susfs_suspicious_kstat info;
        struct stat sb;
        int hide_in_maps;
        if (get_file_stat(argv[2], &sb)) {
            log("[-] Failed to get stat from path: '%s'\n", argv[2]);
            return 1;
        }
        strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME);
        info.target_ino = sb.st_ino;
        prctl(KERNEL_SU_OPTION, CMD_SUSFS_UPDATE_SUSPICIOUS_KSTAT, &info, NULL, &error);
        return error;
    } else if (argc == 3 && !strcmp(argv[1], "add_try_umount")) {
        struct st_susfs_try_umount info;
        strncpy(info.name, argv[2], SUSFS_MAX_LEN_PATHNAME);
        prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_TRY_UMOUNT, &info, NULL, &error);
        return error;
    } else if (argc == 7 && !strcmp(argv[1], "add_uname")) {
        struct st_susfs_uname info;
        strncpy(info.sysname, argv[2], __NEW_UTS_LEN);
        strncpy(info.nodename, argv[3], __NEW_UTS_LEN);
        strncpy(info.release, argv[4], __NEW_UTS_LEN);
        strncpy(info.version, argv[5], __NEW_UTS_LEN);
        strncpy(info.machine, argv[6], __NEW_UTS_LEN);
        prctl(KERNEL_SU_OPTION, CMD_SUSFS_ADD_UNAME, &info, NULL, &error);
        return error;
    } else {
        print_help();
    }
out:
    return 0;
}