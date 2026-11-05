#!/bin/sh
# Build an MS-DOS 6.x test boot floppy from the original setup disk 1.
#   tests/mkmsdos.sh 144OEM1.IMG OUT.IMG
# The setup disk is reduced to the system files plus a few tools; SUBST,
# SHARE, RAMDRIVE and HIMEM are unpacked by booting it once and running
# MS-DOS' own EXPAND.EXE.  (The MS-DOS files are not part of this project.)
set -e
in=$1; out=$2
tmp=$(mktemp -d)
cp "$in" "$out"
truncate -s 1474560 "$out"      # some images lack the trailing empty sectors
keep="IO.SYS MSDOS.SYS COMMAND.COM EXPAND.EXE SUBST.EX_ SHARE.EX_ RAMDRIVE.SY_ HIMEM.SY_ CHKDSK.EXE"
for f in $(mdir -b -i "$out" :: | sed 's|^::/||'); do
    case " $keep " in *" $f "*) ;; *) mdel -i "$out" "::$f" ;; esac
done
printf '@echo off\r\n' > "$tmp/AUTOEXEC.BAT"
for f in SUBST.EX_:SUBST.EXE SHARE.EX_:SHARE.EXE RAMDRIVE.SY_:RAMDRIVE.SYS HIMEM.SY_:HIMEM.SYS; do
    printf 'EXPAND A:\\%s A:\\%s\r\n' "${f%%:*}" "${f##*:}" >> "$tmp/AUTOEXEC.BAT"
done
printf 'A:\\EXIT\r\n' >> "$tmp/AUTOEXEC.BAT"
printf 'mov al,0\nout 0xf4,al\nhlt\n' > "$tmp/exit.asm"
nasm -f bin -o "$tmp/EXIT.COM" "$tmp/exit.asm"
mcopy -o -i "$out" "$tmp/AUTOEXEC.BAT" "$tmp/EXIT.COM" ::
timeout 120 qemu-system-i386 -display none -m 4 \
    -drive file="$out",format=raw,if=floppy -boot a \
    -device isa-debug-exit,iobase=0xf4 || true
mdel -i "$out" ::SUBST.EX_ ::SHARE.EX_ ::RAMDRIVE.SY_ ::HIMEM.SY_ ::AUTOEXEC.BAT ::EXIT.COM
mlabel -i "$out" ::MSDOSTEST     # label in boot sector and root directory
mdir -i "$out" ::
rm -rf "$tmp"
