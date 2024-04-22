#!/system/bin/sh

MODDIR=${0%/*}

SUSFS_BIN=/data/adb/ksu/bin/ksu_susfs

# Hide meme paths
${SUSFS_BIN} add_sus_path /system/addon.d
${SUSFS_BIN} add_sus_path /data/adbroot
${SUSFS_BIN} add_sus_path /vendor/bin/install-recovery.sh
${SUSFS_BIN} add_sus_path /system/bin/install-recovery.sh

#### To spoof the link path in /proc/self/fd/ ####
${SUSFS_BIN} add_sus_proc_fd_link "/dev/binder" "/dev/null"

# Hide every overlay
${SUSFS_BIN} add_sus_mount overlay

# Hide everything dex2oat
for path in $(grep "dex2oat" /proc/mounts | cut -d " " -f2) ; do ${SUSFS_BIN} add_sus_mount $path ; done
