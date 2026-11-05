#!/bin/sh
# Run all emulator tests and compare with the expected results.
#   tests/check.sh [-u]     (-u: update the expected files)
here=$(cd "$(dirname "$0")" && pwd)
work=${WORK:-/tmp/lfntsr-check}
update=0; [ "$1" = "-u" ] && update=1
fail=0

compare() {     # compare FILE EXPECTED
    if [ $update = 1 ]; then cp "$1" "$2"; return; fi
    if ! diff -u "$2" "$1"; then echo "FAIL: $1 differs from $2"; fail=1; fi
}

fsck_img() {    # fsck_img IMAGE OFFSET_SECTORS
    dd if="$1" of="$1.part" bs=512 skip=$2 2>/dev/null
    if ! fsck.fat -n "$1.part" >"$1.fsck" 2>&1; then
        echo "FAIL: fsck.fat on $1"; cat "$1.fsck"; fail=1
    fi
    rm -f "$1.part"
}

listing() {     # listing IMAGE@@OFFSET -> long names of all files
    LC_ALL=C.UTF-8 mdir -/ -b -i "$1" :: | sort
}

for t in basic pass; do
    echo "== $t"
    env FAT32=1 "$here/run.sh" "$here/$t.bat" "$work/$t" || fail=1
    compare "$work/$t/out.txt" "$here/$t.exp"
    fsck_img "$work/$t/hd.img" 63
    fsck_img "$work/$t/fd.img" 0
    fsck_img "$work/$t/hd2.img" 63
    off=$(cat "$work/$t/hdoff")
    listing "$work/$t/hd.img$off" > "$work/$t/hd.files"
    compare "$work/$t/hd.files" "$here/$t.files"
done
[ $fail = 0 ] && echo "ALL TESTS PASSED"
exit $fail
