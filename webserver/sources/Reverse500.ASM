global main
extern malloc
extern free

%define input_len  64


section .text

RC4_encode:
	;input: rsi = pointer to input_string; rcx = input_string_len; rbx - RC4-key
	;rax = pointer to allocated block of memory (we will store RC4 table there)
	;output: rdi = pointer to ciphered input_string  (the same as input)
	mov rdx, rax
	xchg rdx,rdi
	xor rax, rax

	;RC4 init
generate_RC4_table:
	stosb
	inc al
	jnz generate_RC4_table
	sub rdi, 0x100

	xchg rdi,rbx
	mov r8,rax
	mov rsi, rcx
	xor rcx, rcx
	xor r8, r8

	;rdx - pointer to input string, rsi -input_string_len, rbx - mem pinter; r8 = 0 (j), rdi = RC4 key
	;rcx=0 (i), key len = 8 bytes
	;Initial RC4 permutation
rc4_init:
	mov al,cl
	xlatb ;al=S[i]
	add r8b,al; j=j+s[i]
	mov al, cl
	and al,0x7
	shl al, 3
	xchg al, cl
	ror rdi, cl
	mov r9,rdi
	add r8b,r9b ;j=j+s[i]+Key[i mod l]
	rol rdi, cl
	mov cl, al
	xlatb ;al=S[i]
	mov r10b,al    ;r10b = s[i]
	mov al,r8b
	xlatb ; al=s[j]
	lea r9, [rbx+rcx]
	mov byte[r9], al
	lea r9,[rbx+r8]
	mov byte[r9],r10b
	inc cl
	jnz rc4_init

	;begin cipher
	xchg rsi, rcx ; rsi=0 (i), rcx = input_string_len
	xchg rsi, rdx; rdx=0(i), rsi=input string
	xor r8, r8; r8=0 (j)
rc4_encode:
	inc dl
	mov al,dl
	xlatb
	add r8b,al
	mov r10b,al ;r10b=s[i]
	mov al,r8b
	xlatb ;al=s[j]
	lea r9,[rbx+rdx]
	mov byte [r9],al
	lea r9,[rbx+r8]
	mov byte [r9],r10b
	add al,r10b ;t=s[i]+s[j]
	xlat
	xor byte [rsi], al
	inc rsi
	loop rc4_encode
	retn


STRLEN:
	;input: rdi - address of zero-terminated string;
	;output: rcx - string length, rdi still points yo the string start address;
	xor rcx,rcx
	mov al,cl
	dec rcx
	repne scasb
	neg rcx
	dec rcx
	sub rdi,rcx
	dec rcx
	retn



PERMUTATE_BITS:
	;input: rsi- address of string; rcx - string length
	shr rcx, 3
outer_cycle:
	lodsq
	push rcx
	mov rcx,64
	xor rbx,rbx
inner_cycle:
	rol rax,1
	rcr rbx,1
	loop inner_cycle

	;another tarnsform just here
	mov rax,rbx
	shr rbx,1
	mov rdx,rbx
	not rbx
	and rbx,rax
	not rax
	and rax,rdx
	or rax,rbx

	bswap rax
	stosq
	pop rcx
	loop outer_cycle
	retn

DECODE_HEX:
	;response is hex-decoded, we use this function to decode it
	;input: rdi - address of string; rcx - length of string in bytes;
	shr rcx,4
	mov rsi,rdi
outer_cycle2:
	xor rdx,rdx
	xor rax,rax
	push rcx
	mov rcx,0x8
inner_cycle2:
	lodsb
	cmp al,0x30
	jb local_incorrect
	cmp al,0x3a
	jb decode_hex_end
	cmp al,0x41
	jb local_incorrect
	cmp al,0x46
	ja local_incorrect
	sub al,0x7
decode_hex_end:
	sub al,0x30
	or dl,al
	xor ah,1
	jz ror_right
	rol dl,4
	jmp inner_cycle2
ror_right:
	ror rdx,8
	loop inner_cycle2
	mov rax,rdx
	stosq
	pop rcx
	loop outer_cycle2
	xor rax, rax
	inc rax
	retn
local_incorrect:
	pop rcx
	xor rax, rax
	retn

main:
	push rbp
	mov rbp, rsp
	cld
	cmp rdi, 3 ;check if we have pass only two parameter - challenge-response authentification parts
	jne INCORRECT
	mov rdi, [rsi + 8] 
	mov rax, 2
	mov rsi, [rsi + rax*8]
	call START
INCORRECT:
	xor rax, rax
	retn



CRC32_SUM:
	;input: rdi - string, rcx - string length
	;output: rbx - CRC32 sum of string
	xor rbx, rbx
	xor rdx, rdx
	shr rcx, 3
crc32_loop:
	lodsq
	crc32 rdx,rax
	add rbx, rdx
	rol rbx, 32
	loop crc32_loop
	retn


START:
	;rdi - challenge string, rsi - response string
	mov rbx, 2
check_length:
	call STRLEN
	cmp rcx, input_len
	jne local_incorrect2
	xchg rsi,rdi
	dec bl
	jnz check_length
	;rdi still contains address of challenge and rsi contains address of response
	mov r14,rdi ;we will store challenge here
	mov r15,rsi ; we will store response here
	mov r12, rcx ;strings' length will be stored here

	mov rsi, r14
	call CRC32_SUM
	mov r11, rbx ;RC4-key is stored here

	;transform challenge string
	mov rdi, r14
	mov rsi, r14
	xor rax, rax
transform_loop:
	push rax
	call TRANSFORM
	add rsi, 0x20
	pop rax
	xor al, 1
	jnz transform_loop

	mov rsi, r15
	mov rdi, rsi
	mov rcx, r12
	call DECODE_HEX
	test rax, rax
	jz local_incorrect2

	shr r12, 1 ;now the length is half of the original

	;r13 - will always contain the pointer to allocated memory
	mov rdi,0x100
	push r11
	push r12
	call malloc
	mov r13, rax
	pop r12
	pop r11

	mov rdi, r15
	mov rsi, rdi
	mov rcx, r12
	mov rbx, r11
	call RC4_encode

	mov rdi, r13
	call free

	mov rsi,r15
	mov rdi, rsi
	mov rcx,r12
	call PERMUTATE_BITS

	;final check
	mov rdi, r14
	mov rsi, r15
	mov rcx, r12
	shr rcx, 3
	repe cmpsq
	xor rax, rax
	test rcx, rcx
	sete al
	ret
local_incorrect2:
	xor rax, rax
	retn


TRANSFORM:
	;input: rsi - address of string to transform
	;output: rdi - address of transformed string
	;Input length = 32 bytes, output length = 16 bytes.
	;This routine is designed to be non-reversible and non-invertable, ha-ha!
	push rbp
	xor rdx, rdx
	mov rax, [rsi + 8*rdx]
	inc rdx
	mov rbx, [rsi + 8*rdx]
	inc rdx
	mov rbp, [rsi + 8*rdx]
	inc rdx
	mov rdx, [rsi + 8*rdx]

	mov rcx, rbx
	ror rbp , cl
	xor rbp , rdx
	add rbx , rbp
	add rbp , rdx
	mov rcx, rbp
	ror rbx , cl
	mov rcx, rdx
	rol rax , cl
	add rdx , rbp
	xor rbx , rdx
	sub rbx , rdx
	add rdx , rbx
	xor rbp , rdx
	mov rcx, rbp
	ror rdx , cl
	add rdx , rbp
	add rax , rdx
	add rbx , rax
	mov rcx, rdx
	ror rbp , cl
	xchg rax, rbx
	mul rdx
	xchg rax, rbx
	xchg rax, rbp
	mul rax
	xchg rax, rbp
	xor rbx , rbp
	mov rcx, rax
	rol rbx , cl
	mov rcx, rax
	ror rax , cl
	sub rbx , rbp
	mov rcx, rdx
	ror rdx , cl
	xor rax , rbx
	sub rbx , rax
	mov rcx, rax
	ror rbx , cl
	mov rcx, rbp
	ror rbx , cl
	mov rcx, rdx
	ror rbp , cl
	mov rcx, rbx
	rol rbx , cl
	mov rcx, rdx
	rol rax , cl
	mov rcx, rbx
	rol rax , cl
	mov rcx, rdx
	rol rax , cl
	sub rbp , rax
	sub rdx , rbp
	mov rcx, rbx
	rol rax , cl
	xchg rax, rbx
	mul rbp
	xchg rax, rbx
	mov rcx, rdx
	ror rdx , cl
	add rbx , rax
	sub rax , rbx
	xchg rax, rbp
	mul rbx
	xchg rax, rbp
	sub rbp , rdx
	add rbx , rax
	add rbp , rax
	xchg rax, rdx
	mul rax
	xchg rax, rdx
	mov rcx, rax
	ror rax , cl
	mov rcx, rbx
	ror rdx , cl
	mov rcx, rbx
	ror rbp , cl
	add rbx , rbp
	add rdx , rax
	xchg rax, rbx
	mul rax
	xchg rax, rbx
	add rbx , rax
	xor rax , rdx
	mov rcx, rbp
	ror rax , cl
	xchg rax, rax
	mul rax
	xchg rax, rax
	xor rdx , rax
	xor rbp , rdx
	mov rcx, rbp
	rol rbx , cl
	sub rdx , rbp
	mov rcx, rbp
	ror rbx , cl
	add rdx , rbx
	xchg rax, rbx
	mul rbp
	xchg rax, rbx
	mov rcx, rdx
	ror rbx , cl
	xor rbx , rax
	mov rcx, rax
	ror rax , cl
	xchg rax, rax
	mul rbx
	xchg rax, rax
	mov rcx, rax
	rol rbp , cl
	mov rcx, rbx
	rol rax , cl
	sub rdx , rax
	mov rcx, rbp
	rol rbx , cl
	sub rbx , rdx
	mov rcx, rdx
	rol rdx , cl
	xchg rax, rbx
	mul rbp
	xchg rax, rbx
	mov rcx, rax
	rol rax , cl
	xor rax , rbx
	sub rdx , rbx
	mov rcx, rbx
	ror rbx , cl
	xor rbx , rdx
	xchg rax, rbx
	mul rbx
	xchg rax, rbx
	mov rcx, rbp
	rol rbp , cl
	mov rcx, rax
	ror rbx , cl
	mov rcx, rax
	ror rax , cl
	add rbx , rax
	xchg rax, rax
	mul rdx
	xchg rax, rax
	add rax , rbp
	xor rbx , rax
	xor rax , rbp
	mov rcx, rbx
	rol rbp , cl
	xchg rax, rbp
	mul rax
	xchg rax, rbp
	sub rax , rbx
	mov rcx, rbx
	rol rax , cl
	xor rax , rbx
	mov rcx, rbp
	ror rbx , cl
	mov rcx, rbp
	ror rax , cl
	add rdx , rbp
	xchg rax, rax
	mul rbx
	xchg rax, rax
	mov rcx, rdx
	ror rax , cl
	mov rcx, rbp
	rol rdx , cl
	sub rax , rdx
	sub rbx , rbp
	sub rbx , rax
	xchg rax, rbp
	mul rdx
	xchg rax, rbp
	mov rcx, rbp
	ror rbp , cl
	sub rax , rdx
	mov rcx, rdx
	ror rdx , cl
	xor rbx , rbp
	xor rax , rbp
	mov rcx, rbp
	rol rdx , cl
	mov rcx, rbp
	rol rdx , cl
	mov rcx, rbx
	rol rax , cl
	sub rax , rbx
	sub rdx , rbx
	xor rdx , rbx
	xor rax , rdx
	add rbp , rax
	mov rcx, rbx
	ror rdx , cl
	mov rcx, rbx
	ror rbp , cl
	xor rbp , rbx
	sub rbx , rbp
	mov rcx, rbp
	ror rbx , cl
	xor rdx , rax
	xchg rax, rbx
	mul rax
	xchg rax, rbx
	xor rbp , rdx
	xor rbx , rdx
	sub rbx , rax
	add rax , rbp
	xor rax , rbx
	mov rcx, rbx
	ror rbx , cl
	mov rcx, rbx
	rol rbx , cl
	xchg rax, rbx
	mul rax
	xchg rax, rbx
	mov rcx, rbx
	ror rbp , cl
	sub rbp , rax
	xchg rax, rbx
	mul rbp
	xchg rax, rbx
	sub rax , rbp
	xchg rax, rbx
	mul rax
	xchg rax, rbx
	add rax , rbx
	add rdx , rbx
	xor rax , rbp
	mov rcx, rbp
	ror rdx , cl
	add rdx , rbp
	xor rbp , rbx
	mov rcx, rbx
	ror rbp , cl
	mov rcx, rdx
	rol rdx , cl
	sub rbx , rax
	sub rbp , rbx
	add rdx , rax
	add rax , rdx
	xor rdx , rbp
	mov rcx, rbp
	ror rdx , cl
	mov rcx, rbx
	rol rbp , cl
	mov rcx, rbp
	rol rbx , cl
	add rbp , rax
	xchg rax, rbx
	mul rbp
	xchg rax, rbx
	sub rax , rdx
	mov rcx, rdx
	ror rbp , cl
	mov rcx, rdx
	rol rax , cl
	sub rbp , rax
	mov rcx, rdx
	rol rax , cl
	sub rbp , rbx
	sub rbp , rax
	xchg rax, rdx
	mul rax
	xchg rax, rdx
	sub rdx , rbx
	sub rax , rbp
	mov rcx, rdx
	rol rbx , cl
	xor rdx , rbp
	mov rcx, rbp
	rol rax , cl
	mov rcx, rbx
	ror rbx , cl
	add rax , rbp
	xor rdx , rbx
	add rax , rbx
	sub rax , rdx
	add rbx , rdx
	add rdx , rax
	mov rcx, rdx
	rol rdx , cl
	xchg rax, rax
	mul rax
	xchg rax, rax
	sub rbp , rdx
	mov rcx, rdx
	rol rdx , cl
	xor rax , rbp
	mov rcx, rbx
	ror rdx , cl
	mov rcx, rbx
	rol rdx , cl
	sub rax , rbp
	xchg rax, rbp
	mul rdx
	xchg rax, rbp
	xor rbx , rax
	xor rbx , rdx
	add rdx , rbp
	xchg rax, rbx
	mul rdx
	xchg rax, rbx
	xor rax , rbp
	mov rcx, rdx
	rol rdx , cl
	sub rbp , rax
	xchg rax, rdx
	mul rdx
	xchg rax, rdx
	xchg rax, rdx
	mul rax
	xchg rax, rdx
	xor rdx , rbx
	sub rdx , rbx
	mov rcx, rdx
	ror rax , cl
	sub rax , rbp
	xchg rax, rbx
	mul rax
	xchg rax, rbx
	sub rdx , rbx
	xor rbp , rbx
	mov rcx, rbp
	rol rbp , cl
	xchg rax, rbp
	mul rbp
	xchg rax, rbp
	mov rcx, rbx
	rol rbx , cl

	stosq
	xor rbx, rbp
	pop rbp
	xchg rax, rdx
	add rax, rbx
	stosq
	ret









