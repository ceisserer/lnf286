#!/usr/bin/env python3
"""Report orphaned VFAT long name entries in a FAT12/FAT16 image.

    lfncheck.py IMAGE [OFFSET_SECTORS]

An orphan is a live long name entry that is not part of a complete chain
(last part first, sequence numbers descending to 1, same checksum) directly
followed by the short entry it belongs to.  fsck.fat -n does not report
these.  Exit code 1 if any are found.
"""
import struct
import sys


def checksum(name11):
    s = 0
    for b in name11:
        s = (((s & 1) << 7) | (s >> 1)) + b & 0xFF
    return s


def main():
    data = open(sys.argv[1], 'rb').read()
    base = int(sys.argv[2]) * 512 if len(sys.argv) > 2 else 0
    bs = data[base:base + 512]
    bps, spc, rsv, nfats, rootents, tot16, _, fatsz = struct.unpack_from('<HBHBHHBH', bs, 11)
    tot = tot16 or struct.unpack_from('<I', bs, 32)[0]
    rootsecs = (rootents * 32 + bps - 1) // bps
    datastart = rsv + nfats * fatsz + rootsecs
    nclus = (tot - datastart) // spc
    fat16 = nclus >= 4085
    fat = data[base + rsv * bps: base + (rsv + fatsz) * bps]

    def nxt(c):
        if fat16:
            return struct.unpack_from('<H', fat, c * 2)[0]
        v = struct.unpack_from('<H', fat, c * 3 // 2)[0]
        return v >> 4 if c & 1 else v & 0xFFF

    def sec(n):
        return data[base + n * bps: base + (n + 1) * bps]

    def dir_bytes(clus):
        if clus == 0:
            return b''.join(sec(rsv + nfats * fatsz + i) for i in range(rootsecs))
        out = []
        end = 0xFFF8 if fat16 else 0xFF8
        while 2 <= clus < end:
            first = datastart + (clus - 2) * spc
            out.extend(sec(first + i) for i in range(spc))
            clus = nxt(clus)
        return b''.join(out)

    bad = []

    def walk(clus, path):
        d = dir_bytes(clus)
        pending = []        # entry indexes of the current chain
        seq = chk = None

        def orphans():
            bad.extend('%s/ entry %d' % (path, x) for x in pending)
            pending.clear()

        for i in range(0, len(d), 32):
            e = d[i:i + 32]
            if e[0] == 0:
                break
            if e[0] == 0xE5:
                orphans()
                continue
            if e[11] == 0x0F:
                n = e[0] & 0x1F
                if e[0] & 0x40:
                    orphans()
                    pending.append(i // 32); seq, chk = n, e[13]
                elif pending and n == seq - 1 and e[13] == chk:
                    pending.append(i // 32); seq = n
                else:
                    pending.append(i // 32); orphans()
                continue
            if pending and not (seq == 1 and checksum(e[:11]) == chk):
                orphans()
            pending.clear()
            if e[11] & 0x10 and e[0] != ord('.'):
                walk(struct.unpack_from('<H', e, 26)[0],
                     path + '/' + e[:11].decode('latin-1').strip())
        orphans()

    walk(0, '')
    for b in bad:
        print('orphaned long name entry:', b)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
