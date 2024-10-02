#!/system/bin/sh

MODDIR=/data/adb/modules/susfs4ksu

SUSFS_BIN=/data/adb/ksu/bin/ksu_susfs

source ${MODDIR}/utils.sh

## Important Notes:
## - The following command can be run at other stages like service.sh, boot-completed.sh etc..,
## - This module is just an demo showing how to use ksu_susfs tool to commuicate with kernel
##
## - for add_sus_path and add_sus_kstat if target file/dir is re-created or its ino number is changed, you have to re-run the same susfs command
## - for add_sus_mount, if target path is umounted globally, you have to re-run the same susfs command
##
## - add_sus_path, add_sus_kstat, add_sus_mount and set_uname are allowed to be updated with new spoofed values
##   that said, you can run multiple times with same target input but with different spoofed values.
##
## - BE CAUTIOUS that add_sus_path, add_sus_mount must be added after overlay or mounted operations on target files/direcotories,
##   because susfs now mainly compare target's inode to determine whether it is a sus path, sus kstat or sus mount.
##

#### Hide target path and all its sub-paths for process with uid > 2000 ####
## Make sure the target file/directory has no more overlay/mount operation on going. Or add it after it is done being overlayed or mounted ##
# For some custom ROM #
cat <<EOF >/dev/null
${SUSFS_BIN} add_sus_path /system/addon.d
${SUSFS_BIN} add_sus_path /vendor/bin/install-recovery.sh
${SUSFS_BIN} add_sus_path /system/bin/install-recovery.sh
EOF

#### Hide the mounted path in /proc/self/[mounts|mountstat|mountinfo] for all processes ####
## Make sure the target file/directory has no more overlay/mount operation on going. Or add it after it is done being overlayed or mounted ##
cat <<EOF >/dev/null
# for default ksu mounts #
${SUSFS_BIN} add_sus_mount /data/adb/modules
${SUSFS_BIN} add_sus_mount /debug_ramdisk
# for host file #
${SUSFS_BIN} add_sus_mount /system/etc/hosts
## for lsposed, choose those that show up in your mountinfo, no need to add them all ##
# also somehow you may need to add the source path to sus mount as well like below #
${SUSFS_BIN} add_sus_mount /data/adb/modules/zygisk_lsposed/bin/dex2oat
${SUSFS_BIN} add_sus_mount /data/adb/modules/zygisk_lsposed/bin/dex2oat32
${SUSFS_BIN} add_sus_mount /data/adb/modules/zygisk_lsposed/bin/dex2oat64
${SUSFS_BIN} add_sus_mount /system/apex/com.android.art/bin/dex2oat
${SUSFS_BIN} add_sus_mount /system/apex/com.android.art/bin/dex2oat32
${SUSFS_BIN} add_sus_mount /system/apex/com.android.art/bin/dex2oat64
${SUSFS_BIN} add_sus_mount /apex/com.android.art/bin/dex2oat
${SUSFS_BIN} add_sus_mount /apex/com.android.art/bin/dex2oat32
${SUSFS_BIN} add_sus_mount /apex/com.android.art/bin/dex2oat64
EOF

#### Umount the mounted path for no root granted process ####
# Please be reminded that susfs's try_umount takes precedence of ksu's try_umount #
cat <<EOF >/dev/null
# for /system/etc/hosts #
${SUSFS_BIN} add_try_umount /system/etc/hosts 1
# for lsposed, choose those that show up in your mountinfo, no need to add them all #
${SUSFS_BIN} add_try_umount /system/apex/com.android.art/bin/dex2oat 1
${SUSFS_BIN} add_try_umount /system/apex/com.android.art/bin/dex2oat32 1
${SUSFS_BIN} add_try_umount /system/apex/com.android.art/bin/dex2oat64 1
${SUSFS_BIN} add_try_umount /apex/com.android.art/bin/dex2oat 1
${SUSFS_BIN} add_try_umount /apex/com.android.art/bin/dex2oat32 1
${SUSFS_BIN} add_try_umount /apex/com.android.art/bin/dex2oat64 1
EOF

#### Spoof the stat of file/directory dynamically ####
cat <<EOF >/dev/null
# First, before bind mount your file/directory, use 'add_sus_kstat' to add the path #
${SUSFS_BIN} add_sus_kstat '/system/etc/hosts'

# Now bind mount or overlay your path #
mount -o bind "$MODDIR/hosts" /system/etc/hosts

# Finally use 'update_sus_kstat' to update the path again for the changed ino and device number #
# update_sus_kstat updates ino, but blocks and size are remained the same as current stat #
${SUSFS_BIN} update_sus_kstat '/system/etc/hosts'

# if you want to fully clone the stat value from the original stat, use update_sus_kstat_full_clone instead #
${SUSFS_BIN} update_sus_kstat_full_clone '/system/etc/hosts'
EOF

#### Spoof the stat of file/directory statically ####
cat <<EOF >/dev/null
Usage: ksu_susfs add_sus_kstat_statically </path/of/file_or_directory> \
                        <ino> <dev> <nlink> <size> <atime> <atime_nsec> <mtime> <mtime_nsec> <ctime> <ctime_nsec> \
                        <blocks> <blksize>
${SUSFS_BIN} add_sus_kstat_statically '/system/framework/services.jar' 'default' 'default' 'default' 'default' '1230768000' '0' '1230768000' '0' '1230768000' '0' 'default' 'default'
EOF

#### Spoof the uname ####
# you can get your uname args by running 'uname {-r|-v}' on your stock ROM #
# pass 'default' to tell susfs to use the default value by uname #
cat <<EOF >/dev/null
${SUSFS_BIN} set_uname '5.15.137-android14-11-gb572b1fac135-ab11919372' '#1 SMP PREEMPT Mon Jun 3 16:35:10 UTC 2024'
EOF


