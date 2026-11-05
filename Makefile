# LFNTSR - build with NASM (https://www.nasm.us)
NASM ?= nasm

all: lfntsr.com lt.com

lfntsr.com: src/*.asm src/*.inc
	$(NASM) -f bin -I src/ -o $@ src/lfntsr.asm

lt.com: tests/lt.asm
	$(NASM) -f bin -o $@ tests/lt.asm

# regenerate the code page tables (needs Python 3)
codepages:
	python3 tools/gencp.py src/codepage.inc

# run the FreeDOS/QEMU test suite (needs qemu-system-i386, mtools, dosfstools)
check:
	tests/check.sh

clean:
	rm -f lfntsr.com lt.com *.lst

.PHONY: all codepages check clean
