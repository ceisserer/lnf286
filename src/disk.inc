; ===========================================================================
; disk.inc - drive parameters, sector cache, FAT access, directory access
; All routines expect DS = ES = CS.  8086 instruction set only.
; ===========================================================================

; ---------------------------------------------------------------------------
; Sector cache descriptors
;   +0 valid(b) +1 dirty(b) +2 sector(dd) +6 buffer(dw) +8 isfat(b)
; For the FAT cache the sector is relative to the start of the first FAT;
; dirty FAT sectors are written to every FAT copy.
; ---------------------------------------------------------------------------
C_VALID equ 0
C_DIRTY equ 1
C_SEC   equ 2
C_BUF   equ 6
C_ISFAT equ 8

; ---------------------------------------------------------------------------
; disk_io - read/write one sector of drive [dv_drive]
;   in : [io_sec] dword absolute sector, [io_buf] buffer, [io_write] 0/1
;   out: CF set on error
; ---------------------------------------------------------------------------
disk_io:
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        push bp
        push ds
        push es
        cmp byte [dv_ext],0
        jne .ext
        cmp word [io_sec+2],0           ; small partition: classic call
        jne .ioerr
        mov al,[dv_drive]
        mov cx,1
        mov dx,[io_sec]
        mov bx,[io_buf]
        jmp short .go
.ext:   mov ax,[io_sec]
        mov [pkt_sec],ax
        mov ax,[io_sec+2]
        mov [pkt_sec+2],ax
        mov word [pkt_cnt],1
        mov ax,[io_buf]
        mov [pkt_buf],ax
        mov [pkt_buf+2],cs
        mov al,[dv_drive]
        mov cx,0FFFFh
        mov bx,pkt_sec
.go:    DBG 'd'
        cmp byte [io_write],0
        jne .wr
        int 25h
        jmp short .done
.wr:    int 26h
.done:  pop bx                          ; discard flags left by INT 25h/26h
        jmp short .out                  ; (pop/jmp keep CF)
.ioerr: stc
.out:   DBG 'e'
        pop es
        pop ds
        pop bp
        pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        ret

; ---------------------------------------------------------------------------
; cache_write - write back cache SI (assumed valid & dirty); clears dirty
; ---------------------------------------------------------------------------
cache_write:
        push ax
        push cx
        push dx
        mov ax,[si+C_BUF]
        mov [io_buf],ax
        mov byte [io_write],1
        mov ax,[si+C_SEC]
        mov dx,[si+C_SEC+2]
        cmp byte [si+C_ISFAT],0
        je .single
        add ax,[dv_fatstart]
        adc dx,0
        xor ch,ch
        mov cl,[dv_nfats]
.fatlp: mov [io_sec],ax
        mov [io_sec+2],dx
        call disk_io
        jc .err
        add ax,[dv_fatsz]
        adc dx,0
        loop .fatlp
        jmp short .ok
.single:
        mov [io_sec],ax
        mov [io_sec+2],dx
        call disk_io
        jc .err
.ok:    mov byte [si+C_DIRTY],0
        clc
.err:   pop dx
        pop cx
        pop ax
        ret

; ---------------------------------------------------------------------------
; cache_get - make sector DX:AX available in cache SI
;   out: BX = buffer, CF on error
; ---------------------------------------------------------------------------
cache_get:
        cmp byte [si+C_VALID],0
        je .load
        cmp ax,[si+C_SEC]
        jne .load
        cmp dx,[si+C_SEC+2]
        jne .load
        mov bx,[si+C_BUF]
        clc
        ret
.load:  cmp byte [si+C_VALID],0
        je .rd
        cmp byte [si+C_DIRTY],0
        je .rd
        call cache_write
        jc .ret
.rd:    mov byte [si+C_VALID],0
        mov [si+C_SEC],ax
        mov [si+C_SEC+2],dx
        push ax
        push dx
        cmp byte [si+C_ISFAT],0
        je .abs
        add ax,[dv_fatstart]
        adc dx,0
.abs:   mov [io_sec],ax
        mov [io_sec+2],dx
        mov bx,[si+C_BUF]
        mov [io_buf],bx
        mov byte [io_write],0
        call disk_io
        pop dx
        pop ax
        jc .ret
        mov byte [si+C_VALID],1
        mov byte [si+C_DIRTY],0
.ret:   ret

; ---------------------------------------------------------------------------
; cache_flush_all - write back dirty caches.  CF on error.
; cache_drop_all  - flush and invalidate both caches
; ---------------------------------------------------------------------------
cache_flush_all:
        push si
        mov si,c_dir
        call .one
        jc .x
        mov si,c_fat
        call .one
.x:     pop si
        ret
.one:   cmp byte [si+C_VALID],0
        je .clean
        cmp byte [si+C_DIRTY],0
        je .clean
        jmp cache_write
.clean: clc
        ret

cache_drop_all:
        call cache_flush_all
        pushf
        mov byte [c_dir+C_VALID],0
        mov byte [c_fat+C_VALID],0
        popf
        ret

; ---------------------------------------------------------------------------
; dos_sync - write our dirty sectors, forget them, let DOS flush/discard its
; buffers (INT 21h/0Dh).  Used around every DOS call that touches a disk.
; ---------------------------------------------------------------------------
dos_sync:
        push ax
        call cache_drop_all
        pushf
        mov ah,0Dh
        int 21h
        popf
        pop ax
        ret

; ---------------------------------------------------------------------------
; drive_setup - load parameters of physical drive AL (0=A:)
;   out: CF set if the drive is not a FAT12/FAT16 disk we can handle
; ---------------------------------------------------------------------------
drive_setup:
        push ax
        push bx
        push cx
        push dx
        push si
        cmp byte [dv_ok],0
        je .new
        cmp al,[dv_drive]
        jne .new
        cmp byte [dv_fixed],0
        je .new
        clc                             ; cached fixed disk
        jmp .ret
.new:   call cache_drop_all
        mov byte [dv_ok],0
        mov [dv_drive],al
        ; removable?  (IOCTL 4408h: AX=0 removable, 1 fixed)
        mov bl,al
        inc bl
        mov ax,4408h
        int 21h
        mov byte [dv_fixed],0
        jc .rdboot
        cmp ax,1
        jne .rdboot
        mov byte [dv_fixed],1
.rdboot:
        ; boot sector: first the classic call, then the extended one
        mov word [io_sec],0
        mov word [io_sec+2],0
        mov word [io_buf],dirbuf
        mov byte [io_write],0
        mov byte [dv_ext],0
        call disk_io
        jnc .gotbs
        mov byte [dv_ext],1
        call disk_io
        jc .bad
.gotbs: mov si,dirbuf
        cmp word [si+0Bh],512           ; bytes per sector
        jne .bad
        mov al,[si+0Dh]                 ; sectors per cluster (power of 2)
        or al,al
        jz .bad
        mov [dv_spc],al
        xor cl,cl
.shf:   shr al,1
        jc .shfd
        inc cl
        jmp short .shf
.shfd:  or al,al                        ; more bits left -> not a power of 2
        jnz .bad
        mov [dv_spcshift],cl
        mov ax,[si+0Eh]                 ; reserved sectors
        or ax,ax
        jz .bad
        mov [dv_fatstart],ax
        mov al,[si+10h]                 ; number of FATs
        or al,al
        jz .bad
        cmp al,4
        ja .bad
        mov [dv_nfats],al
        mov ax,[si+11h]                 ; root entries
        or ax,ax
        jz .bad                         ; 0 -> FAT32
        mov [dv_rootents],ax
        mov ax,[si+16h]                 ; sectors per FAT
        or ax,ax
        jz .bad                         ; FAT32
        mov [dv_fatsz],ax
        ; root start = reserved + nfats*fatsz
        xor ah,ah
        mov al,[dv_nfats]
        mul word [dv_fatsz]
        or dx,dx
        jnz .bad
        add ax,[dv_fatstart]
        jc .bad
        mov [dv_rootstart],ax
        mov bx,[dv_rootents]
        add bx,15
        mov cl,4
        shr bx,cl
        add ax,bx
        jc .bad
        mov [dv_datastart],ax
        ; total sectors
        mov ax,[si+13h]
        xor dx,dx
        or ax,ax
        jnz .tot
        mov ax,[si+20h]
        mov dx,[si+22h]
.tot:   sub ax,[dv_datastart]
        sbb dx,0
        jc .bad
        mov cl,[dv_spcshift]
        or cl,cl
        jz .ncl
.cdiv:  shr dx,1
        rcr ax,1
        dec cl
        jnz .cdiv
.ncl:   ; DX:AX = number of data clusters
        or dx,dx
        jnz .bad
        cmp ax,65525
        jae .bad                        ; FAT32
        cmp ax,1
        jb .bad
        mov byte [dv_fat16],0
        cmp ax,4085
        jb .f12
        mov byte [dv_fat16],1
.f12:   inc ax
        mov [dv_maxclust],ax            ; highest valid cluster number
        ; volume serial number (extended BPB)
        xor ax,ax
        mov [dv_serial],ax
        mov [dv_serial+2],ax
        cmp byte [si+26h],29h
        jne .noser
        mov ax,[si+27h]
        mov [dv_serial],ax
        mov ax,[si+29h]
        mov [dv_serial+2],ax
.noser: mov word [dv_freehint],2
        mov byte [dv_ok],1
        clc
        jmp short .ret
.bad:   mov byte [dv_ok],0
        stc
.ret:   pop si
        pop dx
        pop cx
        pop bx
        pop ax
        ret

; ---------------------------------------------------------------------------
; clus2sec - AX = cluster -> DX:AX = first sector of cluster
; ---------------------------------------------------------------------------
clus2sec:
        push cx
        sub ax,2
        xor dx,dx
        mov cl,[dv_spcshift]
        or cl,cl
        jz .n
.l:     shl ax,1
        rcl dx,1
        dec cl
        jnz .l
.n:     add ax,[dv_datastart]
        adc dx,0
        pop cx
        ret

; ---------------------------------------------------------------------------
; fat_locate - AX = cluster -> sector (DX:AX relative to FAT) and
;   CX = byte offset in sector.  (internal helper)
; ---------------------------------------------------------------------------
fat_locate:
        cmp byte [dv_fat16],0
        je .f12
        mov cx,ax
        xor ch,ch
        shl cx,1                        ; (cluster & 0FFh) * 2
        mov al,ah
        xor ah,ah                       ; cluster >> 8
        xor dx,dx
        ret
.f12:   mov cx,ax
        shr cx,1
        add cx,ax                       ; cluster * 3 / 2 (< 6200)
        mov ax,cx
        and cx,511
        push cx
        mov cl,9
        shr ax,cl
        pop cx
        xor dx,dx
        ret

; ---------------------------------------------------------------------------
; fat_get - AX = cluster -> AX = FAT entry (EOC/bad normalized to 0FFFFh)
;   CF on I/O error.
; ---------------------------------------------------------------------------
fat_get:
        push bx
        push cx
        push dx
        push si
        push di
        mov di,ax                       ; remember cluster
        call fat_locate
        mov si,c_fat
        call cache_get
        jc .ret
        add bx,cx
        cmp byte [dv_fat16],0
        je .f12
        mov ax,[bx]
        cmp ax,0FFF7h
        jb .ok
        mov ax,0FFFFh
        jmp short .ok
.f12:   mov cl,[bx]                     ; low byte
        cmp bx,fatbuf+511
        jne .same
        push cx
        inc ax                          ; next FAT sector (DX stays 0)
        call cache_get
        pop cx
        jc .ret
        mov ch,[bx]
        jmp short .got
.same:  mov ch,[bx+1]
.got:   mov ax,cx
        test di,1
        jz .even
        mov cl,4
        shr ax,cl
.even:  and ax,0FFFh
        cmp ax,0FF7h
        jb .ok
        mov ax,0FFFFh
.ok:    clc
.ret:   pop di
        pop si
        pop dx
        pop cx
        pop bx
        ret

; ---------------------------------------------------------------------------
; fat_set - set FAT entry of cluster AX to value BX.  CF on error.
; ---------------------------------------------------------------------------
fat_set:
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        push bp
        mov di,ax                       ; cluster
        mov bp,bx                       ; value
        call fat_locate
        mov si,c_fat
        call cache_get
        jc .ret
        add bx,cx
        cmp byte [dv_fat16],0
        je .f12
        mov [bx],bp
        mov byte [si+C_DIRTY],1
        clc
        jmp short .ret
.f12:   and bp,0FFFh
        test di,1
        jz .ev
        ; odd: value occupies high nibble of byte0 and all of byte1
        mov cl,4
        shl bp,cl
        mov dl,[bx]
        and dl,0Fh
        mov cx,bp
        or dl,cl
        mov [bx],dl
        mov byte [si+C_DIRTY],1
        cmp bx,fatbuf+511
        jne .o_same
        inc ax
        call cache_get
        jc .ret
        mov cx,bp
        mov [bx],ch
        mov byte [si+C_DIRTY],1
        clc
        jmp short .ret
.o_same:
        mov cx,bp
        mov [bx+1],ch
        clc
        jmp short .ret
.ev:    ; even: low byte = value low 8 bits, next byte low nibble = bits 8-11
        mov cx,bp
        mov [bx],cl
        mov byte [si+C_DIRTY],1
        cmp bx,fatbuf+511
        jne .e_same
        inc ax
        call cache_get
        jc .ret
        mov cx,bp
        mov dl,[bx]
        and dl,0F0h
        or dl,ch
        mov [bx],dl
        mov byte [si+C_DIRTY],1
        clc
        jmp short .ret
.e_same:
        mov dl,[bx+1]
        and dl,0F0h
        or dl,ch
        mov [bx+1],dl
        clc
.ret:   pop bp
        pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        ret

; ---------------------------------------------------------------------------
; Directory access.  A directory is identified by its first cluster
; (0 = fixed root directory).  Entries are addressed by index.
; ---------------------------------------------------------------------------

; dir_set - AX = first cluster of directory to work on
dir_set:
        mov [d_clus],ax
        mov [d_cc],ax
        mov word [d_ci],0
        ret

; dir_ptr - AX = entry index -> BX = pointer to the 32-byte entry in the
;   directory cache.  CF set if the index lies beyond the end of the
;   directory (or on I/O error; then [dir_ioerr] = 1).
dir_ptr:
        push ax
        push cx
        push dx
        push si
        mov byte [dir_ioerr],0
        mov si,ax                       ; index
        cmp word [d_clus],0
        jne .sub
        cmp ax,[dv_rootents]
        jae .end
        mov cl,4
        shr ax,cl
        add ax,[dv_rootstart]
        xor dx,dx
        jmp short .load
.sub:   mov cl,4
        shr ax,cl                       ; sector number inside directory
        mov dx,ax
        mov cl,[dv_spcshift]
        shr ax,cl                       ; cluster index
        push ax
        mov ax,1
        shl ax,cl
        dec ax
        and dx,ax                       ; sector inside cluster
        pop ax
        mov cx,ax                       ; wanted cluster index
        cmp cx,[d_ci]
        jae .walk
        mov ax,[d_clus]
        mov [d_cc],ax
        mov word [d_ci],0
.walk:  cmp cx,[d_ci]
        je .have
        mov ax,[d_cc]
        push dx
        call fat_get
        pop dx
        jc .ioe
        cmp ax,2
        jb .end
        cmp ax,[dv_maxclust]
        ja .end
        mov [d_cc],ax
        inc word [d_ci]
        jmp short .walk
.have:  mov ax,[d_cc]
        push dx
        call clus2sec
        pop cx
        add ax,cx
        adc dx,0
.load:  push si
        mov si,c_dir
        call cache_get
        pop si
        jc .ioe
        mov ax,si
        and ax,15
        mov cl,5
        shl ax,cl
        add bx,ax
        clc
        jmp short .ret
.ioe:   mov byte [dir_ioerr],1
.end:   stc
.ret:   pop si
        pop dx
        pop cx
        pop ax
        ret

; dir_dirty - mark the directory cache sector as modified
dir_dirty:
        mov byte [c_dir+C_DIRTY],1
        ret

; ---------------------------------------------------------------------------
; dir_extend - append one zeroed cluster to the current (non-root)
;   directory.  CF on error (disk full / root directory / I/O).
; ---------------------------------------------------------------------------
dir_extend:
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        mov ax,[d_clus]
        cmp ax,2
        jb .fail
        ; find last cluster of the chain
.last:  mov si,ax
        call fat_get
        jc .fail
        cmp ax,2
        jb .lastf
        cmp ax,[dv_maxclust]
        jbe .last
.lastf: ; SI = last cluster; search a free cluster
        mov ax,[dv_freehint]
        mov dx,[dv_maxclust]
        dec dx                          ; number of clusters to try
.scan:  cmp ax,[dv_maxclust]
        jbe .chk
        mov ax,2
.chk:   mov di,ax
        call fat_get
        jc .fail
        or ax,ax
        jz .found
        lea ax,[di+1]
        dec dx
        jnz .scan
        jmp short .fail
.found: ; DI = free cluster
        mov ax,di
        mov bx,0FFFFh
        call fat_set
        jc .fail
        mov ax,si
        mov bx,di
        call fat_set
        jc .fail
        lea ax,[di+1]
        mov [dv_freehint],ax
        call cache_flush_all
        jc .fail
        ; zero the new cluster through the directory cache buffer
        mov byte [c_dir+C_VALID],0
        push di
        mov di,dirbuf
        mov cx,256
        xor ax,ax
        rep stosw
        pop di
        mov ax,di
        call clus2sec
        xor ch,ch
        mov cl,[dv_spc]
        mov word [io_buf],dirbuf
        mov byte [io_write],1
.zl:    mov [io_sec],ax
        mov [io_sec+2],dx
        call disk_io
        jc .fail
        add ax,1
        adc dx,0
        loop .zl
        ; restart cluster cache of the directory walker
        mov ax,[d_clus]
        call dir_set
        call free_unknown
        clc
        jmp short .ret
.fail:  stc
.ret:   pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        ret

; ---------------------------------------------------------------------------
; free_unknown - we allocated a cluster behind DOS' back: mark the free
;   cluster count in DOS' drive parameter block as unknown (DOS 4+ layout,
;   offset 1Fh; DOS 3.x keeps it at 1Eh)
; ---------------------------------------------------------------------------
free_unknown:
        push ax
        push bx
        push dx
        push ds
        mov dl,[dv_drive]
        inc dl
        mov bh,[cs:dosmajor]
        mov ah,32h
        int 21h
        cmp al,0FFh
        je .x
        cmp bh,4
        jb .d3
        mov word [bx+1Fh],0FFFFh
        jmp short .x
.d3:    mov word [bx+1Eh],0FFFFh
.x:     pop ds
        pop dx
        pop bx
        pop ax
        ret
