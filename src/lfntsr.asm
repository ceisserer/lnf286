; ===========================================================================
; LFNTSR - Long File Name API for plain DOS
;
; A TSR that implements the Windows 95 long file name API (INT 21h AX=71xxh)
; on FAT12/FAT16 drives by reading and writing VFAT directory entries
; directly (INT 25h/26h).  Drives it cannot handle (network, CD-ROM, FAT32)
; get the same API mapped onto the normal short name DOS calls.
;
; Requires: DOS 3.1+, 8086/8088 CPU.  Assemble with NASM:
;       nasm -f bin -o lfntsr.com lfntsr.asm
;
; Copyright (C) 2026 the lnf286 authors
;
; This program is free software: you can redistribute it and/or modify it
; under the terms of the GNU General Public License as published by the Free
; Software Foundation, either version 3 of the License, or (at your option)
; any later version.  This program is distributed in the hope that it will
; be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
; Public License (file LICENSE) for more details.
; ===========================================================================

        cpu 8086
        bits 16
        org 100h

VERSION equ 0100h               ; 1.00
MAGIC   equ 4C46h               ; 'LF' install check signature
MAXPATH equ 260
NHANDLES equ 8                  ; concurrent find handles
HBASE   equ 1                   ; first find handle number
STACKSZ equ 1536
WPATHSZ equ 600

; find handle structure
H_USED  equ 0
H_MODE  equ 1                   ; 0 = FAT search, 1 = DOS search
H_DRIVE equ 2
H_ALLOW equ 3
H_REQ   equ 4
H_CLUS  equ 6
H_IDX   equ 8
H_LRU   equ 10
H_PAT   equ 12                  ; pattern (FAT) or DTA (DOS)
HSIZE   equ H_PAT + MAXPATH + 1

; DBG 'x' - with -DDEBUG write a character to the Bochs/QEMU debug port E9h
%macro DBG 1
%ifdef DEBUG
        pushf
        push ax
        mov al,%1
        out 0E9h,al
        pop ax
        popf
%endif
%endmacro

start:  jmp install

; ===========================================================================
; Resident part
; ===========================================================================
old21     dd 0
nuldev    dd 0                  ; NUL device header (List of Lists + 22h)
dosmajor  db 0
uptab     times 128 db 0        ; upper case table for 80h..FFh
cptab     times 128 dw 0        ; Unicode for OEM 80h..FFh

; ---------------------------------------------------------------------------
; INT 21h entry
; ---------------------------------------------------------------------------
int21_handler:
        cmp ah,71h
        je .ours
.chain: jmp far [cs:old21]
.ours:  cmp byte [cs:busy],0
        jne .busy
        mov byte [cs:busy],1
        mov [cs:u_ax],ax
        mov [cs:u_bx],bx
        mov [cs:u_cx],cx
        mov [cs:u_dx],dx
        mov [cs:u_si],si
        mov [cs:u_di],di
        mov [cs:u_bp],bp
        mov [cs:u_ds],ds
        mov [cs:u_es],es
        mov [cs:u_ss],ss
        mov [cs:u_sp],sp
        mov ax,cs
        cli
        mov ss,ax
        mov sp,stack_top
        sti
        mov ds,ax
        mov es,ax
        cld
        call int24_set
        call dispatch
        mov byte [res_cf],0
        jnc .ok
        mov [u_ax],ax
        mov byte [res_cf],1
.ok:    call int24_restore
        cli
        mov ss,[u_ss]
        mov sp,[u_sp]
        mov bp,sp
        and word [ss:bp+4],0FFFEh       ; caller's flags: CF
        mov al,[res_cf]
        xor ah,ah
        or [ss:bp+4],ax
        mov ax,[u_ax]
        mov bx,[u_bx]
        mov cx,[u_cx]
        mov dx,[u_dx]
        mov si,[u_si]
        mov di,[u_di]
        mov bp,[u_bp]
        mov es,[u_es]
        mov byte [busy],0
        mov ds,[u_ds]
        iret
.busy:  push bp                         ; re-entered: "not supported"
        mov bp,sp
        or word [bp+6],1
        pop bp
        mov ax,7100h
        iret

; While we work: critical errors always "fail", Ctrl-C/Break is ignored
; (DOS must not abort the program while we are in the middle of a call).
int24_handler:
        mov al,3
int23_handler:
        iret

int24_set:
        push ax
        push es
        xor ax,ax
        mov es,ax
        cli
        mov ax,[es:23h*4]
        mov [sv_int23],ax
        mov ax,[es:23h*4+2]
        mov [sv_int23+2],ax
        mov ax,[es:24h*4]
        mov [sv_int24],ax
        mov ax,[es:24h*4+2]
        mov [sv_int24+2],ax
        mov word [es:23h*4],int23_handler
        mov [es:23h*4+2],cs
        mov word [es:24h*4],int24_handler
        mov [es:24h*4+2],cs
        sti
        pop es
        pop ax
        ret

int24_restore:
        push ax
        push es
        xor ax,ax
        mov es,ax
        cli
        mov ax,[sv_int23]
        mov [es:23h*4],ax
        mov ax,[sv_int23+2]
        mov [es:23h*4+2],ax
        mov ax,[sv_int24]
        mov [es:24h*4],ax
        mov ax,[sv_int24+2]
        mov [es:24h*4+2],ax
        sti
        pop es
        pop ax
        ret

; ---------------------------------------------------------------------------
; is_device - CF if the name SI (base name before the first dot) is the
;   name of a character device (walks the DOS device driver chain)
; ---------------------------------------------------------------------------
is_device:
        push ax
        push bx
        push cx
        push si
        push di
        push es
        mov di,devname
        mov cx,8
        mov al,' '
        rep stosb
        mov di,devname
        mov cx,8
.b:     lodsb
        or al,al
        jz .bd
        cmp al,'.'
        je .bd
        jcxz .no
        call upcase
        stosb
        dec cx
        jmp short .b
.bd:    cmp di,devname
        je .no
        les bx,[nuldev]
        mov ax,200                      ; guard against broken chains
.dl:    test byte [es:bx+5],80h         ; character device?
        jz .nx
        mov si,devname
        lea di,[bx+0Ah]
        mov cx,8
        repe cmpsb
        je .yes
.nx:    les bx,[es:bx]
        cmp bx,0FFFFh
        je .no
        dec ax
        jnz .dl
.no:    clc
        jmp short .x
.yes:   stc
.x:     pop es
        pop di
        pop si
        pop cx
        pop bx
        pop ax
        ret

; ---------------------------------------------------------------------------
; dispatch on AL
; ---------------------------------------------------------------------------
disp_tab:
        db 0Dh
        dw f_reset
        db 39h
        dw f_mkdir
        db 3Ah
        dw f_rmdir
        db 3Bh
        dw f_chdir
        db 41h
        dw f_delete
        db 43h
        dw f_attr
        db 47h
        dw f_getcwd
        db 4Eh
        dw f_findfirst
        db 4Fh
        dw f_findnext
        db 56h
        dw f_rename
        db 60h
        dw f_truename
        db 6Ch
        dw f_open
        db 0A0h
        dw f_volinfo
        db 0A1h
        dw f_findclose
        db 0A6h
        dw f_fileinfo
        db 0A7h
        dw f_timeconv
        db 0A8h
        dw f_genshort
        db 0A9h
        dw f_open
        db 0FFh
        dw f_check
DISP_N  equ ($-disp_tab)/3

dispatch:
        mov al,[u_ax]
        mov si,disp_tab
        mov cx,DISP_N
.l:     cmp al,[si]
        je .hit
        add si,3
        loop .l
        mov ax,7100h
        stc
        ret
.hit:   push word [si+1]
        ; every call starts with fresh sector caches
        mov byte [c_dir+C_VALID],0
        mov byte [c_fat+C_VALID],0
        mov byte [ma_nodir],0
        ret                             ; -> handler

%include "disk.inc"
%include "names.inc"
%include "time.inc"
%include "path.inc"
%include "funcs.inc"

; ---------------------------------------------------------------------------
; go_resident - last step of the installation (runs from resident code
; because it clears the uninitialized data that overlays the installer)
;   DX = paragraphs to keep
; ---------------------------------------------------------------------------
go_resident:
        mov bx,dx
        mov di,bss_start
        mov cx,bss_end-bss_start
        xor al,al
        rep stosb
        mov word [c_dir+C_BUF],dirbuf
        mov word [c_fat+C_BUF],fatbuf
        mov byte [c_fat+C_ISFAT],1
        mov ax,2521h
        mov dx,int21_handler
        int 21h
        mov dx,bx
        mov ax,3100h
        int 21h

resident_end:

; ===========================================================================
; Transient part (installer)
; ===========================================================================
%include "install.inc"
%include "codepage.inc"

transient_end:

; ===========================================================================
; Uninitialized resident data (overlays the installer)
; ===========================================================================
        absolute resident_end
        alignb 2
bss_start:
busy      resb 1
res_cf    resb 1
u_ax      resw 1
u_bx      resw 1
u_cx      resw 1
u_dx      resw 1
u_si      resw 1
u_di      resw 1
u_bp      resw 1
u_ds      resw 1
u_es      resw 1
u_ss      resw 1
u_sp      resw 1
sv_int24  resd 1
sv_int23  resd 1
devname   resb 8
sv_dta    resd 1

; disk layer
io_sec    resd 1
io_buf    resw 1
io_write  resb 1
dir_ioerr resb 1
pkt_sec   resd 1                ; INT 25h/26h packet (keep together)
pkt_cnt   resw 1
pkt_buf   resd 1
c_dir     resb 10               ; cache descriptors
c_fat     resb 10
dv_ok     resb 1
dv_drive  resb 1
dv_fixed  resb 1
dv_ext    resb 1
dv_spc    resb 1
dv_spcshift resb 1
dv_nfats  resb 1
dv_fat16  resb 1
          resb 1
dv_fatstart resw 1
dv_fatsz  resw 1
dv_rootents resw 1
dv_rootstart resw 1
dv_datastart resw 1
dv_maxclust resw 1
dv_freehint resw 1
dv_serial resd 1
d_clus    resw 1
d_cc      resw 1
d_ci      resw 1

; directory scanning
it_idx    resw 1
lfn_start resw 1
lfn_len   resw 1
lfn_next  resb 1
lfn_chk   resb 1
lfn_lossy resb 1
lfn_first resb 1
fmt_flags resb 1
fr_haslfn resb 1
fr_uflags resb 1
p83_lower resb 1
fmt_first resw 1
fr_idx    resw 1
fr_lfnidx resw 1
sfn_idx   resw 1
fr_ent    resb 32
fr_sfn    resb 14
fr_name   resb 262
alias11   resb 12
needlfn   resb 1
ma_lossy  resb 1
ma_nodir  resb 1
          resb 1
ma_start  resw 1
ma_dot    resw 1
ma_baselen resw 1
ma_n      resd 1
ma_tail   resb 8
ma_base   resb 8

; time
acc       resw 4
t_time    resw 1
t_date    resw 1
t_ms      resb 1
t_sec     resb 1
t_min     resb 1
t_hour    resb 1

; path resolution
rf_flags  resb 1
rp_mode   resb 1
rp_isroot resb 1
rp_found  resb 1
rp_isdev  resb 1
rp_ldrive resb 1
rp_pdrive resb 1
rp_src    resw 1
rp_rest   resw 1
rp_srest  resw 1
rp_lrest  resw 1
rp_slast  resw 1
rp_llast  resw 1
rp_parent resw 1
cur_clus  resw 1
ps_end    resw 1
pl_end    resw 1
wptr      resw 1
ncomps    resw 1
minc      resw 1
comps     resw MAXCOMP
tnin      resb 4
tnroot    resb 128
cwdbuf    resb 68
wpath     resb WPATHSZ

; handlers
run_past  resb 1
wl_n      resb 1
wl_chk    resb 1
o_haslfn  resb 1
o_drive   resb 1
del_fn    resb 1
eo_exists resb 1
fi_drive  resb 1
at_fn     resb 1
wl_idx    resw 1
wl_name   resw 1
wl_len    resw 1
al_idx    resw 1
o_parent  resw 1
o_idx     resw 1
o_lfnidx  resw 1
n_parent  resw 1
dw_count  resw 1
dw_err    resw 1
dw_it     resw 1
dw_dirend resw 1
dw_dta    resb 44
op_n      resw 1
op_run    resw 1
vi_size   resw 1
fi_time   resw 1
fi_date   resw 1
fi_svsi   resw 1
fi_size   resd 1
lru_ctr   resw 1
o_name11  resb 12
tmpent    resb 32

inpath    resb MAXPATH+2
inpath2   resb MAXPATH+2
lastname  resb MAXPATH+2
pshort    resb MAXPATH+16
plong     resb MAXPATH+16
outlog    resb MAXPATH+16
oldlog    resb MAXPATH+16
tmpbuf    resb MAXPATH+16
mt_tmp    resb MAXPATH+2

          alignb 2
dirbuf    resb 512
fatbuf    resb 512
handles   resb NHANDLES*HSIZE
          alignb 2
          resb STACKSZ
stack_top:
bss_end:
