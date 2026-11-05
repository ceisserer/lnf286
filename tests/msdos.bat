@echo off
REM MS-DOS specific: SUBST, SHARE, RAMDRIVE (D:), CHKDSK
A:\LFNTSR > A:\OUT.TXT
A:\SHARE > NUL
SET LT=A:\LT
C:
%LT% md "C:\Long Directory Name" >> A:\OUT.TXT
%LT% mk "C:\Long Directory Name\File In Subst.txt" subst >> A:\OUT.TXT
A:\SUBST E: C:\LONGDI~1
%LT% vol E:\ >> A:\OUT.TXT
%LT% dir "E:\*" >> A:\OUT.TXT
%LT% cat "E:\File In Subst.txt" >> A:\OUT.TXT
%LT% md "E:\Dir On Subst" >> A:\OUT.TXT
%LT% cd "E:\Dir On Subst" >> A:\OUT.TXT
%LT% pwd E >> A:\OUT.TXT
E:
%LT% mk "Relative On Subst.txt" rel >> A:\OUT.TXT
%LT% cd .. >> A:\OUT.TXT
%LT% pwd E >> A:\OUT.TXT
%LT% cd .. >> A:\OUT.TXT
%LT% pwd E >> A:\OUT.TXT
%LT% short "E:\Dir On Subst\Relative On Subst.txt" >> A:\OUT.TXT
%LT% long "E:\DIRONS~1\RELATI~1.TXT" >> A:\OUT.TXT
%LT% sub "E:\DIRONS~1\RELATI~1.TXT" >> A:\OUT.TXT
%LT% full "E:\Dir On Subst\New.txt" >> A:\OUT.TXT
%LT% ren "E:\File In Subst.txt" "E:\Renamed In Subst.txt" >> A:\OUT.TXT
%LT% dir "C:\Long Directory Name\*" >> A:\OUT.TXT
%LT% dir "C:\Long Directory Name\Dir On Subst\*" >> A:\OUT.TXT
C:
A:\SUBST E: /D
%LT% vol D:\ >> A:\OUT.TXT
%LT% md "D:\Ram Disk Directory" >> A:\OUT.TXT
for %%i in (1 2 3 4 5 6 7 8 9 10) do %LT% mk "D:\Ram Disk Directory\Ram File %%i.txt" %%i >> NUL
%LT% ren "D:\Ram Disk Directory\Ram File 5.txt" "D:\Ram Disk Directory\Fifth.txt" >> A:\OUT.TXT
%LT% dir "D:\Ram Disk Directory\*" >> A:\OUT.TXT
%LT% delw "D:\Ram Disk Directory\Ram*" >> A:\OUT.TXT
%LT% dir "D:\Ram Disk Directory\*" >> A:\OUT.TXT
%LT% del "D:\Ram Disk Directory\Fifth.txt" >> A:\OUT.TXT
%LT% rd "D:\Ram Disk Directory" >> A:\OUT.TXT
%LT% dir "D:\*" >> A:\OUT.TXT
A:\CHKDSK C: >> A:\OUT.TXT
A:\CHKDSK D: >> A:\OUT.TXT
A:\LFNTSR /U >> A:\OUT.TXT
A:\EXIT
