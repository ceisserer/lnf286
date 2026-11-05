; ===========================================================================
; install.inc - transient part: command line, checks, install / uninstall
; ===========================================================================

install:
        cld
        mov dx,msg_banner
        call print
        ; DOS version >= 3.10
        mov ax,3000h
        int 21h
        cmp al,3
        jb .oldv
        ja .verok
        cmp ah,10
        jae .verok
.oldv:  mov dx,msg_dosver
        jmp fatal
.verok: mov [dosmajor],al

        ; command line
        mov si,81h
.arg:   lodsb
        cmp al,0Dh
        je .argd
        cmp al,'/'
        je .opt
        cmp al,'-'
        je .opt
        cmp al,' '
        je .arg
        cmp al,9
        je .arg
        jmp .usage
.opt:   lodsb
        call upcase
        cmp al,'U'
        je .optu
        cmp al,'F'
        je .optf
        cmp al,'Q'
        je .optq
        cmp al,'?'
        je .usage
        cmp al,'H'
        je .usage
        jmp .usage
.optu:  mov byte [opt_u],1
        jmp short .arg
.optf:  mov byte [opt_f],1
        jmp short .arg
.optq:  mov byte [opt_q],1
        jmp short .arg
.usage: mov dx,msg_usage
        call print
        mov ax,4C00h
        int 21h
.argd:
        ; already installed?
        mov ax,71FFh
        mov bx,MAGIC
        stc
        int 21h
        jc .notinst
        or ax,ax
        jnz .notinst
        mov [res_seg],bx
        cmp byte [opt_u],0
        jne uninstall
        mov dx,msg_already
        jmp fatal
.notinst:
        cmp byte [opt_u],0
        je .inst
        mov dx,msg_notinst
        jmp fatal
.inst:  ; another long file name API present (Windows, DOSLFN, ...)?
        cmp byte [opt_f],0
        jne .noapi
        mov ax,7147h
        xor dl,dl
        mov si,probe_buf
        stc
        int 21h
        cmp ax,7100h
        je .noapi
        mov dx,msg_haveapi
        jmp fatal
.noapi:
        ; upper case table of the current country
        mov ax,6502h
        mov bx,0FFFFh
        mov dx,0FFFFh
        mov cx,5
        mov di,probe_buf
        int 21h
        jc .defup
        cmp byte [probe_buf],2
        jne .defup
        push ds
        lds si,[probe_buf+1]
        lodsw                           ; table size
        cmp ax,128
        jne .defup1
        mov di,uptab
        mov cx,128
        rep movsb
        pop ds
        jmp short .cp
.defup1:
        pop ds
.defup: mov di,uptab                    ; identity
        mov al,80h
        mov cx,128
.du:    stosb
        inc al
        loop .du

.cp:    ; active code page
        mov word [codepage],437
        mov ax,6601h
        int 21h
        jc .cpdef
        mov [codepage],bx
.cpdef: mov si,cp_tables
.cpl:   lodsw
        or ax,ax
        jz .cpnf
        cmp ax,[codepage]
        je .cpf
        add si,256
        jmp short .cpl
.cpnf:  mov dx,msg_cpunk
        call print_q
        mov ax,[codepage]
        call print_dec_q
        mov dx,msg_cpuse
        call print_q
        mov si,cp_tables+2              ; first table = 437
        jmp short .cpcopy
.cpf:   mov dx,msg_cp
        call print_q
        mov ax,[codepage]
        call print_dec_q
        mov dx,msg_crlf
        call print_q
.cpcopy:
        mov di,cptab
        mov cx,128
        rep movsw

        ; device driver chain (NUL device in the List of Lists)
        mov ah,52h
        int 21h
        add bx,22h
        mov [nuldev],bx
        mov [nuldev+2],es
        push cs
        pop es
        ; old INT 21h vector
        mov ax,3521h
        int 21h
        mov [old21],bx
        mov [old21+2],es
        push cs
        pop es
        ; free the environment
        mov es,[2Ch]
        mov ah,49h
        int 21h
        push cs
        pop es
        mov word [2Ch],0
        mov dx,msg_inst
        call print_q
        mov ax,bss_end+15
        mov cl,4
        shr ax,cl
        call print_kb_q
        mov dx,ax
        jmp go_resident

; ---------------------------------------------------------------------------
uninstall:
        mov ax,3521h
        int 21h
        mov ax,es
        push cs
        pop es
        cmp ax,[res_seg]
        jne .hooked
        cmp bx,int21_handler
        jne .hooked
        push ds
        mov ds,[res_seg]
        mov dx,[old21]
        mov ax,[old21+2]
        mov ds,ax
        mov ax,2521h
        int 21h
        pop ds
        mov es,[res_seg]
        mov ah,49h
        int 21h
        push cs
        pop es
        mov dx,msg_removed
        call print
        mov ax,4C00h
        int 21h
.hooked:
        mov dx,msg_hooked
        jmp fatal

; ---------------------------------------------------------------------------
fatal:  call print
        mov ax,4C01h
        int 21h

print_q:
        cmp byte [opt_q],0
        jne prt_ret
print:  push ax
        mov ah,9
        int 21h
        pop ax
prt_ret:
        ret

; print AX as decimal (quiet aware)
print_dec_q:
        cmp byte [opt_q],0
        jne prt_ret
        push ax
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
.p:     pop dx
        add dl,'0'
        mov ah,2
        int 21h
        loop .p
        pop dx
        pop cx
        pop bx
        pop ax
        ret

; print_kb_q - AX = paragraphs -> " nnnnn bytes resident"
print_kb_q:
        push ax
        push dx
        mov dx,16
        mul dx
        call print_dec_q
        mov dx,msg_bytes
        call print_q
        pop dx
        pop ax
        ret

msg_banner  db 'LFNTSR 1.00 - long file name API for DOS (FAT12/FAT16)',13,10,'$'
msg_dosver  db 'DOS 3.10 or newer required.',13,10,'$'
msg_usage   db 'Usage: LFNTSR [/U] [/F] [/Q]',13,10
            db '  /U  uninstall',13,10
            db '  /F  install even if a long file name API is present',13,10
            db '  /Q  quiet',13,10,'$'
msg_already db 'Already installed.',13,10,'$'
msg_notinst db 'Not installed.',13,10,'$'
msg_haveapi db 'A long file name API is already present (use /F to override).',13,10,'$'
msg_cp      db 'Code page $'
msg_cpunk   db 'No table for code page $'
msg_cpuse   db ', using 437.',13,10,'$'
msg_crlf    db 13,10,'$'
msg_inst    db 'Installed, $'
msg_bytes   db ' bytes resident.',13,10,'$'
msg_removed db 'Removed from memory.',13,10,'$'
msg_hooked  db 'Cannot uninstall: INT 21h was hooked by a later program.',13,10,'$'

opt_u     db 0
opt_f     db 0
opt_q     db 0
res_seg   dw 0
codepage  dw 0
probe_buf times 264 db 0
