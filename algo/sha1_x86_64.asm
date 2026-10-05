.code
PUBLIC sha1_block_asm_data_order
ALIGN 16
sha1_block_asm_data_order PROC
ALIGN 16
	push	rdi
	push	rsi
	mov	rdi, rcx
	mov	rsi, rdx
	mov	rdx, r8
	mov	rax, rsp
	push	rbx
	push	rbp
	push	r12
	push	r13
	push	r14
	mov	r8, rdi
	sub	rsp, 72
	mov	r9, rsi
	and	rsp, -64
	mov	r10, rdx
	mov	QWORD PTR [rsp+64], rax
Lprologue:
	mov	esi, DWORD PTR [r8]
	mov	edi, DWORD PTR [r8+4]
	mov	r11d, DWORD PTR [r8+8]
	mov	r12d, DWORD PTR [r8+12]
	mov	r13d, DWORD PTR [r8+16]
	jmp	Lloop
ALIGN 16
Lloop:
	mov	edx, DWORD PTR [r9]
	bswap	edx
	mov	ebp, DWORD PTR [r9+4]
	mov	eax, r12d
	mov	DWORD PTR [rsp], edx
	mov	ecx, esi
	bswap	ebp
	xor	eax, r11d
	rol	ecx, 5
	and	eax, edi
	lea	r13d, [rdx+r13+1518500249]
	add	r13d, ecx
	xor	eax, r12d
	rol	edi, 30
	add	r13d, eax
	mov	r14d, DWORD PTR [r9+8]
	mov	eax, r11d
	mov	DWORD PTR [rsp+4], ebp
	mov	ecx, r13d
	bswap	r14d
	xor	eax, edi
	rol	ecx, 5
	and	eax, esi
	lea	r12d, [rbp+r12+1518500249]
	add	r12d, ecx
	xor	eax, r11d
	rol	esi, 30
	add	r12d, eax
	mov	edx, DWORD PTR [r9+12]
	mov	eax, edi
	mov	DWORD PTR [rsp+8], r14d
	mov	ecx, r12d
	bswap	edx
	xor	eax, esi
	rol	ecx, 5
	and	eax, r13d
	lea	r11d, [r14+r11+1518500249]
	add	r11d, ecx
	xor	eax, edi
	rol	r13d, 30
	add	r11d, eax
	mov	ebp, DWORD PTR [r9+16]
	mov	eax, esi
	mov	DWORD PTR [rsp+12], edx
	mov	ecx, r11d
	bswap	ebp
	xor	eax, r13d
	rol	ecx, 5
	and	eax, r12d
	lea	edi, [rdx+rdi+1518500249]
	add	edi, ecx
	xor	eax, esi
	rol	r12d, 30
	add	edi, eax
	mov	r14d, DWORD PTR [r9+20]
	mov	eax, r13d
	mov	DWORD PTR [rsp+16], ebp
	mov	ecx, edi
	bswap	r14d
	xor	eax, r12d
	rol	ecx, 5
	and	eax, r11d
	lea	esi, [rbp+rsi+1518500249]
	add	esi, ecx
	xor	eax, r13d
	rol	r11d, 30
	add	esi, eax
	mov	edx, DWORD PTR [r9+24]
	mov	eax, r12d
	mov	DWORD PTR [rsp+20], r14d
	mov	ecx, esi
	bswap	edx
	xor	eax, r11d
	rol	ecx, 5
	and	eax, edi
	lea	r13d, [r14+r13+1518500249]
	add	r13d, ecx
	xor	eax, r12d
	rol	edi, 30
	add	r13d, eax
	mov	ebp, DWORD PTR [r9+28]
	mov	eax, r11d
	mov	DWORD PTR [rsp+24], edx
	mov	ecx, r13d
	bswap	ebp
	xor	eax, edi
	rol	ecx, 5
	and	eax, esi
	lea	r12d, [rdx+r12+1518500249]
	add	r12d, ecx
	xor	eax, r11d
	rol	esi, 30
	add	r12d, eax
	mov	r14d, DWORD PTR [r9+32]
	mov	eax, edi
	mov	DWORD PTR [rsp+28], ebp
	mov	ecx, r12d
	bswap	r14d
	xor	eax, esi
	rol	ecx, 5
	and	eax, r13d
	lea	r11d, [rbp+r11+1518500249]
	add	r11d, ecx
	xor	eax, edi
	rol	r13d, 30
	add	r11d, eax
	mov	edx, DWORD PTR [r9+36]
	mov	eax, esi
	mov	DWORD PTR [rsp+32], r14d
	mov	ecx, r11d
	bswap	edx
	xor	eax, r13d
	rol	ecx, 5
	and	eax, r12d
	lea	edi, [r14+rdi+1518500249]
	add	edi, ecx
	xor	eax, esi
	rol	r12d, 30
	add	edi, eax
	mov	ebp, DWORD PTR [r9+40]
	mov	eax, r13d
	mov	DWORD PTR [rsp+36], edx
	mov	ecx, edi
	bswap	ebp
	xor	eax, r12d
	rol	ecx, 5
	and	eax, r11d
	lea	esi, [rdx+rsi+1518500249]
	add	esi, ecx
	xor	eax, r13d
	rol	r11d, 30
	add	esi, eax
	mov	r14d, DWORD PTR [r9+44]
	mov	eax, r12d
	mov	DWORD PTR [rsp+40], ebp
	mov	ecx, esi
	bswap	r14d
	xor	eax, r11d
	rol	ecx, 5
	and	eax, edi
	lea	r13d, [rbp+r13+1518500249]
	add	r13d, ecx
	xor	eax, r12d
	rol	edi, 30
	add	r13d, eax
	mov	edx, DWORD PTR [r9+48]
	mov	eax, r11d
	mov	DWORD PTR [rsp+44], r14d
	mov	ecx, r13d
	bswap	edx
	xor	eax, edi
	rol	ecx, 5
	and	eax, esi
	lea	r12d, [r14+r12+1518500249]
	add	r12d, ecx
	xor	eax, r11d
	rol	esi, 30
	add	r12d, eax
	mov	ebp, DWORD PTR [r9+52]
	mov	eax, edi
	mov	DWORD PTR [rsp+48], edx
	mov	ecx, r12d
	bswap	ebp
	xor	eax, esi
	rol	ecx, 5
	and	eax, r13d
	lea	r11d, [rdx+r11+1518500249]
	add	r11d, ecx
	xor	eax, edi
	rol	r13d, 30
	add	r11d, eax
	mov	r14d, DWORD PTR [r9+56]
	mov	eax, esi
	mov	DWORD PTR [rsp+52], ebp
	mov	ecx, r11d
	bswap	r14d
	xor	eax, r13d
	rol	ecx, 5
	and	eax, r12d
	lea	edi, [rbp+rdi+1518500249]
	add	edi, ecx
	xor	eax, esi
	rol	r12d, 30
	add	edi, eax
	mov	edx, DWORD PTR [r9+60]
	mov	eax, r13d
	mov	DWORD PTR [rsp+56], r14d
	mov	ecx, edi
	bswap	edx
	xor	eax, r12d
	rol	ecx, 5
	and	eax, r11d
	lea	esi, [r14+rsi+1518500249]
	add	esi, ecx
	xor	eax, r13d
	rol	r11d, 30
	add	esi, eax
	xor	ebp, DWORD PTR [rsp]
	mov	eax, r12d
	mov	DWORD PTR [rsp+60], edx
	mov	ecx, esi
	xor	ebp, DWORD PTR [rsp+8]
	xor	eax, r11d
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+32]
	and	eax, edi
	lea	r13d, [rdx+r13+1518500249]
	rol	edi, 30
	xor	eax, r12d
	add	r13d, ecx
	rol	ebp, 1
	add	r13d, eax
	xor	r14d, DWORD PTR [rsp+4]
	mov	eax, r11d
	mov	DWORD PTR [rsp], ebp
	mov	ecx, r13d
	xor	r14d, DWORD PTR [rsp+12]
	xor	eax, edi
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+36]
	and	eax, esi
	lea	r12d, [rbp+r12+1518500249]
	rol	esi, 30
	xor	eax, r11d
	add	r12d, ecx
	rol	r14d, 1
	add	r12d, eax
	xor	edx, DWORD PTR [rsp+8]
	mov	eax, edi
	mov	DWORD PTR [rsp+4], r14d
	mov	ecx, r12d
	xor	edx, DWORD PTR [rsp+16]
	xor	eax, esi
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+40]
	and	eax, r13d
	lea	r11d, [r14+r11+1518500249]
	rol	r13d, 30
	xor	eax, edi
	add	r11d, ecx
	rol	edx, 1
	add	r11d, eax
	xor	ebp, DWORD PTR [rsp+12]
	mov	eax, esi
	mov	DWORD PTR [rsp+8], edx
	mov	ecx, r11d
	xor	ebp, DWORD PTR [rsp+20]
	xor	eax, r13d
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+44]
	and	eax, r12d
	lea	edi, [rdx+rdi+1518500249]
	rol	r12d, 30
	xor	eax, esi
	add	edi, ecx
	rol	ebp, 1
	add	edi, eax
	xor	r14d, DWORD PTR [rsp+16]
	mov	eax, r13d
	mov	DWORD PTR [rsp+12], ebp
	mov	ecx, edi
	xor	r14d, DWORD PTR [rsp+24]
	xor	eax, r12d
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+48]
	and	eax, r11d
	lea	esi, [rbp+rsi+1518500249]
	rol	r11d, 30
	xor	eax, r13d
	add	esi, ecx
	rol	r14d, 1
	add	esi, eax
	xor	edx, DWORD PTR [rsp+20]
	mov	eax, edi
	mov	DWORD PTR [rsp+16], r14d
	mov	ecx, esi
	xor	edx, DWORD PTR [rsp+28]
	xor	eax, r12d
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+52]
	lea	r13d, [r14+r13+1859775393]
	xor	eax, r11d
	add	r13d, ecx
	rol	edi, 30
	add	r13d, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+24]
	mov	eax, esi
	mov	DWORD PTR [rsp+20], edx
	mov	ecx, r13d
	xor	ebp, DWORD PTR [rsp+32]
	xor	eax, r11d
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+56]
	lea	r12d, [rdx+r12+1859775393]
	xor	eax, edi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+28]
	mov	eax, r13d
	mov	DWORD PTR [rsp+24], ebp
	mov	ecx, r12d
	xor	r14d, DWORD PTR [rsp+36]
	xor	eax, edi
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+60]
	lea	r11d, [rbp+r11+1859775393]
	xor	eax, esi
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+32]
	mov	eax, r12d
	mov	DWORD PTR [rsp+28], r14d
	mov	ecx, r11d
	xor	edx, DWORD PTR [rsp+40]
	xor	eax, esi
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp]
	lea	edi, [r14+rdi+1859775393]
	xor	eax, r13d
	add	edi, ecx
	rol	r12d, 30
	add	edi, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+36]
	mov	eax, r11d
	mov	DWORD PTR [rsp+32], edx
	mov	ecx, edi
	xor	ebp, DWORD PTR [rsp+44]
	xor	eax, r13d
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+4]
	lea	esi, [rdx+rsi+1859775393]
	xor	eax, r12d
	add	esi, ecx
	rol	r11d, 30
	add	esi, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+40]
	mov	eax, edi
	mov	DWORD PTR [rsp+36], ebp
	mov	ecx, esi
	xor	r14d, DWORD PTR [rsp+48]
	xor	eax, r12d
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+8]
	lea	r13d, [rbp+r13+1859775393]
	xor	eax, r11d
	add	r13d, ecx
	rol	edi, 30
	add	r13d, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+44]
	mov	eax, esi
	mov	DWORD PTR [rsp+40], r14d
	mov	ecx, r13d
	xor	edx, DWORD PTR [rsp+52]
	xor	eax, r11d
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+12]
	lea	r12d, [r14+r12+1859775393]
	xor	eax, edi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+48]
	mov	eax, r13d
	mov	DWORD PTR [rsp+44], edx
	mov	ecx, r12d
	xor	ebp, DWORD PTR [rsp+56]
	xor	eax, edi
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+16]
	lea	r11d, [rdx+r11+1859775393]
	xor	eax, esi
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+52]
	mov	eax, r12d
	mov	DWORD PTR [rsp+48], ebp
	mov	ecx, r11d
	xor	r14d, DWORD PTR [rsp+60]
	xor	eax, esi
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+20]
	lea	edi, [rbp+rdi+1859775393]
	xor	eax, r13d
	add	edi, ecx
	rol	r12d, 30
	add	edi, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+56]
	mov	eax, r11d
	mov	DWORD PTR [rsp+52], r14d
	mov	ecx, edi
	xor	edx, DWORD PTR [rsp]
	xor	eax, r13d
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+24]
	lea	esi, [r14+rsi+1859775393]
	xor	eax, r12d
	add	esi, ecx
	rol	r11d, 30
	add	esi, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+60]
	mov	eax, edi
	mov	DWORD PTR [rsp+56], edx
	mov	ecx, esi
	xor	ebp, DWORD PTR [rsp+4]
	xor	eax, r12d
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+28]
	lea	r13d, [rdx+r13+1859775393]
	xor	eax, r11d
	add	r13d, ecx
	rol	edi, 30
	add	r13d, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp]
	mov	eax, esi
	mov	DWORD PTR [rsp+60], ebp
	mov	ecx, r13d
	xor	r14d, DWORD PTR [rsp+8]
	xor	eax, r11d
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+32]
	lea	r12d, [rbp+r12+1859775393]
	xor	eax, edi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+4]
	mov	eax, r13d
	mov	DWORD PTR [rsp], r14d
	mov	ecx, r12d
	xor	edx, DWORD PTR [rsp+12]
	xor	eax, edi
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+36]
	lea	r11d, [r14+r11+1859775393]
	xor	eax, esi
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+8]
	mov	eax, r12d
	mov	DWORD PTR [rsp+4], edx
	mov	ecx, r11d
	xor	ebp, DWORD PTR [rsp+16]
	xor	eax, esi
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+40]
	lea	edi, [rdx+rdi+1859775393]
	xor	eax, r13d
	add	edi, ecx
	rol	r12d, 30
	add	edi, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+12]
	mov	eax, r11d
	mov	DWORD PTR [rsp+8], ebp
	mov	ecx, edi
	xor	r14d, DWORD PTR [rsp+20]
	xor	eax, r13d
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+44]
	lea	esi, [rbp+rsi+1859775393]
	xor	eax, r12d
	add	esi, ecx
	rol	r11d, 30
	add	esi, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+16]
	mov	eax, edi
	mov	DWORD PTR [rsp+12], r14d
	mov	ecx, esi
	xor	edx, DWORD PTR [rsp+24]
	xor	eax, r12d
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+48]
	lea	r13d, [r14+r13+1859775393]
	xor	eax, r11d
	add	r13d, ecx
	rol	edi, 30
	add	r13d, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+20]
	mov	eax, esi
	mov	DWORD PTR [rsp+16], edx
	mov	ecx, r13d
	xor	ebp, DWORD PTR [rsp+28]
	xor	eax, r11d
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+52]
	lea	r12d, [rdx+r12+1859775393]
	xor	eax, edi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+24]
	mov	eax, r13d
	mov	DWORD PTR [rsp+20], ebp
	mov	ecx, r12d
	xor	r14d, DWORD PTR [rsp+32]
	xor	eax, edi
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+56]
	lea	r11d, [rbp+r11+1859775393]
	xor	eax, esi
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+28]
	mov	eax, r12d
	mov	DWORD PTR [rsp+24], r14d
	mov	ecx, r11d
	xor	edx, DWORD PTR [rsp+36]
	xor	eax, esi
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+60]
	lea	edi, [r14+rdi+1859775393]
	xor	eax, r13d
	add	edi, ecx
	rol	r12d, 30
	add	edi, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+32]
	mov	eax, r11d
	mov	DWORD PTR [rsp+28], edx
	mov	ecx, edi
	xor	ebp, DWORD PTR [rsp+40]
	xor	eax, r13d
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp]
	lea	esi, [rdx+rsi+1859775393]
	xor	eax, r12d
	add	esi, ecx
	rol	r11d, 30
	add	esi, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+36]
	mov	eax, r12d
	mov	DWORD PTR [rsp+32], ebp
	mov	ebx, r12d
	xor	r14d, DWORD PTR [rsp+44]
	and	eax, r11d
	mov	ecx, esi
	xor	r14d, DWORD PTR [rsp+4]
	lea	r13d, [rbp+r13-1894007588]
	xor	ebx, r11d
	rol	ecx, 5
	add	r13d, eax
	rol	r14d, 1
	and	ebx, edi
	add	r13d, ecx
	rol	edi, 30
	add	r13d, ebx
	xor	edx, DWORD PTR [rsp+40]
	mov	eax, r11d
	mov	DWORD PTR [rsp+36], r14d
	mov	ebx, r11d
	xor	edx, DWORD PTR [rsp+48]
	and	eax, edi
	mov	ecx, r13d
	xor	edx, DWORD PTR [rsp+8]
	lea	r12d, [r14+r12-1894007588]
	xor	ebx, edi
	rol	ecx, 5
	add	r12d, eax
	rol	edx, 1
	and	ebx, esi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, ebx
	xor	ebp, DWORD PTR [rsp+44]
	mov	eax, edi
	mov	DWORD PTR [rsp+40], edx
	mov	ebx, edi
	xor	ebp, DWORD PTR [rsp+52]
	and	eax, esi
	mov	ecx, r12d
	xor	ebp, DWORD PTR [rsp+12]
	lea	r11d, [rdx+r11-1894007588]
	xor	ebx, esi
	rol	ecx, 5
	add	r11d, eax
	rol	ebp, 1
	and	ebx, r13d
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, ebx
	xor	r14d, DWORD PTR [rsp+48]
	mov	eax, esi
	mov	DWORD PTR [rsp+44], ebp
	mov	ebx, esi
	xor	r14d, DWORD PTR [rsp+56]
	and	eax, r13d
	mov	ecx, r11d
	xor	r14d, DWORD PTR [rsp+16]
	lea	edi, [rbp+rdi-1894007588]
	xor	ebx, r13d
	rol	ecx, 5
	add	edi, eax
	rol	r14d, 1
	and	ebx, r12d
	add	edi, ecx
	rol	r12d, 30
	add	edi, ebx
	xor	edx, DWORD PTR [rsp+52]
	mov	eax, r13d
	mov	DWORD PTR [rsp+48], r14d
	mov	ebx, r13d
	xor	edx, DWORD PTR [rsp+60]
	and	eax, r12d
	mov	ecx, edi
	xor	edx, DWORD PTR [rsp+20]
	lea	esi, [r14+rsi-1894007588]
	xor	ebx, r12d
	rol	ecx, 5
	add	esi, eax
	rol	edx, 1
	and	ebx, r11d
	add	esi, ecx
	rol	r11d, 30
	add	esi, ebx
	xor	ebp, DWORD PTR [rsp+56]
	mov	eax, r12d
	mov	DWORD PTR [rsp+52], edx
	mov	ebx, r12d
	xor	ebp, DWORD PTR [rsp]
	and	eax, r11d
	mov	ecx, esi
	xor	ebp, DWORD PTR [rsp+24]
	lea	r13d, [rdx+r13-1894007588]
	xor	ebx, r11d
	rol	ecx, 5
	add	r13d, eax
	rol	ebp, 1
	and	ebx, edi
	add	r13d, ecx
	rol	edi, 30
	add	r13d, ebx
	xor	r14d, DWORD PTR [rsp+60]
	mov	eax, r11d
	mov	DWORD PTR [rsp+56], ebp
	mov	ebx, r11d
	xor	r14d, DWORD PTR [rsp+4]
	and	eax, edi
	mov	ecx, r13d
	xor	r14d, DWORD PTR [rsp+28]
	lea	r12d, [rbp+r12-1894007588]
	xor	ebx, edi
	rol	ecx, 5
	add	r12d, eax
	rol	r14d, 1
	and	ebx, esi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, ebx
	xor	edx, DWORD PTR [rsp]
	mov	eax, edi
	mov	DWORD PTR [rsp+60], r14d
	mov	ebx, edi
	xor	edx, DWORD PTR [rsp+8]
	and	eax, esi
	mov	ecx, r12d
	xor	edx, DWORD PTR [rsp+32]
	lea	r11d, [r14+r11-1894007588]
	xor	ebx, esi
	rol	ecx, 5
	add	r11d, eax
	rol	edx, 1
	and	ebx, r13d
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, ebx
	xor	ebp, DWORD PTR [rsp+4]
	mov	eax, esi
	mov	DWORD PTR [rsp], edx
	mov	ebx, esi
	xor	ebp, DWORD PTR [rsp+12]
	and	eax, r13d
	mov	ecx, r11d
	xor	ebp, DWORD PTR [rsp+36]
	lea	edi, [rdx+rdi-1894007588]
	xor	ebx, r13d
	rol	ecx, 5
	add	edi, eax
	rol	ebp, 1
	and	ebx, r12d
	add	edi, ecx
	rol	r12d, 30
	add	edi, ebx
	xor	r14d, DWORD PTR [rsp+8]
	mov	eax, r13d
	mov	DWORD PTR [rsp+4], ebp
	mov	ebx, r13d
	xor	r14d, DWORD PTR [rsp+16]
	and	eax, r12d
	mov	ecx, edi
	xor	r14d, DWORD PTR [rsp+40]
	lea	esi, [rbp+rsi-1894007588]
	xor	ebx, r12d
	rol	ecx, 5
	add	esi, eax
	rol	r14d, 1
	and	ebx, r11d
	add	esi, ecx
	rol	r11d, 30
	add	esi, ebx
	xor	edx, DWORD PTR [rsp+12]
	mov	eax, r12d
	mov	DWORD PTR [rsp+8], r14d
	mov	ebx, r12d
	xor	edx, DWORD PTR [rsp+20]
	and	eax, r11d
	mov	ecx, esi
	xor	edx, DWORD PTR [rsp+44]
	lea	r13d, [r14+r13-1894007588]
	xor	ebx, r11d
	rol	ecx, 5
	add	r13d, eax
	rol	edx, 1
	and	ebx, edi
	add	r13d, ecx
	rol	edi, 30
	add	r13d, ebx
	xor	ebp, DWORD PTR [rsp+16]
	mov	eax, r11d
	mov	DWORD PTR [rsp+12], edx
	mov	ebx, r11d
	xor	ebp, DWORD PTR [rsp+24]
	and	eax, edi
	mov	ecx, r13d
	xor	ebp, DWORD PTR [rsp+48]
	lea	r12d, [rdx+r12-1894007588]
	xor	ebx, edi
	rol	ecx, 5
	add	r12d, eax
	rol	ebp, 1
	and	ebx, esi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, ebx
	xor	r14d, DWORD PTR [rsp+20]
	mov	eax, edi
	mov	DWORD PTR [rsp+16], ebp
	mov	ebx, edi
	xor	r14d, DWORD PTR [rsp+28]
	and	eax, esi
	mov	ecx, r12d
	xor	r14d, DWORD PTR [rsp+52]
	lea	r11d, [rbp+r11-1894007588]
	xor	ebx, esi
	rol	ecx, 5
	add	r11d, eax
	rol	r14d, 1
	and	ebx, r13d
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, ebx
	xor	edx, DWORD PTR [rsp+24]
	mov	eax, esi
	mov	DWORD PTR [rsp+20], r14d
	mov	ebx, esi
	xor	edx, DWORD PTR [rsp+32]
	and	eax, r13d
	mov	ecx, r11d
	xor	edx, DWORD PTR [rsp+56]
	lea	edi, [r14+rdi-1894007588]
	xor	ebx, r13d
	rol	ecx, 5
	add	edi, eax
	rol	edx, 1
	and	ebx, r12d
	add	edi, ecx
	rol	r12d, 30
	add	edi, ebx
	xor	ebp, DWORD PTR [rsp+28]
	mov	eax, r13d
	mov	DWORD PTR [rsp+24], edx
	mov	ebx, r13d
	xor	ebp, DWORD PTR [rsp+36]
	and	eax, r12d
	mov	ecx, edi
	xor	ebp, DWORD PTR [rsp+60]
	lea	esi, [rdx+rsi-1894007588]
	xor	ebx, r12d
	rol	ecx, 5
	add	esi, eax
	rol	ebp, 1
	and	ebx, r11d
	add	esi, ecx
	rol	r11d, 30
	add	esi, ebx
	xor	r14d, DWORD PTR [rsp+32]
	mov	eax, r12d
	mov	DWORD PTR [rsp+28], ebp
	mov	ebx, r12d
	xor	r14d, DWORD PTR [rsp+40]
	and	eax, r11d
	mov	ecx, esi
	xor	r14d, DWORD PTR [rsp]
	lea	r13d, [rbp+r13-1894007588]
	xor	ebx, r11d
	rol	ecx, 5
	add	r13d, eax
	rol	r14d, 1
	and	ebx, edi
	add	r13d, ecx
	rol	edi, 30
	add	r13d, ebx
	xor	edx, DWORD PTR [rsp+36]
	mov	eax, r11d
	mov	DWORD PTR [rsp+32], r14d
	mov	ebx, r11d
	xor	edx, DWORD PTR [rsp+44]
	and	eax, edi
	mov	ecx, r13d
	xor	edx, DWORD PTR [rsp+4]
	lea	r12d, [r14+r12-1894007588]
	xor	ebx, edi
	rol	ecx, 5
	add	r12d, eax
	rol	edx, 1
	and	ebx, esi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, ebx
	xor	ebp, DWORD PTR [rsp+40]
	mov	eax, edi
	mov	DWORD PTR [rsp+36], edx
	mov	ebx, edi
	xor	ebp, DWORD PTR [rsp+48]
	and	eax, esi
	mov	ecx, r12d
	xor	ebp, DWORD PTR [rsp+8]
	lea	r11d, [rdx+r11-1894007588]
	xor	ebx, esi
	rol	ecx, 5
	add	r11d, eax
	rol	ebp, 1
	and	ebx, r13d
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, ebx
	xor	r14d, DWORD PTR [rsp+44]
	mov	eax, esi
	mov	DWORD PTR [rsp+40], ebp
	mov	ebx, esi
	xor	r14d, DWORD PTR [rsp+52]
	and	eax, r13d
	mov	ecx, r11d
	xor	r14d, DWORD PTR [rsp+12]
	lea	edi, [rbp+rdi-1894007588]
	xor	ebx, r13d
	rol	ecx, 5
	add	edi, eax
	rol	r14d, 1
	and	ebx, r12d
	add	edi, ecx
	rol	r12d, 30
	add	edi, ebx
	xor	edx, DWORD PTR [rsp+48]
	mov	eax, r13d
	mov	DWORD PTR [rsp+44], r14d
	mov	ebx, r13d
	xor	edx, DWORD PTR [rsp+56]
	and	eax, r12d
	mov	ecx, edi
	xor	edx, DWORD PTR [rsp+16]
	lea	esi, [r14+rsi-1894007588]
	xor	ebx, r12d
	rol	ecx, 5
	add	esi, eax
	rol	edx, 1
	and	ebx, r11d
	add	esi, ecx
	rol	r11d, 30
	add	esi, ebx
	xor	ebp, DWORD PTR [rsp+52]
	mov	eax, edi
	mov	DWORD PTR [rsp+48], edx
	mov	ecx, esi
	xor	ebp, DWORD PTR [rsp+60]
	xor	eax, r12d
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+20]
	lea	r13d, [rdx+r13-899497514]
	xor	eax, r11d
	add	r13d, ecx
	rol	edi, 30
	add	r13d, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+56]
	mov	eax, esi
	mov	DWORD PTR [rsp+52], ebp
	mov	ecx, r13d
	xor	r14d, DWORD PTR [rsp]
	xor	eax, r11d
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+24]
	lea	r12d, [rbp+r12-899497514]
	xor	eax, edi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+60]
	mov	eax, r13d
	mov	DWORD PTR [rsp+56], r14d
	mov	ecx, r12d
	xor	edx, DWORD PTR [rsp+4]
	xor	eax, edi
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+28]
	lea	r11d, [r14+r11-899497514]
	xor	eax, esi
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp]
	mov	eax, r12d
	mov	DWORD PTR [rsp+60], edx
	mov	ecx, r11d
	xor	ebp, DWORD PTR [rsp+8]
	xor	eax, esi
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+32]
	lea	edi, [rdx+rdi-899497514]
	xor	eax, r13d
	add	edi, ecx
	rol	r12d, 30
	add	edi, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+4]
	mov	eax, r11d
	mov	DWORD PTR [rsp], ebp
	mov	ecx, edi
	xor	r14d, DWORD PTR [rsp+12]
	xor	eax, r13d
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+36]
	lea	esi, [rbp+rsi-899497514]
	xor	eax, r12d
	add	esi, ecx
	rol	r11d, 30
	add	esi, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+8]
	mov	eax, edi
	mov	DWORD PTR [rsp+4], r14d
	mov	ecx, esi
	xor	edx, DWORD PTR [rsp+16]
	xor	eax, r12d
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+40]
	lea	r13d, [r14+r13-899497514]
	xor	eax, r11d
	add	r13d, ecx
	rol	edi, 30
	add	r13d, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+12]
	mov	eax, esi
	mov	DWORD PTR [rsp+8], edx
	mov	ecx, r13d
	xor	ebp, DWORD PTR [rsp+20]
	xor	eax, r11d
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+44]
	lea	r12d, [rdx+r12-899497514]
	xor	eax, edi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+16]
	mov	eax, r13d
	mov	DWORD PTR [rsp+12], ebp
	mov	ecx, r12d
	xor	r14d, DWORD PTR [rsp+24]
	xor	eax, edi
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+48]
	lea	r11d, [rbp+r11-899497514]
	xor	eax, esi
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+20]
	mov	eax, r12d
	mov	DWORD PTR [rsp+16], r14d
	mov	ecx, r11d
	xor	edx, DWORD PTR [rsp+28]
	xor	eax, esi
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+52]
	lea	edi, [r14+rdi-899497514]
	xor	eax, r13d
	add	edi, ecx
	rol	r12d, 30
	add	edi, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+24]
	mov	eax, r11d
	mov	DWORD PTR [rsp+20], edx
	mov	ecx, edi
	xor	ebp, DWORD PTR [rsp+32]
	xor	eax, r13d
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+56]
	lea	esi, [rdx+rsi-899497514]
	xor	eax, r12d
	add	esi, ecx
	rol	r11d, 30
	add	esi, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+28]
	mov	eax, edi
	mov	DWORD PTR [rsp+24], ebp
	mov	ecx, esi
	xor	r14d, DWORD PTR [rsp+36]
	xor	eax, r12d
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+60]
	lea	r13d, [rbp+r13-899497514]
	xor	eax, r11d
	add	r13d, ecx
	rol	edi, 30
	add	r13d, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+32]
	mov	eax, esi
	mov	DWORD PTR [rsp+28], r14d
	mov	ecx, r13d
	xor	edx, DWORD PTR [rsp+40]
	xor	eax, r11d
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp]
	lea	r12d, [r14+r12-899497514]
	xor	eax, edi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+36]
	mov	eax, r13d
	mov	ecx, r12d
	xor	ebp, DWORD PTR [rsp+44]
	xor	eax, edi
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+4]
	lea	r11d, [rdx+r11-899497514]
	xor	eax, esi
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+40]
	mov	eax, r12d
	mov	ecx, r11d
	xor	r14d, DWORD PTR [rsp+48]
	xor	eax, esi
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+8]
	lea	edi, [rbp+rdi-899497514]
	xor	eax, r13d
	add	edi, ecx
	rol	r12d, 30
	add	edi, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+44]
	mov	eax, r11d
	mov	ecx, edi
	xor	edx, DWORD PTR [rsp+52]
	xor	eax, r13d
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+12]
	lea	esi, [r14+rsi-899497514]
	xor	eax, r12d
	add	esi, ecx
	rol	r11d, 30
	add	esi, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+48]
	mov	eax, edi
	mov	ecx, esi
	xor	ebp, DWORD PTR [rsp+56]
	xor	eax, r12d
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+16]
	lea	r13d, [rdx+r13-899497514]
	xor	eax, r11d
	add	r13d, ecx
	rol	edi, 30
	add	r13d, eax
	rol	ebp, 1
	xor	r14d, DWORD PTR [rsp+52]
	mov	eax, esi
	mov	ecx, r13d
	xor	r14d, DWORD PTR [rsp+60]
	xor	eax, r11d
	rol	ecx, 5
	xor	r14d, DWORD PTR [rsp+20]
	lea	r12d, [rbp+r12-899497514]
	xor	eax, edi
	add	r12d, ecx
	rol	esi, 30
	add	r12d, eax
	rol	r14d, 1
	xor	edx, DWORD PTR [rsp+56]
	mov	eax, r13d
	mov	ecx, r12d
	xor	edx, DWORD PTR [rsp]
	xor	eax, edi
	rol	ecx, 5
	xor	edx, DWORD PTR [rsp+24]
	lea	r11d, [r14+r11-899497514]
	xor	eax, esi
	add	r11d, ecx
	rol	r13d, 30
	add	r11d, eax
	rol	edx, 1
	xor	ebp, DWORD PTR [rsp+60]
	mov	eax, r12d
	mov	ecx, r11d
	xor	ebp, DWORD PTR [rsp+4]
	xor	eax, esi
	rol	ecx, 5
	xor	ebp, DWORD PTR [rsp+28]
	lea	edi, [rdx+rdi-899497514]
	xor	eax, r13d
	add	edi, ecx
	rol	r12d, 30
	add	edi, eax
	rol	ebp, 1
	mov	eax, r11d
	mov	ecx, edi
	xor	eax, r13d
	lea	esi, [rbp+rsi-899497514]
	rol	ecx, 5
	xor	eax, r12d
	add	esi, ecx
	rol	r11d, 30
	add	esi, eax
	add	esi, DWORD PTR [r8]
	add	edi, DWORD PTR [r8+4]
	add	r11d, DWORD PTR [r8+8]
	add	r12d, DWORD PTR [r8+12]
	add	r13d, DWORD PTR [r8+16]
	mov	DWORD PTR [r8], esi
	mov	DWORD PTR [r8+4], edi
	mov	DWORD PTR [r8+8], r11d
	mov	DWORD PTR [r8+12], r12d
	mov	DWORD PTR [r8+16], r13d
	sub	r10, 1
	lea	r9, [r9+64]
	jnz	Lloop
	mov	rsi, QWORD PTR [rsp+64]
	mov	r14, QWORD PTR [rsi-40]
	mov	r13, QWORD PTR [rsi-32]
	mov	r12, QWORD PTR [rsi-24]
	mov	rbp, QWORD PTR [rsi-16]
	mov	rbx, QWORD PTR [rsi-8]
	lea	rsp, [rsi]
	pop	rsi
	pop	rdi
Lepilogue:
	DB	0F3h, 0C3h
sha1_block_asm_data_order ENDP
END
