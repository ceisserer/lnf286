; ===========================================================================
; LT - long file name API test / command line tool
;   LT <command> [args]       (arguments may be quoted with "...")
;
;   md P | rd P | cd P | del P | delw PATTERN | ren A B | pwd [drive]
;   dir PATTERN | mk P TEXT | mknew P TEXT | cat P | full P | short P
;   long P | attr P | setattr P N | gen NAME | vol ROOT | time
;   touch P | finfo P | sub P (long name truename with SUBST flag)
;   plain DOS calls (no 71xxh): sdel P (41h) | sren A B (56h)
;   screat P (3Ch) | snew P (5Bh) | sopen P (6C00h, create only)
;   fdel PATTERN (FCB 13h) | fren A B (FCB 17h)
;   finds N PATTERN: N find-first calls, then find-next on the first handle
;   wait N: wait N seconds
;
; Exit code 0 = ok, 1 = API error (printed as "ERR n"), 2 = usage
; ===========================================================================
        cpu 8086
        org 100h

start:  cld
        ; echo the command line
        mov al,'$'
        call putc
        mov si,81h
.echo:  lodsb
        cmp al,0Dh
        je .echod
        call putc
        jmp short .echo
.echod: call crlf
        mov si,81h
        mov di,argbuf
        xor cx,cx                       ; argc
.tok:   lodsb
        cmp al,' '
        je .tok
        cmp al,9
        je .tok
        cmp al,0Dh
        je .tdone
        mov bx,cx
        shl bx,1
        mov [argv+bx],di
        inc cx
        cmp al,'"'
        je .q
.w:     stosb
        lodsb
        cmp al,' '
        je .end
        cmp al,9
        je .end
        cmp al,0Dh
        je .endl
        jmp short .w
.q:     lodsb
        cmp al,'"'
        je .end
        cmp al,0Dh
        je .endl
        stosb
        jmp short .q
.endl:  dec si
.end:   xor al,al
        stosb
        jmp short .tok
.tdone: mov [argc],cx
        or cx,cx
        jz usage
        ; look up the command
        mov bx,cmds
.cl:    mov si,[bx]
        or si,si
        jz usage
        mov di,[argv]
        call streq
        je .run
        add bx,4
        jmp short .cl
.run:   jmp [bx+2]

usage:  mov dx,msg_usage
        call puts
        mov ax,4C02h
        int 21h

cmds:   dw s_md,c_md, s_rd,c_rd, s_cd,c_cd, s_del,c_del, s_delw,c_delw
        dw s_ren,c_ren, s_pwd,c_pwd, s_dir,c_dir, s_mk,c_mk, s_mknew,c_mknew
        dw s_cat,c_cat, s_full,c_full, s_short,c_short, s_long,c_long
        dw s_attr,c_attr, s_setattr,c_setattr, s_gen,c_gen, s_vol,c_vol
        dw s_time,c_time, s_touch,c_touch, s_finfo,c_finfo, s_sub,c_sub
        dw s_sdel,c_sdel, s_sren,c_sren, s_screat,c_screat, s_snew,c_snew
        dw s_sopen,c_sopen, s_fdel,c_fdel, s_fren,c_fren, s_finds,c_finds
        dw s_wait,c_wait
        dw 0
s_md    db 'md',0
s_rd    db 'rd',0
s_cd    db 'cd',0
s_del   db 'del',0
s_delw  db 'delw',0
s_ren   db 'ren',0
s_pwd   db 'pwd',0
s_dir   db 'dir',0
s_mk    db 'mk',0
s_mknew db 'mknew',0
s_cat   db 'cat',0
s_full  db 'full',0
s_short db 'short',0
s_long  db 'long',0
s_attr  db 'attr',0
s_setattr db 'setattr',0
s_gen   db 'gen',0
s_vol   db 'vol',0
s_time  db 'time',0
s_touch db 'touch',0
s_finfo db 'finfo',0
s_sub   db 'sub',0
s_sdel  db 'sdel',0
s_sren  db 'sren',0
s_screat db 'screat',0
s_snew  db 'snew',0
s_sopen db 'sopen',0
s_fdel  db 'fdel',0
s_fren  db 'fren',0
s_finds db 'finds',0
s_wait  db 'wait',0

; ---------------------------------------------------------------------------
need1:  cmp word [argc],2
        jb usage
        mov dx,[argv+2]
        ret
need2:  cmp word [argc],3
        jb usage
        mov dx,[argv+2]
        ret

c_md:   call need1
        mov ax,7139h
        jmp simple
c_rd:   call need1
        mov ax,713Ah
        jmp simple
c_cd:   call need1
        mov ax,713Bh
        jmp simple
c_del:  call need1
        mov ax,7141h
        xor si,si
        jmp simple
c_delw: call need1
        mov ax,7141h
        mov si,1
        xor cx,cx
        jmp simple
c_ren:  call need2
        mov di,[argv+4]
        mov ax,7156h
        jmp simple

; ---- plain DOS functions
c_sdel: call need1
        mov ah,41h
        jmp short dosc
c_sren: call need2
        push ds
        pop es
        mov di,[argv+4]
        mov ah,56h
dosc:   int 21h
        jc err
        jmp ok

c_screat:
        mov ah,3Ch
        jmp short scr
c_snew: mov ah,5Bh
scr:    push ax
        call need1
        pop ax
        xor cx,cx
        int 21h
        jc err
sclose: mov bx,ax
        mov ah,40h                      ; write something
        mov dx,msg_plain
        mov cx,5
        int 21h
        jc err
        mov ah,3Eh
        int 21h
        jc err
        jmp ok

c_sopen:
        call need1
        mov si,dx
        mov ax,6C00h
        mov bx,2
        xor cx,cx
        mov dx,10h                      ; create, fail if it exists
        int 21h
        jc err
        push ax
        mov dx,msg_act
        call puts
        mov ax,cx
        call putdec
        call crlf
        pop ax
        jmp short sclose

; FCB delete / rename: parse the name(s) with 29h
c_fdel: call need1
        mov si,dx
        mov di,fcb
        call fparse
        mov ah,13h
        jmp short fcall
c_fren: call need2
        mov si,dx
        mov di,fcb
        call fparse
        mov si,[argv+4]
        mov di,fcb+10h
        call fparse
        mov ah,17h
fcall:  mov dx,fcb
        int 21h
        or al,al
        jz ok
        mov ax,0FFh
        jmp err
fparse: push ds
        pop es
        mov ax,2900h
        int 21h
        ret

; finds N PATTERN
c_finds:
        call need2
        mov si,dx
        call atoi
        mov [count],ax
        xor bx,bx
.l:     cmp bx,[count]
        jae .next
        push bx
        mov ax,714Eh
        mov cx,0016h
        mov si,1
        mov dx,[argv+4]
        mov di,fd
        stc
        int 21h
        pop bx
        jc err
        or bx,bx
        jnz .n
        mov [handle],ax
.n:     inc bx
        jmp short .l
.next:  mov ax,714Fh
        mov bx,[handle]
        mov si,1
        mov di,fd
        stc
        int 21h
        jc err
        mov dx,fd+2Ch
        call putz
        call crlf
        jmp ok

c_wait: call need1
        mov si,dx
        call atoi
        mov dx,19                       ; ticks per second (rounded up)
        mul dx
        mov cx,ax
        xor ax,ax
        mov es,ax
        mov bx,[es:046Ch]
.w:     sti
        hlt
        mov ax,[es:046Ch]
        sub ax,bx
        cmp ax,cx
        jb .w
        push ds
        pop es
        jmp ok

simple: stc
        int 21h
        jc err
ok:     mov dx,msg_ok
        call puts
        mov ax,4C00h
        int 21h

err:    push ax
        mov dx,msg_err
        call puts
        pop ax
        call putdec
        call crlf
        mov ax,4C01h
        int 21h

c_pwd:  xor dl,dl
        cmp word [argc],2
        jb .d
        mov si,[argv+2]
        mov dl,[si]
        and dl,1Fh                      ; 'A'/'a' -> 1
.d:     mov si,buf
        mov ax,7147h
        stc
        int 21h
        jc err
        mov al,'\'
        call putc
        mov dx,buf
        call putz
        call crlf
        jmp ok

c_dir:  call need1
        mov ax,714Eh
        mov cx,0016h                    ; hidden, system, directories
        mov si,1                        ; DOS date/time
        mov di,fd
        stc
        int 21h
        jc err
        mov [handle],ax
.show:  ; long name, short name, attributes, size
        mov dx,fd+2Ch
        call putz
        mov dx,msg_bar
        call puts
        mov dx,fd+130h
        call putz
        mov dx,msg_bar
        call puts
        mov al,[fd]
        call puthexb
        mov al,' '
        call putc
        mov ax,[fd+20h]
        call putdec
        call crlf
        mov ax,714Fh
        mov bx,[handle]
        mov si,1
        mov di,fd
        stc
        int 21h
        jnc .show
        cmp ax,18
        jne err
        mov ax,71A1h
        mov bx,[handle]
        stc
        int 21h
        jc err
        jmp ok

c_mk:   mov word [action],12h           ; create or truncate
        jmp short mkc
c_mknew:
        mov word [action],10h           ; create only
mkc:    call need2
        mov si,dx
        mov ax,716Ch
        mov bx,2                        ; read/write
        xor cx,cx
        mov dx,[action]
        stc
        int 21h
        jc err
        mov [handle],ax
        mov [taken],cx
        mov si,[argv+4]
        call strlen
        mov dx,si
        mov bx,[handle]
        mov ah,40h
        int 21h
        jc err
        mov ah,3Eh
        int 21h
        jc err
        mov dx,msg_act
        call puts
        mov ax,[taken]
        call putdec
        call crlf
        jmp ok

c_cat:  call need1
        mov si,dx
        mov ax,716Ch
        xor bx,bx
        xor cx,cx
        mov dx,1
        stc
        int 21h
        jc err
        mov bx,ax
        mov ah,3Fh
        mov cx,200
        mov dx,buf
        int 21h
        jc err
        mov si,ax
        mov byte [buf+si],0
        mov ah,3Eh
        int 21h
        mov dx,buf
        call putz
        call crlf
        jmp ok

c_full: xor cx,cx
        jmp short tn
c_short:
        mov cx,1
        jmp short tn
c_long: mov cx,2
        jmp short tn
c_sub:  mov cx,8002h
tn:     call need1
        mov si,dx
        mov di,buf
        mov ax,7160h
        stc
        int 21h
        jc err
        mov dx,buf
        call putz
        call crlf
        jmp ok

c_attr: call need1
        mov ax,7143h
        mov bl,0
        stc
        int 21h
        jc err
        mov ax,cx
        call puthex
        call crlf
        jmp ok

c_setattr:
        call need2
        mov si,[argv+4]
        call atoi
        mov cx,ax
        mov dx,[argv+2]
        mov ax,7143h
        mov bl,1
        jmp simple

c_gen:  call need1
        mov si,dx
        mov di,buf
        mov dx,0100h
        mov ax,71A8h
        stc
        int 21h
        jc err
        mov dx,buf
        call putz
        call crlf
        mov si,[argv+2]
        mov di,buf
        mov dx,0000h
        mov ax,71A8h
        stc
        int 21h
        jc err
        mov byte [buf+11],0
        mov al,'['
        call putc
        mov dx,buf
        call putz
        mov al,']'
        call putc
        call crlf
        jmp ok

c_vol:  call need1
        mov di,buf
        mov cx,32
        mov ax,71A0h
        stc
        int 21h
        jc err
        push dx
        push cx
        mov ax,bx
        call puthex
        mov al,' '
        call putc
        pop ax
        call putdec
        mov al,' '
        call putc
        pop ax
        call putdec
        mov al,' '
        call putc
        mov dx,buf
        call putz
        call crlf
        jmp ok

c_time: ; 2026-11-05 13:45:58.37 -> FILETIME -> back
        mov dx,(46<<9)|(11<<5)|5
        mov cx,(13<<11)|(45<<5)|29
        mov bh,137
        mov bl,1
        mov di,buf
        mov ax,71A7h
        stc
        int 21h
        jc err
        mov ax,[buf+6]
        call puthex
        mov ax,[buf+4]
        call puthex
        mov ax,[buf+2]
        call puthex
        mov ax,[buf]
        call puthex
        mov al,' '
        call putc
        mov si,buf
        mov bl,0
        mov ax,71A7h
        stc
        int 21h
        jc err
        push cx
        mov ax,dx
        call puthex
        mov al,' '
        call putc
        pop ax
        call puthex
        mov al,' '
        call putc
        mov al,bh
        xor ah,ah
        call putdec
        call crlf
        jmp ok

c_touch:
        call need1
        mov ax,7143h
        mov bl,3
        mov cx,(12<<11)|(34<<5)|28      ; 12:34:56
        mov di,(40<<9)|(2<<5)|29        ; 2020-02-29
        stc
        int 21h
        jc err
        mov dx,[argv+2]
        mov ax,7143h
        mov bl,7
        mov cx,(1<<11)|(2<<5)|3
        mov di,(30<<9)|(6<<5)|15
        mov si,150
        stc
        int 21h
        jc err
        mov dx,[argv+2]
        mov ax,7143h
        mov bl,4
        stc
        int 21h
        jc err
        push cx
        mov ax,di
        call puthex
        mov al,' '
        call putc
        pop ax
        call puthex
        mov al,' '
        call putc
        mov dx,[argv+2]
        mov ax,7143h
        mov bl,8
        stc
        int 21h
        jc err
        push si
        push cx
        mov ax,di
        call puthex
        mov al,' '
        call putc
        pop ax
        call puthex
        mov al,' '
        call putc
        pop ax
        call putdec
        mov al,' '
        call putc
        mov dx,[argv+2]
        mov ax,7143h
        mov bl,2
        stc
        int 21h
        jc err
        call putdec
        call crlf
        jmp ok

c_finfo:
        call need1
        mov si,dx
        mov ax,716Ch
        xor bx,bx
        xor cx,cx
        mov dx,1
        stc
        int 21h
        jc err
        mov [handle],ax
        mov bx,ax
        mov dx,fd
        mov ax,71A6h
        stc
        int 21h
        jc err
        mov bx,[handle]
        mov ah,3Eh
        int 21h
        mov al,[fd]
        call puthexb
        mov al,' '
        call putc
        mov ax,[fd+24h]                 ; size low
        call putdec
        mov al,' '
        call putc
        mov ax,[fd+28h]                 ; number of links
        call putdec
        mov al,' '
        call putc
        mov ax,[fd+14h+6]               ; write FILETIME high word
        call puthex
        mov ax,[fd+14h+4]
        call puthex
        call crlf
        jmp ok

; ---------------------------------------------------------------------------
streq:  push si
        push di
.l:     mov al,[si]
        cmp al,[di]
        jne .x
        inc si
        inc di
        or al,al
        jnz .l
.x:     pop di
        pop si
        ret

strlen: push si
        xor cx,cx
.l:     lodsb
        or al,al
        jz .x
        inc cx
        jmp short .l
.x:     pop si
        ret

atoi:   xor ax,ax
.l:     mov bl,[si]
        sub bl,'0'
        jb .x
        cmp bl,9
        ja .x
        mov cx,10
        mul cx
        xor bh,bh
        add ax,bx
        inc si
        jmp short .l
.x:     ret

puts:   mov ah,9
        int 21h
        ret

putz:   push si
        mov si,dx
.l:     lodsb
        or al,al
        jz .x
        call putc
        jmp short .l
.x:     pop si
        ret

putc:   push ax
        push dx
        mov dl,al
        mov ah,2
        int 21h
        pop dx
        pop ax
        ret

crlf:   push ax
        mov al,13
        call putc
        mov al,10
        call putc
        pop ax
        ret

putdec: push ax
        push bx
        push cx
        push dx
        mov bx,10
        xor cx,cx
.d:     xor dx,dx
        div bx
        push dx
        inc cx
        or ax,ax
        jnz .d
.p:     pop ax
        add al,'0'
        call putc
        loop .p
        pop dx
        pop cx
        pop bx
        pop ax
        ret

puthex: push ax
        mov al,ah
        call puthexb
        pop ax
puthexb:
        push ax
        push cx
        mov cl,4
        mov ah,al
        shr al,cl
        call .n
        mov al,ah
        and al,0Fh
        call .n
        pop cx
        pop ax
        ret
.n:     add al,'0'
        cmp al,'9'
        jbe .p
        add al,7
.p:     jmp putc

msg_usage db 'usage: LT command args',13,10,'$'
msg_ok    db 'OK',13,10,'$'
msg_err   db 'ERR $'
msg_bar   db ' | $'
msg_act   db 'action $'
msg_plain db 'plain'
count   dw 0
fcb     times 40 db 0

argc    dw 0
argv    times 8 dw 0
handle  dw 0
taken   dw 0
action  dw 0
argbuf  times 140 db 0
buf     times 300 db 0
fd      times 320 db 0
