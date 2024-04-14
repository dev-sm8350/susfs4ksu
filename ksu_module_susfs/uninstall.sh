MODDIR=${0%/*}

FILES_TO_DELETE=(
/data/adb/ksu/bin/ksu_susfs
)

for FILE in ${FILES_TO_DELETE[@]}; do
    rm ${FILE} 2>/dev/null
done
