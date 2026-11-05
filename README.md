# LFNTSR – long file names for plain DOS

LFNTSR is a small TSR that adds the Windows 95 long file name API
(INT 21h, AX=71xxh) to DOS, in the spirit of DOSLFN and LFNDOS. Programs
that know this API (DJGPP and Open Watcom programs, many FreeDOS tools,
...) can then create, find, rename and
delete files with long names on FAT12/FAT16 disks. The names are
stored in standard VFAT format, so Windows, Linux and mtools read
them as usual, and the reverse also works.

* **8086/8088 compatible.** The source is assembled with `cpu 8086`, so
  it uses no 186/286/386 instructions. 32-bit sector numbers and the
  64-bit FILETIME arithmetic are built from 16-bit operations.
* **DOS 3.10 or newer** (MS-DOS, PC DOS, DR-DOS, FreeDOS, ...).
* Written for **NASM** (free, open source). There are no other build
  dependencies.
* About 19 KB resident. It can be loaded high with `LH`/`LOADHIGH`.

## Building

```bash
nasm -f bin -I src/ -o lfntsr.com src/lfntsr.asm
```

or simply `make`. `make check` runs the test suite (see below).

## Usage

```
LFNTSR [/U] [/F] [/Q]
  /U  uninstall
  /F  install even if a long file name API is already present
  /Q  quiet
```

Put `LFNTSR` (or `LH LFNTSR /Q`) into `AUTOEXEC.BAT`. On startup it
reads the active code page (INT 21h/6601h) and the country upper-case
table (6502h). It refuses to load when Windows or another LFN driver
already provides the API, unless you give `/F`.

## Supported functions

| AX     | Function                                    | Notes |
|--------|---------------------------------------------|-------|
| 710Dh  | reset drive                                 | |
| 7139h  | create directory                            | |
| 713Ah  | remove directory                            | |
| 713Bh  | change directory                            | |
| 7141h  | delete file                                 | SI=1: wildcards + attribute masks |
| 7143h  | get/set attributes, times, size             | BL=0..8 |
| 7147h  | get current directory (long form)           | |
| 714Eh  | find first                                  | DOS or FILETIME time format |
| 714Fh  | find next                                   | |
| 71A1h  | find close                                  | 8 concurrent handles (LRU reuse) |
| 7156h  | rename / move file                          | also changing only the letter case |
| 7160h  | truename (CL=0 full, 1 short, 2 long path)  | CH bit 7 = don't expand SUBST |
| 716Ch  | extended open/create                        | |
| 71A0h  | get volume information                      | BX=4006h on FAT12/16 |
| 71A6h  | get file info by handle                     | |
| 71A7h  | FILETIME <-> DOS time conversion            | |
| 71A8h  | generate short name                         | |
| 71A9h  | server open/create                          | same as 716Ch |

Any other 71xxh call returns AX=7100h with carry set, as Windows does
for functions it doesn't support.

## How it works

LFNTSR hooks INT 21h and handles only AH=71h. Each call runs on a private
stack. While a call is in progress it installs its own INT 24h handler
(critical errors return "fail") and an INT 23h handler that ignores
Ctrl-C, so DOS can't abort a program halfway through an update.

* **Path resolution.** A long path is converted into a short one by
  reading directories directly with INT 25h (both the classic and the
  >32 MB packet form). Long names are assembled from the VFAT entries
  and checked against the SFN checksum. The starting point comes from
  DOS itself (TRUENAME of `X:\` and the current directory), which should
  make SUBST'd drives work (not tested yet).
* **Doing the work.** Wherever possible the real operation is done by
  DOS on the short name (mkdir, rmdir, delete, rename, open, set
  attributes). LFNTSR then adds or removes the long name entries in
  front of the short entry with INT 26h, moving the short entry when
  there is no room. A new file with a long name is created
  differently: LFNTSR writes the long name entries and an empty short
  entry, then lets DOS open it.
* **Growing directories.** When a subdirectory needs more room, LFNTSR
  appends a cluster itself (FAT12 and FAT16, all FAT copies). It then
  marks the free-cluster count in DOS's drive parameter block as
  unknown, so DOS recounts.
* **Keeping DOS consistent.** Around every DOS call and direct disk
  access, LFNTSR calls INT 21h/0Dh. Both MS-DOS (4.0 source) and
  FreeDOS write dirty buffers *and invalidate all buffers* there, so
  DOS never works with stale directory or FAT sectors.
* **Short names** follow the Windows 95 rules: `Long File Name.txt` →
  `LONGFI~1.TXT`, then `~2` ..., then `LONGF~10.TXT`. Characters that
  are illegal in short names become `_`. A valid 8.3 name that contains
  lower case letters gets a long name entry to keep its case, as on
  Windows 95.
* **Unicode.** OEM ↔ Unicode tables for code pages 437, 737, 775, 850,
  852, 855, 857, 860, 861, 862, 863, 865, 866 and 869 are built in.
  Only the table for the active code page stays resident. Characters
  the code page can't show appear as `?`, and find-first/next sets the
  corresponding flag in CX.
* **Devices.** Names like `CON`, `NUL` or `LPT1.TXT` are recognized by
  walking the DOS device driver chain and are passed to DOS.

### Drives that are not FAT12/FAT16

Network drives, CD-ROMs (MSCDEX), FAT32 volumes and disks with sector
sizes other than 512 bytes are not modified directly. On these drives
the 71xxh calls are mapped to the normal DOS functions (short names
only). Programs keep working, and 71A0h reports that long names are not
supported (BX=0).

## Limitations

* No FAT32 long name support (FAT32 only exists in DOS 7.1 and FreeDOS;
  those drives are handled as described above).
* Single-byte code pages only (no DBCS).
* 71A6h returns attributes 20h and file index 0. FILETIME values are
  local time (there is no time zone information in DOS).
* 7142h (64-bit seek) and 71AAh (SUBST) are not implemented.
* Moving a directory to another directory fails, because DOS can't do it.
* LFNTSR is not reentrant: a 71xxh call made while another one is in
  progress (e.g. from an interrupt handler) returns 7100h.
* Tested with FreeDOS (kernel 2040) under QEMU. The MS-DOS buffer
  handling was checked against the published MS-DOS 4.0 source but has
  not been tested on real MS-DOS.

## Tests

`tests/` contains an automated test suite. It boots FreeDOS in QEMU with
a FAT12 floppy, a FAT16 hard disk and a FAT32 hard disk, and runs a few
hundred API calls through the `LT.COM` test tool. The output is compared
with the expected output, and the disk images are checked with
`fsck.fat` and mtools from Linux. See [tests/README.md](tests/README.md).

## License

MIT, see [LICENSE](LICENSE). The FreeDOS boot floppy used by the tests
(`tests/fdboot.img`) is GPL software; see `tests/README.md`.
