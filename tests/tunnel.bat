@echo off
REM Plain DOS create / delete / rename: long name tunnelling, removal of the
REM long name entries, find handles
A:\LFNTSR > A:\OUT.TXT
SET LT=A:\LT
C:
REM ---- orphaned long name entries (made by mkorphan.sh) are removed by the
REM      sweep after an FCB wildcard delete in their directory
%LT% cd C:\ORPHANS >> A:\OUT.TXT
%LT% dir * >> A:\OUT.TXT
%LT% fdel *.ZZZ >> A:\OUT.TXT
%LT% dir * >> A:\OUT.TXT
%LT% md "C:\Tunnel Test" >> A:\OUT.TXT
%LT% cd "C:\Tunnel Test" >> A:\OUT.TXT
REM ---- delete + create (Write's "Save As")
%LT% mk "Save Demo Document.wri" first >> A:\OUT.TXT
%LT% sdel SAVEDE~1.WRI >> A:\OUT.TXT
%LT% screat SAVEDE~1.WRI >> A:\OUT.TXT
%LT% cat "Save Demo Document.wri" >> A:\OUT.TXT
REM ---- temporary file + renames
%LT% mk "Long Report.txt" v1 >> A:\OUT.TXT
%LT% screat REPORT.TMP >> A:\OUT.TXT
%LT% sren LONGRE~1.TXT LONGRE~1.BAK >> A:\OUT.TXT
%LT% sren REPORT.TMP LONGRE~1.TXT >> A:\OUT.TXT
%LT% cat "Long Report.txt" >> A:\OUT.TXT
REM ---- create new (5Bh) and extended open (6Ch)
%LT% mk "Created With 5Bh.txt" x >> A:\OUT.TXT
%LT% sdel CREATE~1.TXT >> A:\OUT.TXT
%LT% snew CREATE~1.TXT >> A:\OUT.TXT
%LT% mk "Opened With 6Ch.txt" x >> A:\OUT.TXT
%LT% sdel OPENED~1.TXT >> A:\OUT.TXT
%LT% sopen OPENED~1.TXT >> A:\OUT.TXT
%LT% sopen OPENED~1.TXT >> A:\OUT.TXT
REM ---- plain delete / rename only: the long name entries must go
%LT% mk "Deleted For Good.txt" x >> A:\OUT.TXT
%LT% sdel DELETE~1.TXT >> A:\OUT.TXT
%LT% mk "Renamed By DOS.txt" x >> A:\OUT.TXT
%LT% sren RENAME~1.TXT PLAIN.TXT >> A:\OUT.TXT
%LT% screat OTHER.TXT >> A:\OUT.TXT
REM ---- FCB delete / rename
%LT% mk "Fcb Delete One.xyz" 1 >> A:\OUT.TXT
%LT% mk "Fcb Delete Two.xyz" 2 >> A:\OUT.TXT
%LT% fdel *.XYZ >> A:\OUT.TXT
%LT% mk "Fcb Rename Me.abc" x >> A:\OUT.TXT
%LT% fren FCBREN~1.ABC FCBPLAIN.ABC >> A:\OUT.TXT
%LT% mk "Fcb Wild One.qqq" 1 >> A:\OUT.TXT
%LT% mk "Fcb Wild Two.qqq" 2 >> A:\OUT.TXT
%LT% fren *.QQQ *.RRR >> A:\OUT.TXT
REM ---- tunnelling stays in its directory
%LT% mk "Dir Bound.txt" x >> A:\OUT.TXT
%LT% sdel DIRBOU~1.TXT >> A:\OUT.TXT
%LT% md Sub >> A:\OUT.TXT
%LT% screat SUB\DIRBOU~1.TXT >> A:\OUT.TXT
%LT% screat DIRBOU~1.TXT >> A:\OUT.TXT
%LT% dir * >> A:\OUT.TXT
%LT% dir "Sub\*" >> A:\OUT.TXT
REM ---- root directory of the floppy
%LT% mk "A:\Floppy Save Test.txt" x >> A:\OUT.TXT
%LT% sdel A:\FLOPPY~1.TXT >> A:\OUT.TXT
%LT% screat A:\FLOPPY~1.TXT >> A:\OUT.TXT
%LT% dir "A:\Flop*" >> A:\OUT.TXT
REM ---- after 15 seconds the long name is forgotten
%LT% mk "Expired Name.txt" x >> A:\OUT.TXT
%LT% sdel EXPIRE~1.TXT >> A:\OUT.TXT
%LT% wait 17 >> A:\OUT.TXT
%LT% screat EXPIRE~1.TXT >> A:\OUT.TXT
%LT% dir "EXP*" >> A:\OUT.TXT
REM ---- find handles: a handle taken over by a newer search is invalid
%LT% finds 8 * >> A:\OUT.TXT
%LT% finds 9 * >> A:\OUT.TXT
REM ---- /T:0 (no tunnel), /H:12
A:\LFNTSR /U >> A:\OUT.TXT
A:\LFNTSR /H:0 >> A:\OUT.TXT
A:\LFNTSR /T:0 /H:12 >> A:\OUT.TXT
%LT% mk "No Tunnel.txt" x >> A:\OUT.TXT
%LT% sdel NOTUNN~1.TXT >> A:\OUT.TXT
%LT% screat NOTUNN~1.TXT >> A:\OUT.TXT
%LT% dir "NOT*" >> A:\OUT.TXT
%LT% finds 12 * >> A:\OUT.TXT
%LT% finds 13 * >> A:\OUT.TXT
A:\LFNTSR /U >> A:\OUT.TXT
A:\EXIT
