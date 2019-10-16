bits 64
global antiVM_2:function,antiVM_3:function

section .text

antiVM_2:
	mov rax,0x40000000
	xor rbx,rbx
	xor rcx,rcx
	xor rdx,rdx
	cpuid
	xor eax,eax
	add edx,ecx
	xor edx, ebx
	cmp edx,0xd3bf8bbf
	jnz not_under_VM
	inc eax
not_under_VM:
	retn
antiVM_2_end:

antiVM_3:
	xor ebx,ebx
	mov eax,0x564d5868
	mov ecx,0x0a
	mov edx,0x5658
	in eax,dx
	inc ebx
	mov ax,bx
	retn
antiVM_3_end:
	
	
