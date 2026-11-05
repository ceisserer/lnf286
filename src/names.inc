; ===========================================================================
; names.inc - string helpers, code page / Unicode conversion, directory
; scanning with long name assembly, short name (alias) generation, wildcards
; ===========================================================================

; ---------------------------------------------------------------------------
; upcase - AL -> upper case AL (ASCII + DOS country table for 80h-FFh)
; ---------------------------------------------------------------------------
upcase:
        cmp al,'a'
        jb .x
        cmp al,'z'
        ja .hi
        sub al,20h
.x:     ret
.hi:    cmp al,80h
        jb .x
        push bx
        mov bx,uptab-80h
        xlat
        pop bx
        ret

; strlen - SI -> CX length (excluding NUL)
strlen:
        push si
        xor cx,cx
.l:     lodsb
        or al,al
        jz .e
        inc cx
        jmp short .l
.e:     pop si
        ret

; strcpy - copy NUL terminated SI to DI; DI points at the NUL afterwards
strcpy:
        push ax
        push si
.l:     lodsb
        stosb
        or al,al
        jnz .l
        dec di
        pop si
        pop ax
        ret

; stricmp - compare SI and DI case-insensitively.  ZF=1 if equal.
stricmp:
        push ax
        push bx
        push si
        push di
.l:     lodsb
        call upcase
        mov bl,al
        mov al,[di]
        inc di
        call upcase
        cmp al,bl
        jne .x
        or al,al
        jnz .l
.x:     pop di
        pop si
        pop bx
        pop ax
        ret

; ---------------------------------------------------------------------------
; oem2uni - AL = OEM char -> AX = Unicode
; uni2oem - AX = Unicode -> AL = OEM char, CF if not representable
; ---------------------------------------------------------------------------
oem2uni:
        cmp al,80h
        jae .tab
        xor ah,ah
        ret
.tab:   push bx
        mov bl,al
        xor bh,bh
        shl bx,1
        mov ax,[cptab-100h+bx]
        pop bx
        or ax,ax
        jnz .r
        mov ax,'_'
.r:     ret

uni2oem:
        cmp ax,80h
        jae .srch
        clc
        ret
.srch:  push cx
        push di
        mov di,cptab
        mov cx,128
        repne scasw
        jne .no
        sub di,cptab+2
        shr di,1
        mov ax,di
        add al,80h
        clc
        jmp short .r
.no:    mov al,'?'
        stc
.r:     pop di
        pop cx
        ret

; ---------------------------------------------------------------------------
; sfn_checksum - SI = 11 byte name -> AL = LFN checksum
; ---------------------------------------------------------------------------
sfn_checksum:
        push cx
        push si
        xor al,al
        mov cx,11
.l:     ror al,1
        add al,[si]
        inc si
        loop .l
        pop si
        pop cx
        ret

; ---------------------------------------------------------------------------
; fmt_sfn - format 11-byte directory name at SI as "NAME.EXT" at DI
;   BL = NT case flags (byte 0Ch of the entry; 08h base lower, 10h ext lower)
;   DI points at the terminating NUL afterwards.
; ---------------------------------------------------------------------------
fmt_sfn:
        push ax
        push cx
        push dx
        push si
        ; length of base without trailing blanks
        mov cx,8
.bl:    mov bx,cx
        cmp byte [si+bx-1],' '
        jne .bd
        loop .bl
.bd:    mov dl,08h
        mov bh,0
        call .copy
        add si,8
        mov cx,3
.el:    mov bx,cx
        cmp byte [si+bx-1],' '
        jne .ed
        loop .el
.ed:    jcxz .done
        mov al,'.'
        stosb
        mov dl,10h
        call .copy
.done:  mov byte [di],0
        pop si
        pop dx
        pop cx
        pop ax
        ret
; copy CX chars from SI (SI not advanced), lower-casing if flag DL set
.copy:  jcxz .cr
        push cx
        push si
        mov bl,[fmt_flags]
.cl:    lodsb
        cmp si,[fmt_first]
        jne .n05
        cmp al,05h
        jne .n05
        mov al,0E5h
.n05:   test bl,dl
        jz .st
        cmp al,'A'
        jb .st
        cmp al,'Z'
        ja .st
        add al,20h
.st:    stosb
        loop .cl
        pop si
        pop cx
.cr:    ret

; fmt_sfn_entry - SI = dir entry, DI = output; uses the entry's case flags
fmt_sfn_entry:
        push ax
        mov al,[si+0Ch]
        mov [fmt_flags],al
        lea ax,[si+1]
        mov [fmt_first],ax
        push bx
        call fmt_sfn
        pop bx
        pop ax
        ret

; fmt_name11 - SI = 11 byte name (no case flags), DI = output
fmt_name11:
        mov byte [fmt_flags],0
        mov word [fmt_first],0
        push bx
        call fmt_sfn
        pop bx
        ret

; ---------------------------------------------------------------------------
; dir_rewind - start scanning the current directory at entry 0
; dir_next   - return the next file entry of the current directory
;   out: CF=1 no more entries (check [dir_ioerr] for I/O errors)
;        CF=0: fr_name (long or short name), fr_sfn ("NAME.EXT"),
;              fr_ent (copy of the 32-byte entry), fr_idx, fr_lfnidx,
;              fr_haslfn, fr_uflags
; ---------------------------------------------------------------------------
dir_rewind:
        mov word [it_idx],0
        ret

LFNORD  db 1,3,5,7,9,14,16,18,20,22,24,28,30

dir_next:
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        mov byte [lfn_next],0FFh
.loop:  mov ax,[it_idx]
        call dir_ptr
        jc .eod
        mov al,[bx]
        or al,al
        jz .eod
        inc word [it_idx]
        cmp al,0E5h
        je .reset
        cmp byte [bx+0Bh],0Fh
        je .lfn
        jmp .sfn
.reset: mov byte [lfn_next],0FFh
        jmp short .loop
.eod:   stc
        jmp .ret

        ; ---- long name fragment
.lfn:   mov cl,al
        and cl,1Fh                      ; sequence number
        jz .reset
        test al,40h
        jz .cont
        cmp cl,20
        ja .reset
        mov [lfn_next],cl
        mov al,[bx+0Dh]
        mov [lfn_chk],al
        mov ax,[it_idx]
        dec ax
        mov [lfn_start],ax
        mov byte [lfn_lossy],0
        ; default length: seq*13 (no terminator)
        mov al,13
        mul cl
        mov [lfn_len],ax
        mov byte [lfn_first],1
        jmp short .frag
.cont:  cmp cl,[lfn_next]
        jne .reset
        mov al,[bx+0Dh]
        cmp al,[lfn_chk]
        jne .reset
        mov byte [lfn_first],0
.frag:  ; CL = sequence number, BX = entry
        dec cl
        mov al,13
        mul cl
        mov di,ax                       ; position of first char
        mov si,LFNORD
        mov cx,13
.ch:    push bx
        mov al,[si]
        xor ah,ah
        add bx,ax
        mov ax,[bx]
        pop bx
        inc si
        or ax,ax
        jz .term
        cmp ax,0FFFFh
        je .nxt
        call uni2oem
        jnc .stc
        mov byte [lfn_lossy],1
.stc:   cmp di,255
        jae .nxt
        mov [fr_name+di],al
.nxt:   inc di
        loop .ch
        jmp short .fragd
.term:  cmp byte [lfn_first],0
        je .fragd
        mov [lfn_len],di
.fragd: mov al,[lfn_next]
        dec al
        mov [lfn_next],al
        jmp .loop

        ; ---- short entry
.sfn:   mov si,bx
        mov di,fr_ent
        mov cx,16
        rep movsw
        mov ax,[it_idx]
        dec ax
        mov [fr_idx],ax
        mov si,fr_ent
        mov di,fr_sfn
        call fmt_sfn_entry
        mov byte [fr_haslfn],0
        mov byte [fr_uflags],0
        mov [fr_lfnidx],ax
        cmp byte [lfn_next],0
        jne .nolfn
        call sfn_checksum               ; SI = fr_ent
        cmp al,[lfn_chk]
        jne .nolfn
        mov bx,[lfn_len]
        cmp bx,255
        ja .nolfn
        or bx,bx
        jz .nolfn
        mov byte [fr_name+bx],0
        mov byte [fr_haslfn],1
        mov al,[lfn_lossy]
        mov [fr_uflags],al
        mov ax,[lfn_start]
        mov [fr_lfnidx],ax
        jmp short .ok
.nolfn: mov si,fr_sfn
        mov di,fr_name
        call strcpy
.ok:    mov byte [lfn_next],0FFh
        clc
.ret:   pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        ret

; ---------------------------------------------------------------------------
; dir_find - search the current directory for name SI (long or short,
;   case-insensitive, no wildcards).  Volume labels are ignored.
;   out: CF=0 found (fr_* valid), CF=1 not found
; ---------------------------------------------------------------------------
dir_find:
        push di
        call dir_rewind
.l:     call dir_next
        jc .x
        mov al,[fr_ent+0Bh]
        test al,08h
        jnz .l
        mov di,fr_name
        call stricmp
        je .found
        mov di,fr_sfn
        call stricmp
        jne .l
.found: clc
.x:     pop di
        ret

; ---------------------------------------------------------------------------
; sfn_exists - is the 11 byte name at SI used in the current directory?
;   out: CF=1 yes (or error), CF=0 free
; ---------------------------------------------------------------------------
sfn_exists:
        push ax
        push bx
        push cx
        push si
        push di
        mov byte [dir_ioerr],0
        cmp byte [ma_nodir],0
        jne .free
        xor ax,ax
.l:     call dir_ptr
        jc .free
        cmp byte [bx],0
        je .free
        cmp byte [bx],0E5h
        je .n
        cmp byte [bx+0Bh],0Fh
        je .n
        test byte [bx+0Bh],08h
        jnz .n
        push si
        mov di,bx
        mov cx,11
        repe cmpsb
        pop si
        je .hit
.n:     inc ax
        jmp short .l
.free:  cmp byte [dir_ioerr],0
        jne .used
        clc
        jmp short .x
.hit:   mov [sfn_idx],ax
.used:  stc
.x:     pop di
        pop si
        pop cx
        pop bx
        pop ax
        ret

; ---------------------------------------------------------------------------
; Character classes
;   bad_lfn_char - CF if AL may not appear in a long name
;   bad_sfn_char - CF if AL may not appear in a short name
; ---------------------------------------------------------------------------
bad_lfn_char:
        cmp al,20h
        jb .bad
        push di
        push cx
        mov di,lfn_bad
        mov cx,lfn_bad_n
        repne scasb
        pop cx
        pop di
        je .bad
        clc
        ret
.bad:   stc
        ret

bad_sfn_char:
        call bad_lfn_char
        jc .r
        cmp al,' '
        je .bad
        push di
        push cx
        mov di,sfn_bad
        mov cx,sfn_bad_n
        repne scasb
        pop cx
        pop di
        je .bad
        clc
.r:     ret
.bad:   stc
        ret

lfn_bad db '"*/:<>?\|'
lfn_bad_n equ $-lfn_bad
sfn_bad db '+,;=[].'
sfn_bad_n equ $-sfn_bad

; ---------------------------------------------------------------------------
; check_lfn - validate long name SI.  CF if invalid (empty, too long,
;   illegal characters, "." or "..").
; ---------------------------------------------------------------------------
check_lfn:
        push ax
        push cx
        push si
        call strlen
        jcxz .bad
        cmp cx,255
        ja .bad
        cmp byte [si],'.'
        jne .chars
        cmp cx,2
        ja .chars
        cmp byte [si+1],0
        je .bad
        cmp byte [si+1],'.'
        je .bad
.chars: lodsb
        or al,al
        jz .ok
        call bad_lfn_char
        jc .bad
        jmp short .chars
.ok:    clc
        jmp short .x
.bad:   stc
.x:     pop si
        pop cx
        pop ax
        ret

; ---------------------------------------------------------------------------
; parse83 - try to interpret SI as a plain 8.3 name.
;   out: CF=0 ok, alias11 = upper-case 11 byte name, AL=1 if the name
;        contains lower case letters (then a long name entry is wanted)
;        CF=1 not an 8.3 name
; ---------------------------------------------------------------------------
parse83:
        push bx
        push cx
        push si
        push di
        mov di,alias11
        mov cx,11
        mov al,' '
        rep stosb
        mov byte [p83_lower],0
        mov di,alias11
        mov cx,8                        ; room left in this part
        mov bl,0                        ; 0 = base, 1 = extension
        cmp byte [si],'.'
        je .bad
.l:     lodsb
        or al,al
        jz .end
        cmp al,'.'
        je .dot
        call bad_sfn_char
        jc .bad
        jcxz .bad
        mov ah,al
        call upcase
        cmp al,ah
        je .st
        mov byte [p83_lower],1
.st:    cmp di,alias11
        jne .st2
        cmp al,0E5h
        jne .st2
        mov al,05h
.st2:   stosb
        dec cx
        jmp short .l
.dot:   or bl,bl
        jnz .bad
        cmp di,alias11
        je .bad
        mov bl,1
        mov di,alias11+8
        mov cx,3
        cmp byte [si],0                 ; "NAME." is not a plain name
        je .bad
        jmp short .l
.end:   cmp di,alias11
        je .bad
        mov al,[p83_lower]
        clc
        jmp short .x
.bad:   stc
.x:     pop di
        pop si
        pop cx
        pop bx
        ret

; ---------------------------------------------------------------------------
; make_alias - choose the short name for new long name SI in the current
;   directory.  out: alias11, [needlfn] (1 = long name entries needed),
;   CF on error (no free alias / I/O)
; ---------------------------------------------------------------------------
make_alias:
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        call parse83
        jc .gen
        mov [needlfn],al
        clc
        jmp .x
.gen:   mov byte [needlfn],1
        mov byte [ma_lossy],0
        ; find the last dot (ignoring leading dots)
        mov bx,si
.ld:    cmp byte [bx],'.'
        jne .ldd
        inc bx
        mov byte [ma_lossy],1
        jmp short .ld
.ldd:   mov [ma_start],bx
        xor dx,dx                       ; DX = last dot
.fd:    mov al,[bx]
        or al,al
        jz .fdd
        cmp al,'.'
        jne .fdn
        mov dx,bx
.fdn:   inc bx
        jmp short .fd
.fdd:   or dx,dx
        jnz .hasd
        mov dx,bx                       ; no extension: "dot" at the end
.hasd:  mov [ma_dot],dx
        ; fill
        mov di,alias11
        mov cx,11
        mov al,' '
        rep stosb
        ; base part
        mov si,[ma_start]
        mov di,alias11
        mov cx,8
.bp:    cmp si,[ma_dot]
        jae .bpd
        lodsb
        call ma_char
        jc .bp
        jcxz .bptr
        stosb
        dec cx
        jmp short .bp
.bptr:  mov byte [ma_lossy],1           ; truncated
        jmp short .bp
.bpd:   mov [ma_baselen],di
        sub word [ma_baselen],alias11
        ; extension
        mov si,[ma_dot]
        cmp byte [si],0
        je .exd
        inc si
        mov di,alias11+8
        mov cx,3
.ep:    lodsb
        or al,al
        jz .exd
        call ma_char
        jc .ep
        jcxz .eptr
        stosb
        dec cx
        jmp short .ep
.eptr:  mov byte [ma_lossy],1
        jmp short .ep
.exd:   cmp word [ma_baselen],0
        jne .hb
        mov byte [alias11],'_'          ; nothing usable left for the base
        mov word [ma_baselen],1
        mov byte [ma_lossy],1
.hb:    cmp byte [alias11],0E5h
        jne .ne5
        mov byte [alias11],05h
.ne5:   cmp byte [ma_lossy],0
        jne .tail
        mov si,alias11
        call sfn_exists
        jnc .ok
        cmp byte [dir_ioerr],0
        jne .err
.tail:  ; try NAME~1, NAME~2, ...
        push si
        push di
        push cx
        mov si,alias11
        mov di,ma_base
        mov cx,8
        rep movsb
        pop cx
        pop di
        pop si
        mov word [ma_n],1
        mov word [ma_n+2],0
.tl:    ; render "~n" so that it ends at ma_tail+7
        mov ax,[ma_n]
        mov dx,[ma_n+2]
        mov di,ma_tail+7
        mov byte [di],0
        mov cx,10
.dg:    push ax                         ; DX:AX / 10
        mov ax,dx
        xor dx,dx
        div cx
        mov bx,ax                       ; high quotient
        pop ax
        div cx                          ; DX = remainder
        add dl,'0'
        dec di
        mov [di],dl
        mov dx,bx
        mov bx,ax
        or bx,dx
        jnz .dg
        dec di
        mov byte [di],'~'
        call ma_rebuild
        mov si,alias11
        call sfn_exists
        jnc .ok
        cmp byte [dir_ioerr],0
        jne .err
        add word [ma_n],1
        adc word [ma_n+2],0
        cmp word [ma_n],0                 ; stop after ~65535
        jne .tl
.err:   stc
        jmp short .x
.ok:    clc
.x:     pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        ret

; ma_char - convert AL for use in a short name; CF = skip the character
;   (spaces and dots are dropped, illegal characters become '_')
ma_char:
        cmp al,' '
        je .skip
        cmp al,'.'
        je .skip
        mov ah,al
        call upcase
        call bad_sfn_char
        jnc .ok
        mov al,'_'
        mov byte [ma_lossy],1
.ok:    clc
        ret
.skip:  mov byte [ma_lossy],1
        stc
        ret

; ma_rebuild - alias11[0..7] = first k chars of the generated base + "~n"
;   where "~n" is the NUL terminated string ending at ma_tail+7 and
;   k = min(baselen, 8 - len(tail)).  The base chars are kept in ma_base.
ma_rebuild:
        push ax
        push cx
        push si
        push di
        ; locate tail
        mov si,ma_tail+7
.f:     dec si
        cmp byte [si],'~'
        jne .f
        mov cx,ma_tail+7
        sub cx,si                       ; tail length
        mov ax,8
        sub ax,cx
        cmp ax,[ma_baselen]
        jbe .k
        mov ax,[ma_baselen]
.k:     push si
        push cx
        mov si,ma_base
        mov di,alias11
        mov cx,ax
        rep movsb
        pop cx
        pop si
        rep movsb
        mov cx,alias11+8
        sub cx,di
        mov al,' '
        rep stosb
        pop di
        pop si
        pop cx
        pop ax
        ret

; ---------------------------------------------------------------------------
; wmatch - match pattern SI against name DI ('*' and '?', case-insensitive)
;   out: ZF=1 match
; ---------------------------------------------------------------------------
wmatch:
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        xor dx,dx                       ; DX = pattern after last '*'
.l:     mov al,[di]
        or al,al
        jz .end
        mov ah,[si]
        cmp ah,'*'
        jne .nst
        inc si
        mov dx,si
        mov bx,di                       ; BX = name position for the star
        jmp short .l
.nst:   cmp ah,'?'
        je .adv
        call upcase
        mov cl,al
        mov al,ah
        call upcase
        cmp al,cl
        jne .back
        or al,al
        jz .back
.adv:   inc si
        inc di
        jmp short .l
.back:  or dx,dx
        jz .fail
        mov si,dx
        inc bx
        mov di,bx
        jmp short .l
.end:   cmp byte [si],'*'
        jne .e2
        inc si
        jmp short .end
.e2:    cmp byte [si],0
        jmp short .x
.fail:  or sp,sp                        ; ZF=0 (SP is never 0)
.x:     pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        ret

; ---------------------------------------------------------------------------
; match_entry - does pattern SI match the current entry (fr_name/fr_sfn)?
;   Implements the usual DOS/Windows rules: "*.*" matches everything,
;   "name.*" also matches "name", a trailing '.' means "no extension".
;   out: ZF=1 match
; ---------------------------------------------------------------------------
match_entry:
        push ax
        push cx
        push si
        push di
        mov di,fr_name
        call wmatch
        je .x
        mov di,fr_sfn
        call wmatch
        je .x
        ; pattern variants
        call strlen
        cmp cx,2
        jb .no
        mov di,si
        add di,cx
        cmp byte [di-1],'.'
        je .tdot
        cmp word [di-2],'.*'
        jne .no
        ; "xxx.*" -> try "xxx" too
        push si
        mov di,mt_tmp
        call strcpy
        pop si
        mov di,mt_tmp
        add di,cx
        mov byte [di-2],0
        mov si,mt_tmp
        mov di,fr_name
        call wmatch
        je .x
        mov di,fr_sfn
        call wmatch
        jmp short .x
.tdot:  ; "xxx." -> names without extension matching "xxx"
        push si
        mov di,mt_tmp
        call strcpy
        pop si
        mov di,mt_tmp
        add di,cx
        mov byte [di-1],0
        mov si,fr_sfn
.nd:    lodsb
        or al,al
        jz .nodot
        cmp al,'.'
        je .no
        jmp short .nd
.nodot: mov si,mt_tmp
        mov di,fr_sfn
        call wmatch
        je .x
        mov di,fr_name
        call wmatch
        jmp short .x
.no:    or sp,sp
.x:     pop di
        pop si
        pop cx
        pop ax
        ret

; has_wild - CF if SI contains '*' or '?'
has_wild:
        push ax
        push si
.l:     lodsb
        or al,al
        jz .no
        cmp al,'*'
        je .y
        cmp al,'?'
        jne .l
.y:     stc
        jmp short .x
.no:    clc
.x:     pop si
        pop ax
        ret
