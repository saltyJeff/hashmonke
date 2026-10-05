.code
PUBLIC md5_block_asm_data_order
ALIGN 16
md5_block_asm_data_order PROC
ALIGN 16
	push	rdi
	push	rsi
	mov	rdi, rcx
	mov	rsi, rdx
	mov	rdx, r8
	push	rbp
	push	rbx
	push	r12
	push	r14
	push	r15
Lprologue:
	mov	rbp, rdi
	shl	rdx, 6
	lea	rdi, [rsi+rdx]
	mov	eax, DWORD PTR [rbp]
	mov	ebx, DWORD PTR [rbp+4]
	mov	ecx, DWORD PTR [rbp+8]
	mov	edx, DWORD PTR [rbp+12]
	cmp	rsi, rdi
	je	Lend
Lloop:
	mov	r8d, eax
	mov	r9d, ebx
	mov	r14d, ecx
	mov	r15d, edx
	mov	r10d, DWORD PTR [rsi]
	mov	r11d, edx
	xor	r11d, ecx
	lea	eax, [rax+r10-680876936]
	and	r11d, ebx
	xor	r11d, edx
	mov	r10d, DWORD PTR [rsi+4]
	add	eax, r11d
	rol	eax, 7
	mov	r11d, ecx
	add	eax, ebx
	xor	r11d, ebx
	lea	edx, [rdx+r10-389564586]
	and	r11d, eax
	xor	r11d, ecx
	mov	r10d, DWORD PTR [rsi+8]
	add	edx, r11d
	rol	edx, 12
	mov	r11d, ebx
	add	edx, eax
	xor	r11d, eax
	lea	ecx, [rcx+r10+606105819]
	and	r11d, edx
	xor	r11d, ebx
	mov	r10d, DWORD PTR [rsi+12]
	add	ecx, r11d
	rol	ecx, 17
	mov	r11d, eax
	add	ecx, edx
	xor	r11d, edx
	lea	ebx, [rbx+r10-1044525330]
	and	r11d, ecx
	xor	r11d, eax
	mov	r10d, DWORD PTR [rsi+16]
	add	ebx, r11d
	rol	ebx, 22
	mov	r11d, edx
	add	ebx, ecx
	xor	r11d, ecx
	lea	eax, [rax+r10-176418897]
	and	r11d, ebx
	xor	r11d, edx
	mov	r10d, DWORD PTR [rsi+20]
	add	eax, r11d
	rol	eax, 7
	mov	r11d, ecx
	add	eax, ebx
	xor	r11d, ebx
	lea	edx, [rdx+r10+1200080426]
	and	r11d, eax
	xor	r11d, ecx
	mov	r10d, DWORD PTR [rsi+24]
	add	edx, r11d
	rol	edx, 12
	mov	r11d, ebx
	add	edx, eax
	xor	r11d, eax
	lea	ecx, [rcx+r10-1473231341]
	and	r11d, edx
	xor	r11d, ebx
	mov	r10d, DWORD PTR [rsi+28]
	add	ecx, r11d
	rol	ecx, 17
	mov	r11d, eax
	add	ecx, edx
	xor	r11d, edx
	lea	ebx, [rbx+r10-45705983]
	and	r11d, ecx
	xor	r11d, eax
	mov	r10d, DWORD PTR [rsi+32]
	add	ebx, r11d
	rol	ebx, 22
	mov	r11d, edx
	add	ebx, ecx
	xor	r11d, ecx
	lea	eax, [rax+r10+1770035416]
	and	r11d, ebx
	xor	r11d, edx
	mov	r10d, DWORD PTR [rsi+36]
	add	eax, r11d
	rol	eax, 7
	mov	r11d, ecx
	add	eax, ebx
	xor	r11d, ebx
	lea	edx, [rdx+r10-1958414417]
	and	r11d, eax
	xor	r11d, ecx
	mov	r10d, DWORD PTR [rsi+40]
	add	edx, r11d
	rol	edx, 12
	mov	r11d, ebx
	add	edx, eax
	xor	r11d, eax
	lea	ecx, [rcx+r10-42063]
	and	r11d, edx
	xor	r11d, ebx
	mov	r10d, DWORD PTR [rsi+44]
	add	ecx, r11d
	rol	ecx, 17
	mov	r11d, eax
	add	ecx, edx
	xor	r11d, edx
	lea	ebx, [rbx+r10-1990404162]
	and	r11d, ecx
	xor	r11d, eax
	mov	r10d, DWORD PTR [rsi+48]
	add	ebx, r11d
	rol	ebx, 22
	mov	r11d, edx
	add	ebx, ecx
	xor	r11d, ecx
	lea	eax, [rax+r10+1804603682]
	and	r11d, ebx
	xor	r11d, edx
	mov	r10d, DWORD PTR [rsi+52]
	add	eax, r11d
	rol	eax, 7
	mov	r11d, ecx
	add	eax, ebx
	xor	r11d, ebx
	lea	edx, [rdx+r10-40341101]
	and	r11d, eax
	xor	r11d, ecx
	mov	r10d, DWORD PTR [rsi+56]
	add	edx, r11d
	rol	edx, 12
	mov	r11d, ebx
	add	edx, eax
	xor	r11d, eax
	lea	ecx, [rcx+r10-1502002290]
	and	r11d, edx
	xor	r11d, ebx
	mov	r10d, DWORD PTR [rsi+60]
	add	ecx, r11d
	rol	ecx, 17
	mov	r11d, eax
	add	ecx, edx
	xor	r11d, edx
	lea	ebx, [rbx+r10+1236535329]
	and	r11d, ecx
	xor	r11d, eax
	mov	r10d, DWORD PTR [rsi]
	add	ebx, r11d
	rol	ebx, 22
	mov	r11d, edx
	add	ebx, ecx
	mov	r10d, DWORD PTR [rsi+4]
	mov	r11d, edx
	mov	r12d, edx
	not	r11d
	lea	eax, [rax+r10-165796510]
	and	r12d, ebx
	and	r11d, ecx
	mov	r10d, DWORD PTR [rsi+24]
	add	eax, r11d
	mov	r11d, ecx
	add	eax, r12d
	mov	r12d, ecx
	rol	eax, 5
	add	eax, ebx
	not	r11d
	lea	edx, [rdx+r10-1069501632]
	and	r12d, eax
	and	r11d, ebx
	mov	r10d, DWORD PTR [rsi+44]
	add	edx, r11d
	mov	r11d, ebx
	add	edx, r12d
	mov	r12d, ebx
	rol	edx, 9
	add	edx, eax
	not	r11d
	lea	ecx, [rcx+r10+643717713]
	and	r12d, edx
	and	r11d, eax
	mov	r10d, DWORD PTR [rsi]
	add	ecx, r11d
	mov	r11d, eax
	add	ecx, r12d
	mov	r12d, eax
	rol	ecx, 14
	add	ecx, edx
	not	r11d
	lea	ebx, [rbx+r10-373897302]
	and	r12d, ecx
	and	r11d, edx
	mov	r10d, DWORD PTR [rsi+20]
	add	ebx, r11d
	mov	r11d, edx
	add	ebx, r12d
	mov	r12d, edx
	rol	ebx, 20
	add	ebx, ecx
	not	r11d
	lea	eax, [rax+r10-701558691]
	and	r12d, ebx
	and	r11d, ecx
	mov	r10d, DWORD PTR [rsi+40]
	add	eax, r11d
	mov	r11d, ecx
	add	eax, r12d
	mov	r12d, ecx
	rol	eax, 5
	add	eax, ebx
	not	r11d
	lea	edx, [rdx+r10+38016083]
	and	r12d, eax
	and	r11d, ebx
	mov	r10d, DWORD PTR [rsi+60]
	add	edx, r11d
	mov	r11d, ebx
	add	edx, r12d
	mov	r12d, ebx
	rol	edx, 9
	add	edx, eax
	not	r11d
	lea	ecx, [rcx+r10-660478335]
	and	r12d, edx
	and	r11d, eax
	mov	r10d, DWORD PTR [rsi+16]
	add	ecx, r11d
	mov	r11d, eax
	add	ecx, r12d
	mov	r12d, eax
	rol	ecx, 14
	add	ecx, edx
	not	r11d
	lea	ebx, [rbx+r10-405537848]
	and	r12d, ecx
	and	r11d, edx
	mov	r10d, DWORD PTR [rsi+36]
	add	ebx, r11d
	mov	r11d, edx
	add	ebx, r12d
	mov	r12d, edx
	rol	ebx, 20
	add	ebx, ecx
	not	r11d
	lea	eax, [rax+r10+568446438]
	and	r12d, ebx
	and	r11d, ecx
	mov	r10d, DWORD PTR [rsi+56]
	add	eax, r11d
	mov	r11d, ecx
	add	eax, r12d
	mov	r12d, ecx
	rol	eax, 5
	add	eax, ebx
	not	r11d
	lea	edx, [rdx+r10-1019803690]
	and	r12d, eax
	and	r11d, ebx
	mov	r10d, DWORD PTR [rsi+12]
	add	edx, r11d
	mov	r11d, ebx
	add	edx, r12d
	mov	r12d, ebx
	rol	edx, 9
	add	edx, eax
	not	r11d
	lea	ecx, [rcx+r10-187363961]
	and	r12d, edx
	and	r11d, eax
	mov	r10d, DWORD PTR [rsi+32]
	add	ecx, r11d
	mov	r11d, eax
	add	ecx, r12d
	mov	r12d, eax
	rol	ecx, 14
	add	ecx, edx
	not	r11d
	lea	ebx, [rbx+r10+1163531501]
	and	r12d, ecx
	and	r11d, edx
	mov	r10d, DWORD PTR [rsi+52]
	add	ebx, r11d
	mov	r11d, edx
	add	ebx, r12d
	mov	r12d, edx
	rol	ebx, 20
	add	ebx, ecx
	not	r11d
	lea	eax, [rax+r10-1444681467]
	and	r12d, ebx
	and	r11d, ecx
	mov	r10d, DWORD PTR [rsi+8]
	add	eax, r11d
	mov	r11d, ecx
	add	eax, r12d
	mov	r12d, ecx
	rol	eax, 5
	add	eax, ebx
	not	r11d
	lea	edx, [rdx+r10-51403784]
	and	r12d, eax
	and	r11d, ebx
	mov	r10d, DWORD PTR [rsi+28]
	add	edx, r11d
	mov	r11d, ebx
	add	edx, r12d
	mov	r12d, ebx
	rol	edx, 9
	add	edx, eax
	not	r11d
	lea	ecx, [rcx+r10+1735328473]
	and	r12d, edx
	and	r11d, eax
	mov	r10d, DWORD PTR [rsi+48]
	add	ecx, r11d
	mov	r11d, eax
	add	ecx, r12d
	mov	r12d, eax
	rol	ecx, 14
	add	ecx, edx
	not	r11d
	lea	ebx, [rbx+r10-1926607734]
	and	r12d, ecx
	and	r11d, edx
	mov	r10d, DWORD PTR [rsi]
	add	ebx, r11d
	mov	r11d, edx
	add	ebx, r12d
	mov	r12d, edx
	rol	ebx, 20
	add	ebx, ecx
	mov	r10d, DWORD PTR [rsi+20]
	mov	r11d, ecx
	lea	eax, [rax+r10-378558]
	mov	r10d, DWORD PTR [rsi+32]
	xor	r11d, edx
	xor	r11d, ebx
	add	eax, r11d
	rol	eax, 4
	mov	r11d, ebx
	add	eax, ebx
	lea	edx, [rdx+r10-2022574463]
	mov	r10d, DWORD PTR [rsi+44]
	xor	r11d, ecx
	xor	r11d, eax
	add	edx, r11d
	rol	edx, 11
	mov	r11d, eax
	add	edx, eax
	lea	ecx, [rcx+r10+1839030562]
	mov	r10d, DWORD PTR [rsi+56]
	xor	r11d, ebx
	xor	r11d, edx
	add	ecx, r11d
	rol	ecx, 16
	mov	r11d, edx
	add	ecx, edx
	lea	ebx, [rbx+r10-35309556]
	mov	r10d, DWORD PTR [rsi+4]
	xor	r11d, eax
	xor	r11d, ecx
	add	ebx, r11d
	rol	ebx, 23
	mov	r11d, ecx
	add	ebx, ecx
	lea	eax, [rax+r10-1530992060]
	mov	r10d, DWORD PTR [rsi+16]
	xor	r11d, edx
	xor	r11d, ebx
	add	eax, r11d
	rol	eax, 4
	mov	r11d, ebx
	add	eax, ebx
	lea	edx, [rdx+r10+1272893353]
	mov	r10d, DWORD PTR [rsi+28]
	xor	r11d, ecx
	xor	r11d, eax
	add	edx, r11d
	rol	edx, 11
	mov	r11d, eax
	add	edx, eax
	lea	ecx, [rcx+r10-155497632]
	mov	r10d, DWORD PTR [rsi+40]
	xor	r11d, ebx
	xor	r11d, edx
	add	ecx, r11d
	rol	ecx, 16
	mov	r11d, edx
	add	ecx, edx
	lea	ebx, [rbx+r10-1094730640]
	mov	r10d, DWORD PTR [rsi+52]
	xor	r11d, eax
	xor	r11d, ecx
	add	ebx, r11d
	rol	ebx, 23
	mov	r11d, ecx
	add	ebx, ecx
	lea	eax, [rax+r10+681279174]
	mov	r10d, DWORD PTR [rsi]
	xor	r11d, edx
	xor	r11d, ebx
	add	eax, r11d
	rol	eax, 4
	mov	r11d, ebx
	add	eax, ebx
	lea	edx, [rdx+r10-358537222]
	mov	r10d, DWORD PTR [rsi+12]
	xor	r11d, ecx
	xor	r11d, eax
	add	edx, r11d
	rol	edx, 11
	mov	r11d, eax
	add	edx, eax
	lea	ecx, [rcx+r10-722521979]
	mov	r10d, DWORD PTR [rsi+24]
	xor	r11d, ebx
	xor	r11d, edx
	add	ecx, r11d
	rol	ecx, 16
	mov	r11d, edx
	add	ecx, edx
	lea	ebx, [rbx+r10+76029189]
	mov	r10d, DWORD PTR [rsi+36]
	xor	r11d, eax
	xor	r11d, ecx
	add	ebx, r11d
	rol	ebx, 23
	mov	r11d, ecx
	add	ebx, ecx
	lea	eax, [rax+r10-640364487]
	mov	r10d, DWORD PTR [rsi+48]
	xor	r11d, edx
	xor	r11d, ebx
	add	eax, r11d
	rol	eax, 4
	mov	r11d, ebx
	add	eax, ebx
	lea	edx, [rdx+r10-421815835]
	mov	r10d, DWORD PTR [rsi+60]
	xor	r11d, ecx
	xor	r11d, eax
	add	edx, r11d
	rol	edx, 11
	mov	r11d, eax
	add	edx, eax
	lea	ecx, [rcx+r10+530742520]
	mov	r10d, DWORD PTR [rsi+8]
	xor	r11d, ebx
	xor	r11d, edx
	add	ecx, r11d
	rol	ecx, 16
	mov	r11d, edx
	add	ecx, edx
	lea	ebx, [rbx+r10-995338651]
	mov	r10d, DWORD PTR [rsi]
	xor	r11d, eax
	xor	r11d, ecx
	add	ebx, r11d
	rol	ebx, 23
	mov	r11d, ecx
	add	ebx, ecx
	mov	r10d, DWORD PTR [rsi]
	mov	r11d, 0ffffffffh
	xor	r11d, edx
	lea	eax, [rax+r10-198630844]
	or	r11d, ebx
	xor	r11d, ecx
	add	eax, r11d
	mov	r10d, DWORD PTR [rsi+28]
	mov	r11d, 0ffffffffh
	rol	eax, 6
	xor	r11d, ecx
	add	eax, ebx
	lea	edx, [rdx+r10+1126891415]
	or	r11d, eax
	xor	r11d, ebx
	add	edx, r11d
	mov	r10d, DWORD PTR [rsi+56]
	mov	r11d, 0ffffffffh
	rol	edx, 10
	xor	r11d, ebx
	add	edx, eax
	lea	ecx, [rcx+r10-1416354905]
	or	r11d, edx
	xor	r11d, eax
	add	ecx, r11d
	mov	r10d, DWORD PTR [rsi+20]
	mov	r11d, 0ffffffffh
	rol	ecx, 15
	xor	r11d, eax
	add	ecx, edx
	lea	ebx, [rbx+r10-57434055]
	or	r11d, ecx
	xor	r11d, edx
	add	ebx, r11d
	mov	r10d, DWORD PTR [rsi+48]
	mov	r11d, 0ffffffffh
	rol	ebx, 21
	xor	r11d, edx
	add	ebx, ecx
	lea	eax, [rax+r10+1700485571]
	or	r11d, ebx
	xor	r11d, ecx
	add	eax, r11d
	mov	r10d, DWORD PTR [rsi+12]
	mov	r11d, 0ffffffffh
	rol	eax, 6
	xor	r11d, ecx
	add	eax, ebx
	lea	edx, [rdx+r10-1894986606]
	or	r11d, eax
	xor	r11d, ebx
	add	edx, r11d
	mov	r10d, DWORD PTR [rsi+40]
	mov	r11d, 0ffffffffh
	rol	edx, 10
	xor	r11d, ebx
	add	edx, eax
	lea	ecx, [rcx+r10-1051523]
	or	r11d, edx
	xor	r11d, eax
	add	ecx, r11d
	mov	r10d, DWORD PTR [rsi+4]
	mov	r11d, 0ffffffffh
	rol	ecx, 15
	xor	r11d, eax
	add	ecx, edx
	lea	ebx, [rbx+r10-2054922799]
	or	r11d, ecx
	xor	r11d, edx
	add	ebx, r11d
	mov	r10d, DWORD PTR [rsi+32]
	mov	r11d, 0ffffffffh
	rol	ebx, 21
	xor	r11d, edx
	add	ebx, ecx
	lea	eax, [rax+r10+1873313359]
	or	r11d, ebx
	xor	r11d, ecx
	add	eax, r11d
	mov	r10d, DWORD PTR [rsi+60]
	mov	r11d, 0ffffffffh
	rol	eax, 6
	xor	r11d, ecx
	add	eax, ebx
	lea	edx, [rdx+r10-30611744]
	or	r11d, eax
	xor	r11d, ebx
	add	edx, r11d
	mov	r10d, DWORD PTR [rsi+24]
	mov	r11d, 0ffffffffh
	rol	edx, 10
	xor	r11d, ebx
	add	edx, eax
	lea	ecx, [rcx+r10-1560198380]
	or	r11d, edx
	xor	r11d, eax
	add	ecx, r11d
	mov	r10d, DWORD PTR [rsi+52]
	mov	r11d, 0ffffffffh
	rol	ecx, 15
	xor	r11d, eax
	add	ecx, edx
	lea	ebx, [rbx+r10+1309151649]
	or	r11d, ecx
	xor	r11d, edx
	add	ebx, r11d
	mov	r10d, DWORD PTR [rsi+16]
	mov	r11d, 0ffffffffh
	rol	ebx, 21
	xor	r11d, edx
	add	ebx, ecx
	lea	eax, [rax+r10-145523070]
	or	r11d, ebx
	xor	r11d, ecx
	add	eax, r11d
	mov	r10d, DWORD PTR [rsi+44]
	mov	r11d, 0ffffffffh
	rol	eax, 6
	xor	r11d, ecx
	add	eax, ebx
	lea	edx, [rdx+r10-1120210379]
	or	r11d, eax
	xor	r11d, ebx
	add	edx, r11d
	mov	r10d, DWORD PTR [rsi+8]
	mov	r11d, 0ffffffffh
	rol	edx, 10
	xor	r11d, ebx
	add	edx, eax
	lea	ecx, [rcx+r10+718787259]
	or	r11d, edx
	xor	r11d, eax
	add	ecx, r11d
	mov	r10d, DWORD PTR [rsi+36]
	mov	r11d, 0ffffffffh
	rol	ecx, 15
	xor	r11d, eax
	add	ecx, edx
	lea	ebx, [rbx+r10-343485551]
	or	r11d, ecx
	xor	r11d, edx
	add	ebx, r11d
	mov	r10d, DWORD PTR [rsi]
	mov	r11d, 0ffffffffh
	rol	ebx, 21
	xor	r11d, edx
	add	ebx, ecx
	add	eax, r8d
	add	ebx, r9d
	add	ecx, r14d
	add	edx, r15d
	add	rsi, 64
	cmp	rsi, rdi
	jb	Lloop
Lend:
	mov	DWORD PTR [rbp], eax
	mov	DWORD PTR [rbp+4], ebx
	mov	DWORD PTR [rbp+8], ecx
	mov	DWORD PTR [rbp+12], edx
	mov	r15, QWORD PTR [rsp]
	mov	r14, QWORD PTR [rsp+8]
	mov	r12, QWORD PTR [rsp+16]
	mov	rbx, QWORD PTR [rsp+24]
	mov	rbp, QWORD PTR [rsp+32]
	add	rsp, 40
	pop	rsi
	pop	rdi
Lepilogue:
	DB	0F3h, 0C3h
md5_block_asm_data_order ENDP
END
