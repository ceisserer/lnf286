; ===========================================================================
; time.inc - DOS date/time <-> Win32 FILETIME (100ns units since 1601-01-01)
; 64 bit arithmetic on the accumulator [acc] using 16 bit operations only.
; ===========================================================================

DAYS_1601_1980 equ 138426       ; days from 1601-01-01 to 1980-01-01

; acc_mul - acc *= CX
acc_mul:
        push ax
        push bx
        push dx
        push si
        xor bx,bx                       ; carry word
        xor si,si
.l:     mov ax,[acc+si]
        mul cx
        add ax,bx
        adc dx,0
        mov [acc+si],ax
        mov bx,dx
        add si,2
        cmp si,8
        jb .l
        pop si
        pop dx
        pop bx
        pop ax
        ret

; acc_add - acc += AX
acc_add:
        add [acc],ax
        adc word [acc+2],0
        adc word [acc+4],0
        adc word [acc+6],0
        ret

; acc_div - acc /= CX, remainder -> DX
acc_div:
        push ax
        push si
        xor dx,dx
        mov si,6
.l:     mov ax,[acc+si]
        div cx
        mov [acc+si],ax
        sub si,2
        jnc .l
        pop si
        pop ax
        ret

; is_leap - AL = year - 1980 -> ZF=1 if leap year (valid 1980..2107)
is_leap:
        test al,3
        jnz .x
        cmp al,120                      ; 2100 is not a leap year
        je .no
        cmp al,al                       ; ZF=1
.x:     ret
.no:    or sp,sp                        ; ZF=0
        ret

month_days dw 0,31,59,90,120,151,181,212,243,273,304,334,365

; ---------------------------------------------------------------------------
; dos2ft - DX = DOS date, CX = DOS time, BL = 10ms units (0..199)
;          -> acc = FILETIME
; ---------------------------------------------------------------------------
dos2ft:
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        mov [t_time],cx
        mov [t_ms],bl
        mov [t_date],dx
        ; year offset
        mov ax,dx
        mov cl,9
        shr ax,cl
        mov di,ax                       ; DI = year - 1980
        ; days for whole years: off*365 + (off+3)/4 - (off>120)
        mov cx,365
        mul cx
        mov dx,[t_date]
        mov si,ax
        mov ax,di
        add ax,3
        shr ax,1
        shr ax,1
        add si,ax
        cmp di,120
        jbe .ny
        dec si
.ny:    ; month
        mov ax,dx
        mov cl,5
        shr ax,cl
        and ax,15
        jnz .m1
        inc ax
.m1:    cmp ax,12
        jbe .m2
        mov ax,12
.m2:    mov bx,ax
        dec bx
        shl bx,1
        add si,[month_days+bx]
        cmp ax,2
        jbe .nl
        mov ax,di
        call is_leap
        jne .nl
        inc si
.nl:    mov ax,dx
        and ax,31
        jz .d0
        dec ax
.d0:    add si,ax
        ; acc = SI + DAYS_1601_1980
        mov word [acc+4],0
        mov word [acc+6],0
        mov word [acc],DAYS_1601_1980 & 0FFFFh
        mov word [acc+2],DAYS_1601_1980 >> 16
        mov ax,si
        call acc_add
        ; hours
        mov cx,24
        call acc_mul
        mov ax,[t_time]
        mov cl,11
        shr ax,cl
        call acc_add
        mov cx,60
        call acc_mul
        mov ax,[t_time]
        mov cl,5
        shr ax,cl
        and ax,63
        call acc_add
        mov cx,60
        call acc_mul
        mov ax,[t_time]
        and ax,31
        shl ax,1
        call acc_add
        mov cx,100
        call acc_mul
        mov al,[t_ms]
        xor ah,ah
        call acc_add
        mov cx,1000
        call acc_mul
        mov cx,100
        call acc_mul
        pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        ret

; ---------------------------------------------------------------------------
; ft2dos - acc = FILETIME -> DX = date, CX = time, BH = 10ms units
;   CF if the time is outside the DOS range (1980..2107)
; ---------------------------------------------------------------------------
ft2dos:
        push ax
        push si
        push di
        mov cx,1000
        call acc_div
        mov cx,100
        call acc_div                    ; acc = 10ms units
        call acc_div                    ; DX = 10ms remainder
        mov [t_ms],dl
        mov cx,60
        call acc_div
        mov [t_sec],dl
        call acc_div
        mov [t_min],dl
        mov cx,24
        call acc_div
        mov [t_hour],dl
        ; acc = days since 1601
        cmp word [acc+4],0
        jne .bad
        cmp word [acc+6],0
        jne .bad
        mov ax,[acc]
        mov dx,[acc+2]
        sub ax,DAYS_1601_1980 & 0FFFFh
        sbb dx,DAYS_1601_1980 >> 16
        jc .bad
        or dx,dx
        jnz .bad
        ; AX = days since 1980
        xor di,di                       ; year offset
.yl:    mov si,365
        push ax
        mov ax,di
        call is_leap
        pop ax
        jne .y365
        inc si
.y365:  cmp ax,si
        jb .yd
        sub ax,si
        inc di
        cmp di,127
        ja .bad
        jmp short .yl
.yd:    ; month
        mov si,1
.ml:    mov bx,si
        shl bx,1
        mov cx,[month_days+bx]          ; days before month si+1
        cmp si,2
        jb .mnl
        push ax
        mov ax,di
        call is_leap
        pop ax
        jne .mnl
        inc cx
.mnl:   cmp si,12
        jae .md
        cmp ax,cx
        jb .md
        inc si
        jmp short .ml
.md:    ; SI = month, days before it:
        mov bx,si
        dec bx
        shl bx,1
        mov cx,[month_days+bx]
        cmp si,2
        jbe .mn2
        push ax
        mov ax,di
        call is_leap
        pop ax
        jne .mn2
        inc cx
.mn2:   sub ax,cx
        inc ax                          ; day of month
        ; date = off<<9 | month<<5 | day
        mov dx,di
        mov cl,9
        shl dx,cl
        mov bx,si
        mov cl,5
        shl bx,cl
        or dx,bx
        or dx,ax
        ; time
        mov al,[t_hour]
        xor ah,ah
        mov cl,11
        shl ax,cl
        mov cx,ax
        mov al,[t_min]
        xor ah,ah
        push cx
        mov cl,5
        shl ax,cl
        pop cx
        or cx,ax
        mov al,[t_sec]
        mov bh,[t_ms]
        shr al,1
        jnc .even
        add bh,100
.even:  xor ah,ah
        or cx,ax
        clc
        jmp short .x
.bad:   stc
.x:     pop di
        pop si
        pop ax
        ret

; acc_store - copy acc to ES:DI (8 bytes), DI += 8
acc_store:
        push si
        push cx
        mov si,acc
        mov cx,4
        rep movsw
        pop cx
        pop si
        ret
