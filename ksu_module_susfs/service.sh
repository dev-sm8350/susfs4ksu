#!/system/bin/sh
MODDIR=/data/adb/modules/susfs4ksu

SUSFS_BIN=/data/adb/ksu/bin/ksu_susfs

source ${MODDIR}/utils.sh

## Hexpatch prop name for newer pixel device ##
cat <<EOF >/dev/null
# Remember the length of search value and replace value has to be the same #
resetprop -n "ro.boot.verifiedbooterror" "0"
susfs_hexpatch_prop_name "ro.boot.verifiedbooterror" "verifiedbooterror" "hello_my_newworld"

resetprop -n "ro.boot.verifyerrorpart" "true"
susfs_hexpatch_prop_name "ro.boot.verifyerrorpart" "verifyerrorpart" "letsgopartyyeah"

resetprop --delete "crashrecovery.rescue_boot_count"
EOF

# NOTE: sus_su 2 can be only run during or after service stage #
# uncomment it below to enable sus_su with mode 2 #
${SUSFS_BIN} sus_su 2

## Disable susfs kernel log ##
${SUSFS_BIN} enable_log 0

