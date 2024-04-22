#!/system/bin/sh

MODDIR=${0%/*}

SUSFS_BIN=/data/adb/ksu/bin/ksu_susfs

## Important Notes: 
## - The following command can be run at other stages like service.sh, boot-completed.sh etc..,
## - This module is just an demo showing how to use ksu_susfs tool to commuicate with kernel

${SUSFS_BIN} enable_log 1

#### For some custom ROM ####
${SUSFS_BIN} add_suspicious_path /system/addon.d
${SUSFS_BIN} add_suspicious_path /data/adbroot
${SUSFS_BIN} add_suspicious_path /vendor/bin/install-recovery.sh
${SUSFS_BIN} add_suspicious_path /system/bin/install-recovery.sh

#### Hide all overlayfs type in /proc/mounts ####
${SUSFS_BIN} add_mount_type overlay

#### Hide some other mount path in /proc/mounts if they are not umounted ####
${SUSFS_BIN} add_mount_path /data/adb
${SUSFS_BIN} add_mount_path /data/app
${SUSFS_BIN} add_mount_path /system/apex/com.android.art/bin/dex2oat
${SUSFS_BIN} add_mount_path /apex/com.android.art/bin/dex2oat
${SUSFS_BIN} add_mount_path /apex/com.android.art/bin/dex2oat32
${SUSFS_BIN} add_mount_path /apex/com.android.art/bin/dex2oat64

#### Always umount /system/etc/hosts if hosts module is used ####
${SUSFS_BIN} add_try_umount /system/etc/hosts

#### Spoof the uname ####
${SUSFS_BIN} add_uname 'default' 'default' '4.9.337-g3291538446b7' 'default' 'default'


#### Enable / Disable susfs logging to kernel, 0 -> disable, 1 -> enable ####
cat <<EOF >/dev/null
${SUSFS_BIN} enable_log 0
EOF

#### To spoof the stat of file/directory that is NOT bind mounted or overlayed ####
cat <<EOF >/dev/null
${SUSFS_BIN} add_sus_kstat_statically '/system/framework/services.jar' 'default' 'default' '1230768000' '0' '1230768000' '0' '1230768000' '0'
EOF

#### To spoof the stat of file/directory that is bind mounted or overlayed ####
cat <<EOF >/dev/null
## First, before bind mount your file/directory, use 'add_sus_kstat' to add the path
${SUSFS_BIN} add_sus_kstat '/system/etc/hosts'

## Now bind mount or overlay your path
mount --bind "$MODDIR/hosts" /system/etc/hosts

## Finally use 'update_sus_kstat' to update the path again for the changed ino and device number
${SUSFS_BIN} update_sus_kstat '/system/etc/hosts'
EOF

#### To spoof the kstat in /proc/self/[maps|smaps] ####
cat <<EOF >/dev/null
TARGET_PID=$(ps -ef | grep "<TARGET_PROCESS_NAME>" | head -n1 | awk '{print $2}')
TARGET_INO=$(cat /proc/${TARGET_PID}/maps | grep -E "/memfd:/some/suspicious/path" | head -n1 | awk '{print $5}')
SPOOFED_INO=$(stat -c "%i" /some/suspicious/path)
SPOOFED_DEV=$(stat -c "%d" /some/suspicious/path)
SPOOFED_PATHNAME="/some/suspicious/path"
${SUSFS_BIN} add_sus_maps_statically ${TARGET_INO} ${SPOOFED_INO} ${SPOOFED_DEV} ${SPOOFED_PATHNAME}
EOF

