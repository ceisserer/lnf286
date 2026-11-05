#!/usr/bin/env python3
"""Create a raw hard disk image with a classic DOS MBR (16 heads, 63 sectors
per track, partition starting at sector 63) and one primary partition.
usage: mkhd.py IMAGE CYLINDERS TYPE  -> prints "start size" (sectors)"""
import struct, sys
img, cyls, ptype = sys.argv[1], int(sys.argv[2]), int(sys.argv[3], 0)
H, S = 16, 63
total = cyls * H * S
start, size = S, total - S
def chs(lba):
    c = lba // (H * S); h = (lba // S) % H; s = lba % S + 1
    c = min(c, 1023)
    return bytes([h, ((c >> 2) & 0xC0) | s, c & 0xFF])
entry = bytes([0x80]) + chs(start) + bytes([ptype]) + chs(start + size - 1) \
        + struct.pack("<II", start, size)
mbr = bytearray(512)
mbr[446:462] = entry
mbr[510:512] = b"\x55\xaa"
with open(img, "wb") as f:
    f.write(mbr)
    f.truncate(total * 512)
print(start, size)
