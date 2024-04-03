
SUSFS_BIN_ARM64=${TMPDIR}/susfs/tools/ksu_susfs_arm64
SUSFS_BIN_ARM=${TMPDIR}/susfs/tools/ksu_susfs_arm

DEST_PATH=/data/adb/ksu_susfs

unzip ${ZIPFILE} -d ${TMPDIR}/susfs


if [ ${ARCH} = "arm64" ]; then
    ui_print "copying ${SUSFS_BIN_ARM64} to ${DEST_PATH}"
    cp ${SUSFS_BIN_ARM64} ${DEST_PATH}
elif [ ${ARCH} = "arm" ]; then
    ui_print "copying ${SUSFS_BIN_ARM} to ${DEST_PATH}"
    cp ${SUSFS_BIN_ARM} ${DEST_PATH}
fi

chmod 777 ${DEST_PATH}/ksu_susfs
chmod 644 ${MODPATH}/post-fs-data.sh 
chmod 644 ${MODPATH}/service.sh 
chmod 644 ${MODPATH}/uninstall.sh 


rm -rf ${MODPATH}/tools
rm ${MODPATH}/customize.sh ${MODPATH}/build.sh


