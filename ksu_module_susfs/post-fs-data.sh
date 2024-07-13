#!/system/bin/sh

MODDIR=${0%/*}

SUSFS_BIN=/data/adb/ksu/bin/ksu_susfs

## Important Notes: 
## - The following command can be run at other stages like service.sh, boot-completed.sh etc..,
## - This module is just an demo showing how to use ksu_susfs tool to commuicate with kernel

#### For some custom ROM ####
cat <<EOF >/dev/null
${SUSFS_BIN} add_sus_path /system/addon.d
${SUSFS_BIN} add_sus_path /data/adbroot
${SUSFS_BIN} add_sus_path /vendor/bin/install-recovery.sh
${SUSFS_BIN} add_sus_path /system/bin/install-recovery.sh
EOF

#### Trying to hide the mounted path in /proc/self/[mounts|mountstat|mountinfo] for all processes ####
cat <<EOF >/dev/null
${SUSFS_BIN} add_sus_mount /system/apex/com.android.art/bin/dex2oat
${SUSFS_BIN} add_sus_mount /apex/com.android.art/bin/dex2oat
${SUSFS_BIN} add_sus_mount /apex/com.android.art/bin/dex2oat32
${SUSFS_BIN} add_sus_mount /apex/com.android.art/bin/dex2oat64
EOF

#### Always umount /system/etc/hosts if hosts module is used ####
## Note that susfs's try_umount takes precedence of ksu's try_umount ##
## Also, umount can be detected in ksu, or try using add_sus_mount instead ##
cat <<EOF >/dev/null
${SUSFS_BIN} add_try_umount /system/etc/hosts '1'
EOF

#### Spoof the uname ####
cat <<EOF >/dev/null
${SUSFS_BIN} set_uname 'default' 'default' '4.9.337-g3291538446b7' 'default' 'default'
EOF

#### Enable / Disable susfs logging to kernel, 0 -> disable, 1 -> enable ####
cat <<EOF >/dev/null
${SUSFS_BIN} enable_log 0
EOF

#### To spoof the stat of file/directory statically ####
cat <<EOF >/dev/null
${SUSFS_BIN} add_sus_kstat_statically '/system/framework/services.jar' 'default' 'default' 'default' '1230768000' '0' '1230768000' '0' '1230768000' '0'
EOF

#### To spoof the stat of file/directory dynamically ####
cat <<EOF >/dev/null
## First, before bind mount your file/directory, use 'add_sus_kstat' to add the path
${SUSFS_BIN} add_sus_kstat '/system/etc/hosts'

## Now bind mount or overlay your path
mount --bind "$MODDIR/hosts" /system/etc/hosts

## Finally use 'update_sus_kstat' to update the path again for the changed ino and device number
${SUSFS_BIN} update_sus_kstat '/system/etc/hosts'
EOF

#### To spoof only the ino and dev in /proc/self/[maps|smaps] dynamically ####
cat <<EOF >/dev/null
${SUSFS_BIN} add_sus_maps ${SPOOFED_PATHNAME}
mount --bind "$MODDIR/hosts" /system/etc/hosts
${SUSFS_BIN} update_sus_maps ${SPOOFED_PATHNAME}
EOF

#### To spoof whole entry in /proc/self/[maps|smaps] statically ####
cat <<EOF >/dev/null
## Mode 1 ##
TARGET_PID=$(/system/bin/ps -ef | grep "PROCESS_NAME" | head -n1 | awk '{print $2}')
PATHNAME_TO_SEARCH="/system/etc/hosts"
TARGET_INO=$(cat /proc/${TARGET_PID}/maps | grep -E "${PATHNAME_TO_SEARCH}" | head -n1 | awk '{print $5}')
if [ ! -z ${TARGET_INO} ]; then
    MODE=1
    SPOOFED_PATHNAME="empty"
    SPOOFED_INO=0
    SPOOFED_DEV=0
    SPOOFED_PGOFF=0
    SPOOFED_PROT="---p"
    ${SUSFS_BIN} add_sus_maps_statically ${MODE} ${TARGET_INO} ${SPOOFED_PATHNAME} ${SPOOFED_INO} ${SPOOFED_DEV} ${SPOOFED_PGOFF} ${SPOOFED_PROT}
fi

## Mode 2, entry is not isolated, previous ino of target ino and next ino of target ino == (target ino +- 1) ##
TARGET_PID=$(/system/bin/ps -ef | grep "PROCESS_NAME" | head -n1 | awk '{print $2}')
PATHNAME_TO_SEARCH="/system/etc/hosts"
TARGET_INO=$(cat /proc/${TARGET_PID}/maps | grep -E "${PATHNAME_TO_SEARCH}" | head -n1 | awk '{print $5}')
if [ ! -z ${TARGET_INO} ]; then
    MODE=2
    TARGET_ADDR_SIZE=4096
    TARGET_PGOFF=0
    TARGET_PROT="r--p"
    SPOOFED_PATHNAME=/syste/etc/my_hosts
    SPOOFED_INO="default"
    SPOOFED_DEV="default"
    SPOOFED_PGOFF="default"
    SPOOFED_PROT="default"
    IS_ISOLATED_ENTRY=0
    ${SUSFS_BIN} add_sus_maps_statically ${MODE} ${TARGET_INO} ${TARGET_ADDR_SIZE} ${TARGET_PGOFF} ${TARGET_PROT} ${SPOOFED_PATHNAME} ${SPOOFED_INO} ${SPOOFED_DEV} ${SPOOFED_PGOFF} ${SPOOFED_PROT} ${IS_ISOLATED_ENTRY}
fi

## Mode 2, but entry is isolated, not consecutive ##
TARGET_PID=$(/system/bin/ps -ef | grep "PROCESS_NAME" | head -n1 | awk '{print $2}')
PATHNAME_TO_SEARCH="/system/etc/hosts"
TARGET_INO=$(cat /proc/${TARGET_PID}/maps | grep -E "${PATHNAME_TO_SEARCH}" | head -n1 | awk '{print $5}')
if [ ! -z ${TARGET_INO} ]; then
    MODE=2
    TARGET_PGOFF=0
    TARGET_PROT="r--p"
    SPOOFED_PATHNAME=/syste/etc/my_hosts
    SPOOFED_INO=0
    SPOOFED_DEV=0
    SPOOFED_PGOFF=0
    SPOOFED_PROT="---p"
    IS_ISOLATED_ENTRY=1
    ${SUSFS_BIN} add_sus_maps_statically ${MODE} ${TARGET_INO} ${TARGET_PGOFF} ${TARGET_PROT} ${SPOOFED_PATHNAME} ${SPOOFED_INO} ${SPOOFED_DEV} ${SPOOFED_PGOFF} ${SPOOFED_PROT} ${IS_ISOLATED_ENTRY}
fi

## Mode 3, current entry ino is 0, compare with prev_target_ino and next_target_ino ##
## Note: when prev_target_ino or next_target_ino is 0, it will not be compared, if both are not zero, both will be compared ##
MODE=3
PREV_TARGET_INO=0
PREV_TARGET_INO=30
SPOOFED_PATHNAME="empty"
SPOOFED_INO=0
SPOOFED_DEV=0
SPOOFED_PGOFF=0
SPOOFED_PROT="---p"
${SUSFS_BIN} add_sus_maps_statically ${MODE} ${TARGET_INO} ${TARGET_PGOFF} ${TARGET_PROT} ${SPOOFED_PATHNAME} ${SPOOFED_INO} ${SPOOFED_DEV} ${SPOOFED_PGOFF} ${SPOOFED_PROT} ${IS_ISOLATED_ENTRY}

## Mode 4, all entries match with [is_file,target_addr_size,target_prot,target_pgoff,target_dev] will be spoofed with user defined entry ##
MODE=4
IS_FILE=1
TARGET_DEV=6
TARGET_PGOFF=$(echo $((0x2000000)))
TARGET_PROT="rw-p"
TARGET_ADDR_SIZE=$(echo $((0x2000000)))
SPOOFED_PATHNAME="empty"
SPOOFED_INO=0
SPOOFED_DEV=0
SPOOFED_PGOFF=0
SPOOFED_PROT="---p"
${SUSFS_BIN} add_sus_maps_statically ${MODE} ${IS_FILE} ${TARGET_DEV} ${TARGET_PGOFF} ${TARGET_PROT} ${TARGET_ADDR_SIZE} ${SPOOFED_PATHNAME} ${SPOOFED_INO} ${SPOOFED_DEV} ${SPOOFED_PGOFF} ${SPOOFED_PROT}
EOF

#### To spoof the link path in /proc/self/fd/ ####
cat <<EOF >/dev/null
${SUSFS_BIN} add_sus_proc_fd_link "/dev/binder" "/dev/null"
EOF

#### To prevent a memfd from being created by all process ####
cat <<EOF >/dev/null
${SUSFS_BIN} add_sus_memfd "memfd:/jit-cache"
EOF
