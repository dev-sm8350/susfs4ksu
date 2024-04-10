#!/system/bin/sh

MODDIR=${0%/*}

SUSFS_BIN=/data/adb/ksu/bin/ksu_susfs


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

