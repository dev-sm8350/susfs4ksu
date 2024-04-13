#!/system/bin/sh

MODDIR=${0%/*}

SUSFS_BIN=/data/adb/ksu/bin/ksu_susfs

## Enable / Disable susfs logging to kernel, 0 -> disable, 1 -> enable ##
#${SUSFS_BIN} enable_log 0

${SUSFS_BIN} add_suspicious_path /system/addon.d
${SUSFS_BIN} add_suspicious_path /data/adbroot
${SUSFS_BIN} add_suspicious_path /system/lib/libzygisk.so
${SUSFS_BIN} add_suspicious_path /system/lib64/libzygisk.so
${SUSFS_BIN} add_suspicious_path /dev/zygisk
${SUSFS_BIN} add_suspicious_path /vendor/bin/install-recovery.sh
${SUSFS_BIN} add_suspicious_path /system/bin/install-recovery.sh

${SUSFS_BIN} add_mount_type overlay

${SUSFS_BIN} add_mount_path /data/adb
${SUSFS_BIN} add_mount_path /data/app
${SUSFS_BIN} add_mount_path /apex/com.android.art/bin/dex2oat
${SUSFS_BIN} add_mount_path /system/apex/com.android.art/bin/dex2oat
${SUSFS_BIN} add_mount_path /system/etc/preloaded-classes
${SUSFS_BIN} add_mount_path /dev/zygisk

${SUSFS_BIN} add_try_umount /system/etc/hosts

${SUSFS_BIN} add_uname 'default' 'default' '4.9.337-g3291538446b7' 'default' 'default'

#### To spoof the stat of file/directory that is NOT bind mounted or overlayed ####
# ${SUSFS_BIN} add_suspicious_kstat_statically '/system/framework/services.jar' '0' 'default' 'default' '1230768000' '0' '1230768000' '0' '1230768000' '0'

#### To spoof the stat of file/directory that is bind mounted or overlayed ####
## First, before bind mount your file/directory, use 'add_suspicious_kstat' to add the path 
# ${SUSFS_BIN} add_suspicious_kstat '/system/etc/hosts' '0'
## Now bind mount or overlay your path
# mount --bind "$MODDIR/hosts" /system/etc/hosts
## Finally use 'update_suspicious_kstat' to update the path again for the changed ino and device number
# ${SUSFS_BIN} update_suspicious_kstat '/system/etc/hosts'
