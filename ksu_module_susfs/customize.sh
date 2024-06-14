
SUSFS_BIN_ARM64=${TMPDIR}/susfs/tools/ksu_susfs_arm64
SUSFS_BIN_ARM=${TMPDIR}/susfs/tools/ksu_susfs_arm

DEST_BIN_DIR=/data/adb/ksu/bin
DEST_BIN_PATH=${DEST_BIN_DIR}/ksu_susfs

if [ ! -d ${DEST_BIN_DIR} ]; then
    ui_print "'${DEST_BIN_DIR}' not existed, installation aborted."
    rm -rf ${MODPATH}    
    exit 1
fi

unzip ${ZIPFILE} -d ${TMPDIR}/susfs

if [ ${ARCH} = "arm64" ]; then
    ui_print "copying ${SUSFS_BIN_ARM64} to ${DEST_BIN_PATH}"
    cp ${SUSFS_BIN_ARM64} ${DEST_BIN_PATH}
elif [ ${ARCH} = "arm" ]; then
    ui_print "copying ${SUSFS_BIN_ARM} to ${DEST_BIN_PATH}"
    cp ${SUSFS_BIN_ARM} ${DEST_BIN_PATH}
fi

chmod 755 ${DEST_BIN_PATH}
chmod 644 ${MODPATH}/post-fs-data.sh 
chmod 644 ${MODPATH}/service.sh 
chmod 644 ${MODPATH}/uninstall.sh 


rm -rf ${MODPATH}/tools
rm ${MODPATH}/customize.sh ${MODPATH}/README.md


