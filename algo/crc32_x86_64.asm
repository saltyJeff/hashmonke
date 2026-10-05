.const
ALIGN 16
LCPI0_0 QWORD 0, 1c6e41596h
LCPI0_1 QWORD 0, 0ccaa009eh
LCPI0_2 QWORD 0, 1f7011641h

.code
PUBLIC crc32_arch_asm
ALIGN 16
crc32_arch_asm PROC
    push    rdi
    push    rsi
    sub     rsp, 88
    movdqu  XMMWORD PTR [rsp], xmm6
    movdqu  XMMWORD PTR [rsp+16], xmm7
    movdqu  XMMWORD PTR [rsp+32], xmm8
    movdqu  XMMWORD PTR [rsp+48], xmm9
    movdqu  XMMWORD PTR [rsp+64], xmm10
    mov     edi, ecx
    mov     rsi, rdx
    mov     rdx, r8
    movdqu  xmm1, XMMWORD PTR [rsi]
    movdqu  xmm2, XMMWORD PTR [rsi+16]
    movdqu  xmm4, XMMWORD PTR [rsi+32]
    movdqu  xmm3, XMMWORD PTR [rsi+48]
    movd    xmm0, edi
    pxor    xmm0, xmm1
    add     rdx, -64
    add     rsi, 64
    cmp     rdx, 64
    jb      LBB0_3

    mov     rax, 154442BD4h
    movq    xmm1, rax
    movdqa  xmm5, XMMWORD PTR [LCPI0_0]
    ALIGN 16
LBB0_2:
    movdqa      xmm6, xmm0
    pclmulqdq   xmm6, xmm1, 0
    movdqa      xmm7, xmm2
    pclmulqdq   xmm7, xmm1, 0
    movdqa      xmm8, xmm4
    pclmulqdq   xmm8, xmm1, 0
    movdqa      xmm9, xmm3
    pclmulqdq   xmm9, xmm1, 0
    pclmulqdq   xmm0, xmm5, 17
    pclmulqdq   xmm2, xmm5, 17
    pclmulqdq   xmm4, xmm5, 17
    pclmulqdq   xmm3, xmm5, 17
    movdqu      xmm10, XMMWORD PTR [rsi]
    pxor        xmm10, xmm6
    pxor        xmm0, xmm10
    movdqu      xmm6, XMMWORD PTR [rsi+16]
    pxor        xmm6, xmm7
    pxor        xmm2, xmm6
    movdqu      xmm6, XMMWORD PTR [rsi+32]
    pxor        xmm6, xmm8
    pxor        xmm4, xmm6
    movdqu      xmm6, XMMWORD PTR [rsi+48]
    pxor        xmm6, xmm9
    pxor        xmm3, xmm6
    add         rdx, -64
    add         rsi, 64
    cmp         rdx, 63
    ja          LBB0_2

LBB0_3:
    mov         rax, 1751997D0h
    movq        xmm1, rax
    movdqa      xmm5, xmm0
    pclmulqdq   xmm5, xmm1, 0
    pxor        xmm5, xmm2
    movdqa      xmm2, XMMWORD PTR [LCPI0_1]
    pclmulqdq   xmm0, xmm2, 17
    pxor        xmm0, xmm5
    movdqa      xmm5, xmm0
    pclmulqdq   xmm5, xmm1, 0
    pxor        xmm5, xmm4
    pclmulqdq   xmm0, xmm2, 17
    pxor        xmm0, xmm5
    movdqa      xmm4, xmm0
    pclmulqdq   xmm4, xmm1, 0
    pxor        xmm4, xmm3
    pclmulqdq   xmm0, xmm2, 17
    pxor        xmm0, xmm4
    cmp         rdx, 16
    jb          LBB0_4

    lea     rax, [rdx-16]
    mov     ecx, eax
    not     ecx
    test    cl, 48
    je      LBB0_8

    mov     ecx, eax
    shr     ecx, 4
    inc     ecx
    and     ecx, 3
    ALIGN 16
LBB0_7:
    movdqu      xmm3, XMMWORD PTR [rsi]
    movdqa      xmm4, xmm0
    pclmulqdq   xmm4, xmm1, 0
    pxor        xmm4, xmm3
    pclmulqdq   xmm0, xmm2, 17
    pxor        xmm0, xmm4
    add         rsi, 16
    add         rdx, -16
    dec         rcx
    jne         LBB0_7

LBB0_8:
    movdqa  xmm3, xmm0
    cmp     rax, 48
    jb      LBB0_10
    ALIGN 16
LBB0_9:
    movdqa      xmm4, xmm0
    pclmulqdq   xmm4, xmm1, 0
    pclmulqdq   xmm0, xmm2, 17
    movdqu      xmm3, XMMWORD PTR [rsi]
    pxor        xmm3, xmm4
    pxor        xmm3, xmm0
    movdqu      xmm0, XMMWORD PTR [rsi+16]
    movdqu      xmm4, XMMWORD PTR [rsi+32]
    movdqu      xmm5, XMMWORD PTR [rsi+48]
    movdqa      xmm6, xmm3
    pclmulqdq   xmm6, xmm1, 0
    pxor        xmm6, xmm0
    pclmulqdq   xmm3, xmm2, 17
    pxor        xmm3, xmm6
    movdqa      xmm0, xmm3
    pclmulqdq   xmm0, xmm1, 0
    pxor        xmm0, xmm4
    pclmulqdq   xmm3, xmm2, 17
    pxor        xmm3, xmm0
    movdqa      xmm0, xmm3
    pclmulqdq   xmm0, xmm1, 0
    pxor        xmm0, xmm5
    pclmulqdq   xmm3, xmm2, 17
    pxor        xmm3, xmm0
    add         rsi, 64
    add         rdx, -64
    movdqa      xmm0, xmm3
    cmp         rdx, 15
    ja          LBB0_9
    jmp         LBB0_10

LBB0_4:
    movdqa  xmm3, xmm0

LBB0_10:
    movdqa      xmm0, XMMWORD PTR [LCPI0_1]
    pclmulqdq   xmm0, xmm3, 1
    psrldq      xmm3, 8
    pxor        xmm3, xmm0
    pxor        xmm0, xmm0
    pxor        xmm1, xmm1
    pblendw     xmm1, xmm3, 3
    psrldq      xmm3, 4
    mov         rax, 163CD6124h
    movq        xmm2, rax
    pclmulqdq   xmm2, xmm1, 0
    pxor        xmm2, xmm3
    pxor        xmm1, xmm1
    pblendw     xmm1, xmm2, 3
    pclmulqdq   xmm1, XMMWORD PTR [LCPI0_2], 16
    pblendw     xmm1, xmm0, 252
    mov         rax, 1DB710641h
    movq        xmm0, rax
    pclmulqdq   xmm0, xmm1, 0
    pxor        xmm0, xmm2
    pextrd      eax, xmm0, 1
    movdqu      xmm6, XMMWORD PTR [rsp]
    movdqu      xmm7, XMMWORD PTR [rsp+16]
    movdqu      xmm8, XMMWORD PTR [rsp+32]
    movdqu      xmm9, XMMWORD PTR [rsp+48]
    movdqu      xmm10, XMMWORD PTR [rsp+64]
    add         rsp, 88
    pop         rsi
    pop         rdi
    ret
crc32_arch_asm ENDP
END

