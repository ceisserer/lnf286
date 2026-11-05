@echo off
A:\LFNTSR > A:\OUT.TXT
A:\LFNTSR >> A:\OUT.TXT
SET LT=A:\LT
C:
%LT% vol C:\ >> A:\OUT.TXT
%LT% vol A:\ >> A:\OUT.TXT
%LT% md "C:\Long Directory Name" >> A:\OUT.TXT
%LT% md "C:\long directory name" >> A:\OUT.TXT
%LT% cd "C:\Long Directory Name" >> A:\OUT.TXT
%LT% pwd >> A:\OUT.TXT
%LT% mk "My Document.txt" "hello world" >> A:\OUT.TXT
%LT% cat "my document.TXT" >> A:\OUT.TXT
%LT% mk readme.txt "lower case 8.3" >> A:\OUT.TXT
%LT% mk UPPER.TXT "upper case 8.3" >> A:\OUT.TXT
%LT% mknew "My Document.txt" x >> A:\OUT.TXT
%LT% mk "My Document.txt" "replaced" >> A:\OUT.TXT
%LT% cat MYDOCU~1.TXT >> A:\OUT.TXT
%LT% mk "another long file name with many characters.text" content >> A:\OUT.TXT
for %%i in (1 2 3 4 5 6 7 8 9 10 11) do %LT% mk "Test File %%i.txt" x >> A:\OUT.TXT
%LT% dir * >> A:\OUT.TXT
%LT% dir "*.txt" >> A:\OUT.TXT
%LT% dir "test*" >> A:\OUT.TXT
%LT% dir "*.text" >> A:\OUT.TXT
%LT% dir "nomatch*" >> A:\OUT.TXT
%LT% dir "C:\No Such Dir\*" >> A:\OUT.TXT
%LT% short "My Document.txt" >> A:\OUT.TXT
%LT% long "c:\longdi~1\mydocu~1.txt" >> A:\OUT.TXT
%LT% full "My Document.txt" >> A:\OUT.TXT
%LT% full "Not There Yet.txt" >> A:\OUT.TXT
%LT% short "Not There Yet.txt" >> A:\OUT.TXT
%LT% long C:\ >> A:\OUT.TXT
%LT% ren "My Document.txt" "Renamed Document.txt" >> A:\OUT.TXT
%LT% cat "Renamed Document.txt" >> A:\OUT.TXT
%LT% cat "My Document.txt" >> A:\OUT.TXT
%LT% ren readme.txt ReadMe.txt >> A:\OUT.TXT
%LT% ren UPPER.TXT "Upper Case.Txt" >> A:\OUT.TXT
%LT% ren "Renamed Document.txt" "C:\Moved File.txt" >> A:\OUT.TXT
%LT% ren "Test File 1.txt" "Test File 2.txt" >> A:\OUT.TXT
%LT% attr "Test File 2.txt" >> A:\OUT.TXT
%LT% setattr "Test File 2.txt" 1 >> A:\OUT.TXT
%LT% attr "Test File 2.txt" >> A:\OUT.TXT
%LT% del "Test File 2.txt" >> A:\OUT.TXT
%LT% setattr "Test File 2.txt" 32 >> A:\OUT.TXT
%LT% del "Test File 2.txt" >> A:\OUT.TXT
%LT% del "Test File 2.txt" >> A:\OUT.TXT
%LT% delw "Test File 1*.txt" >> A:\OUT.TXT
%LT% dir * >> A:\OUT.TXT
%LT% touch "Upper Case.Txt" >> A:\OUT.TXT
%LT% finfo "Upper Case.Txt" >> A:\OUT.TXT
%LT% gen "Long File Name.txt" >> A:\OUT.TXT
%LT% gen readme.txt >> A:\OUT.TXT
%LT% gen .bashrc >> A:\OUT.TXT
%LT% gen "a+b=c [1].txt" >> A:\OUT.TXT
%LT% gen x.y.z >> A:\OUT.TXT
%LT% gen "bad*name" >> A:\OUT.TXT
%LT% time >> A:\OUT.TXT
%LT% md "Sub Dir With Spaces" >> A:\OUT.TXT
%LT% mk "C:\Long Directory Name\Sub Dir With Spaces\Deep File.txt" deep >> A:\OUT.TXT
%LT% cd \ >> A:\OUT.TXT
%LT% cd "Long Directory Name/Sub Dir With Spaces" >> A:\OUT.TXT
%LT% pwd >> A:\OUT.TXT
%LT% cat "Deep File.txt" >> A:\OUT.TXT
%LT% cd .. >> A:\OUT.TXT
%LT% pwd >> A:\OUT.TXT
%LT% rd "Sub Dir With Spaces" >> A:\OUT.TXT
%LT% del "Sub Dir With Spaces\Deep File.txt" >> A:\OUT.TXT
%LT% rd "Sub Dir With Spaces" >> A:\OUT.TXT
%LT% md "Directory To Rename" >> A:\OUT.TXT
%LT% mk "Directory To Rename\Inner File.txt" inner >> A:\OUT.TXT
%LT% ren "Directory To Rename" "Renamed Directory" >> A:\OUT.TXT
%LT% cat "Renamed Directory\Inner File.txt" >> A:\OUT.TXT
%LT% mk nul "to the null device" >> A:\OUT.TXT
%LT% mk "C:\Long Directory Name\con.txt" "to the console" >> A:\OUT.TXT
%LT% attr nul >> A:\OUT.TXT
%LT% dir "C:\*" >> A:\OUT.TXT
%LT% dir "C:\Long Directory Name\*e*" >> A:\OUT.TXT
%LT% cat "C:\Moved File.txt" >> A:\OUT.TXT
%LT% mk "Grüße aus Köln.txt" umlaut >> A:\OUT.TXT
%LT% cat "GRÜßE AUS KÖLN.TXT" >> A:\OUT.TXT
%LT% cat "grüße aus köln.txt" >> A:\OUT.TXT
%LT% mk "A file name that is longer than eighty characters to use many directory entries.txt" long >> A:\OUT.TXT
%LT% cat "a file name that is longer than eighty characters to use many directory entries.txt" >> A:\OUT.TXT
%LT% md "C:\Many" >> A:\OUT.TXT
%LT% cd "C:\Many" >> A:\OUT.TXT
for %%i in (1 2 3 4 5 6 7 8 9 10) do %LT% mk "Many Long Named File Number %%i.dat" %%i >> NUL
for %%i in (11 12 13 14 15 16 17 18 19 20) do %LT% mk "Many Long Named File Number %%i.dat" %%i >> NUL
for %%i in (21 22 23 24 25 26 27 28 29 30) do %LT% mk "Many Long Named File Number %%i.dat" %%i >> NUL
%LT% dir "*" >> A:\OUT.TXT
%LT% cat "Many Long Named File Number 30.dat" >> A:\OUT.TXT
%LT% delw "*" >> A:\OUT.TXT
%LT% dir "*" >> A:\OUT.TXT
REM ---- FAT12 floppy
A:
%LT% md "A:\Floppy Directory" >> A:\OUT.TXT
%LT% cd "A:\Floppy Directory" >> A:\OUT.TXT
for %%i in (1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20) do %LT% mk "Floppy Long Name %%i.txt" %%i >> A:\NUL
%LT% dir "*" >> A:\OUT.TXT
%LT% ren "Floppy Long Name 7.txt" "Renamed On Floppy.txt" >> A:\OUT.TXT
%LT% cat "Renamed On Floppy.txt" >> A:\OUT.TXT
%LT% delw "Floppy Long Name 1*" >> A:\OUT.TXT
%LT% dir "*" >> A:\OUT.TXT
%LT% pwd >> A:\OUT.TXT
%LT% pwd C >> A:\OUT.TXT
cd \
%LT% short "A:\Floppy Directory\Renamed On Floppy.txt" >> A:\OUT.TXT
REM ---- uninstall
A:\LFNTSR /U >> A:\OUT.TXT
%LT% vol C:\ >> A:\OUT.TXT
A:\EXIT
