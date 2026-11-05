#!/bin/sh
# Prepare the hard disk for tunnel.bat: \ORPHANS holds a file whose short
# name was changed behind the back of its long name entries (what a plain
# MS-DOS rename does), so that they are orphans, and a long named *.ZZZ file
# for an FCB wildcard delete in that directory (which sweeps it).
#   tests/mkorphan.sh IMAGE@@OFFSET
set -e
img=${1%@@*}
tmp=$(mktemp)
echo orphan > "$tmp"
mmd -i "$1" ::/ORPHANS
mcopy -i "$1" "$tmp" "::/ORPHANS/Orphan Long Name.txt"
mcopy -i "$1" "$tmp" "::/ORPHANS/Victim Long Name.zzz"
rm -f "$tmp"
python3 - "$img" <<'PY'
import sys
d = bytearray(open(sys.argv[1], 'rb').read())
i = d.index(b'ORPHAN~1TXT')
assert d.find(b'ORPHAN~1TXT', i + 1) < 0
d[i:i + 11] = b'ORPHANXXTXT'
open(sys.argv[1], 'wb').write(d)
PY
