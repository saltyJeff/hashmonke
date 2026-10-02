	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
@feat.00 = 0
	.file	"gen_crc.c"
	.def	crc32_arch_asm;
	.scl	2;
	.type	32;
	.endef
	.section	.rdata,"dr"
	.p2align	4, 0x0                          # -- Begin function crc32_arch_asm
.LCPI0_0:
	.zero	8
	.quad	7631803798                      # 0x1c6e41596
.LCPI0_1:
	.zero	8
	.quad	3433693342                      # 0xccaa009e
.LCPI0_2:
	.zero	8
	.quad	8439010881                      # 0x1f7011641
	.text
	.globl	crc32_arch_asm
	.p2align	4
crc32_arch_asm:                         # @crc32_arch_asm
# %bb.0:
	movdqu	(%rsi), %xmm1
	movdqu	16(%rsi), %xmm2
	movdqu	32(%rsi), %xmm4
	movdqu	48(%rsi), %xmm3
	movd	%edi, %xmm0
	pxor	%xmm1, %xmm0
	addq	$-64, %rdx
	addq	$64, %rsi
	cmpq	$64, %rdx
	jb	.LBB0_3
# %bb.1:
	movabsq	$5708721108, %rax               # imm = 0x154442BD4
	movq	%rax, %xmm1
	movdqa	.LCPI0_0(%rip), %xmm5           # xmm5 = [u,7631803798]
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	movdqa	%xmm0, %xmm6
	pclmulqdq	$0, %xmm1, %xmm6
	movdqa	%xmm2, %xmm7
	pclmulqdq	$0, %xmm1, %xmm7
	movdqa	%xmm4, %xmm8
	pclmulqdq	$0, %xmm1, %xmm8
	movdqa	%xmm3, %xmm9
	pclmulqdq	$0, %xmm1, %xmm9
	pclmulqdq	$17, %xmm5, %xmm0
	pclmulqdq	$17, %xmm5, %xmm2
	pclmulqdq	$17, %xmm5, %xmm4
	pclmulqdq	$17, %xmm5, %xmm3
	movdqu	(%rsi), %xmm10
	pxor	%xmm6, %xmm10
	pxor	%xmm10, %xmm0
	movdqu	16(%rsi), %xmm6
	pxor	%xmm7, %xmm6
	pxor	%xmm6, %xmm2
	movdqu	32(%rsi), %xmm6
	pxor	%xmm8, %xmm6
	pxor	%xmm6, %xmm4
	movdqu	48(%rsi), %xmm6
	pxor	%xmm9, %xmm6
	pxor	%xmm6, %xmm3
	addq	$-64, %rdx
	addq	$64, %rsi
	cmpq	$63, %rdx
	ja	.LBB0_2
.LBB0_3:
	movabsq	$6259578832, %rax               # imm = 0x1751997D0
	movq	%rax, %xmm1
	movdqa	%xmm0, %xmm5
	pclmulqdq	$0, %xmm1, %xmm5
	pxor	%xmm2, %xmm5
	movdqa	.LCPI0_1(%rip), %xmm2           # xmm2 = [u,3433693342]
	pclmulqdq	$17, %xmm2, %xmm0
	pxor	%xmm5, %xmm0
	movdqa	%xmm0, %xmm5
	pclmulqdq	$0, %xmm1, %xmm5
	pxor	%xmm4, %xmm5
	pclmulqdq	$17, %xmm2, %xmm0
	pxor	%xmm5, %xmm0
	movdqa	%xmm0, %xmm4
	pclmulqdq	$0, %xmm1, %xmm4
	pxor	%xmm3, %xmm4
	pclmulqdq	$17, %xmm2, %xmm0
	pxor	%xmm4, %xmm0
	cmpq	$16, %rdx
	jb	.LBB0_4
# %bb.5:
	leaq	-16(%rdx), %rax
	movl	%eax, %ecx
	notl	%ecx
	testb	$48, %cl
	je	.LBB0_8
# %bb.6:
	movl	%eax, %ecx
	shrl	$4, %ecx
	incl	%ecx
	andl	$3, %ecx
	.p2align	4
.LBB0_7:                                # =>This Inner Loop Header: Depth=1
	movdqu	(%rsi), %xmm3
	movdqa	%xmm0, %xmm4
	pclmulqdq	$0, %xmm1, %xmm4
	pxor	%xmm3, %xmm4
	pclmulqdq	$17, %xmm2, %xmm0
	pxor	%xmm4, %xmm0
	addq	$16, %rsi
	addq	$-16, %rdx
	decq	%rcx
	jne	.LBB0_7
.LBB0_8:
	movdqa	%xmm0, %xmm3
	cmpq	$48, %rax
	jb	.LBB0_10
	.p2align	4
.LBB0_9:                                # =>This Inner Loop Header: Depth=1
	movdqa	%xmm0, %xmm4
	pclmulqdq	$0, %xmm1, %xmm4
	pclmulqdq	$17, %xmm2, %xmm0
	movdqu	(%rsi), %xmm3
	pxor	%xmm4, %xmm3
	pxor	%xmm0, %xmm3
	movdqu	16(%rsi), %xmm0
	movdqu	32(%rsi), %xmm4
	movdqu	48(%rsi), %xmm5
	movdqa	%xmm3, %xmm6
	pclmulqdq	$0, %xmm1, %xmm6
	pxor	%xmm0, %xmm6
	pclmulqdq	$17, %xmm2, %xmm3
	pxor	%xmm6, %xmm3
	movdqa	%xmm3, %xmm0
	pclmulqdq	$0, %xmm1, %xmm0
	pxor	%xmm4, %xmm0
	pclmulqdq	$17, %xmm2, %xmm3
	pxor	%xmm0, %xmm3
	movdqa	%xmm3, %xmm0
	pclmulqdq	$0, %xmm1, %xmm0
	pxor	%xmm5, %xmm0
	pclmulqdq	$17, %xmm2, %xmm3
	pxor	%xmm0, %xmm3
	addq	$64, %rsi
	addq	$-64, %rdx
	movdqa	%xmm3, %xmm0
	cmpq	$15, %rdx
	ja	.LBB0_9
	jmp	.LBB0_10
.LBB0_4:
	movdqa	%xmm0, %xmm3
.LBB0_10:
	movdqa	.LCPI0_1(%rip), %xmm0           # xmm0 = [u,3433693342]
	pclmulqdq	$1, %xmm3, %xmm0
	psrldq	$8, %xmm3                       # xmm3 = xmm3[8,9,10,11,12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero
	pxor	%xmm0, %xmm3
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	pblendw	$3, %xmm3, %xmm1                # xmm1 = xmm3[0,1],xmm1[2,3,4,5,6,7]
	psrldq	$4, %xmm3                       # xmm3 = xmm3[4,5,6,7,8,9,10,11,12,13,14,15],zero,zero,zero,zero
	movabsq	$5969371428, %rax               # imm = 0x163CD6124
	movq	%rax, %xmm2
	pclmulqdq	$0, %xmm1, %xmm2
	pxor	%xmm3, %xmm2
	pxor	%xmm1, %xmm1
	pblendw	$3, %xmm2, %xmm1                # xmm1 = xmm2[0,1],xmm1[2,3,4,5,6,7]
	pclmulqdq	$16, .LCPI0_2(%rip), %xmm1
	pblendw	$252, %xmm0, %xmm1              # xmm1 = xmm1[0,1],xmm0[2,3,4,5,6,7]
	movabsq	$7976584769, %rax               # imm = 0x1DB710641
	movq	%rax, %xmm0
	pclmulqdq	$0, %xmm1, %xmm0
	pxor	%xmm2, %xmm0
	pextrd	$1, %xmm0, %eax
	retq
                                        # -- End function
	.section	.debug$S,"dr"
	.p2align	2, 0x0
	.long	4                               # Debug section magic
	.long	241
	.long	.Ltmp1-.Ltmp0                   # Subsection size
.Ltmp0:
	.short	.Ltmp3-.Ltmp2                   # Record length
.Ltmp2:
	.short	4353                            # Record kind: S_OBJNAME
	.long	0                               # Signature
	.byte	0                               # Object name
	.p2align	2, 0x0
.Ltmp3:
	.short	.Ltmp5-.Ltmp4                   # Record length
.Ltmp4:
	.short	4412                            # Record kind: S_COMPILE3
	.long	0                               # Flags and language
	.short	208                             # CPUType
	.short	22                              # Frontend version
	.short	1
	.short	8
	.short	0
	.short	22018                           # Backend version
	.short	0
	.short	0
	.short	0
	.asciz	"clang version 22.1.8 (https://github.com/msys2/MINGW-packages 6e4e79c2f86eeb534e324e583f2057dc9fd5ecab)" # Null-terminated compiler version string
	.p2align	2, 0x0
.Ltmp5:
.Ltmp1:
	.p2align	2, 0x0
	.addrsig
