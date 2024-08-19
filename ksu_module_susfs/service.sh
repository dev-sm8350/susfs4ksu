#!/system/bin/sh
MODDIR=/data/adb/modules/susfs4ksu

SUSFS_BIN=/data/adb/ksu/bin/ksu_susfs

source ${MODDIR}/utils.sh

## sus_su ##
enable_sus_su(){
	## Loop untile sys.boot_completed becomes 1 ##
	while true; do
		if [ "$(getprop sys.boot_completed)" = "0" ]; then
			sleep 1
			continue
		fi
		break
	done
	## Hide the mount point /system/bin ##
	${SUSFS_BIN} add_sus_mount /system/bin
	## Umount it for no root granted process ##
	${SUSFS_BIN} add_try_umount /system/bin 1
	## Create a 'overlay' folder in module root directory for storing the 'su' and sus_su_drv_path in /system/bin/ ##
	SYSTEM_OL=${MODDIR}/overlay
	rm -rf ${SYSTEM_OL}  2>/dev/null
	mkdir -p ${SYSTEM_OL}/system_bin 2>/dev/null
	## Enable sus_su ##
	${SUSFS_BIN} sus_su 1
	## Copy the new generated sus_su_drv_path and 'sus_su' to /system/bin/ and rename 'sus_su' to 'su' ##
	cp -f /data/adb/ksu/bin/sus_su ${SYSTEM_OL}/system_bin/su
	cp -f /data/adb/ksu/bin/sus_su_drv_path ${SYSTEM_OL}/system_bin/sus_su_drv_path
	## Setup permission ##
	susfs_clone_perm ${SYSTEM_OL}/system_bin /system/bin
	susfs_clone_perm ${SYSTEM_OL}/system_bin/su /system/bin/sh
	susfs_clone_perm ${SYSTEM_OL}/system_bin/sus_su_drv_path /system/bin/sh
	## Finally mount the overlay ##
	mount -t overlay overlay -o "lowerdir=${SYSTEM_OL}/system_bin:/system/bin" /system/bin
}

## Enable sus_su ##
#enable_sus_su


