# Tests

`check.sh` boots FreeDOS in QEMU, loads LFNTSR and runs the batch files
`basic.bat` (FAT16 hard disk C: and FAT12 floppy A:) and `pass.bat`
(FAT32 disk D:, which LFNTSR hands through to DOS). Each run is compared
with the expected output (`*.exp`). The test also checks the disk images
from Linux: `fsck.fat` must find no errors, and the long file names that
mtools sees must match `*.files`.

    make check            # or: tests/check.sh
    tests/check.sh -u     # accept the current output as expected

Requirements: `nasm`, `qemu-system-i386`, `mtools`, `dosfstools`
(`mkfs.fat`, `fsck.fat`), `python3`.

`lt.asm` builds `LT.COM`, a small command line front end for the
INT 21h/71xxh calls (`LT md "Long Name"`, `LT dir "*.txt"`, ...). It is
handy for experiments on a real machine too.

## fdboot.img

`fdboot.img` is an unmodified FreeDOS 1.44 MB boot floppy (FreeDOS kernel
build 2040 and FreeCOM 0.82pl3), taken from
<https://github.com/infernalheaven/freedosbootdisks>. FreeDOS is free
software under the GNU GPL; the sources are available from
<https://github.com/FDOS/kernel>, <https://github.com/FDOS/freecom> and
<https://www.freedos.org/>. Use `FDBOOT=/path/to/other.img` to test with
another boot disk.
