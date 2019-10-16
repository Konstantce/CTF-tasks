
#include "stdafx.h"
#include <Windows.h>
#include <string.h>
#define N 2

// TODO: for bytes (sizeof int) is too small to hold result, let's enlarge
int trigger=0;

LONG WINAPI FILTER(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	DWORD code=ExceptionInfo->ExceptionRecord->ExceptionCode;
	DWORD* _eip=&(ExceptionInfo->ContextRecord->Eip);
	switch(code)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		//always point to initialization
		*_eip=0x40119a;
		break;
	case EXCEPTION_SINGLE_STEP:
		//points to start if byte_ptr[esi]=0 otherxase points to exit
		if (*(char*)(ExceptionInfo->ContextRecord->Esi))
			*_eip+=0x18;
		*_eip+=0x14;
		trigger=0;
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		//always points to cycle
		*_eip-=0x4a;
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		//if esi != edx - jump to cycle, utherwhise to sum_start
		if (ExceptionInfo->ContextRecord->Esi == ExceptionInfo->ContextRecord->Edx)
			*_eip+=0x4d;
		*_eip-=0xc;
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		//if esi !=edx or edx !=ebx jump to sum_1, otherwise to final
		//do metamorphs here
		if ((ExceptionInfo->ContextRecord->Edx == ExceptionInfo->ContextRecord->Esi) ||
			(ExceptionInfo->ContextRecord->Edx == ExceptionInfo->ContextRecord->Ebx))
		{
			*_eip-=0x2a;
			*(int*)(*_eip)^=0xb090f2da;
			*(int*)(*_eip+4)^=0x25ef7a16;
			trigger=37;
		}
		else
		{
			*_eip+=trigger-0x8;
			trigger=0;
		}
		break;
	case EXCEPTION_BREAKPOINT:
		{
		//always jump to end(trap)
			*_eip-=0x1e;
		}
	}
	return -1;
}

char* str1="I will give you the flag, but it can be time-consuming...\n";
char* str2="THE FLAG IS:\n";
char* str3="%x%x%x\n";

void __declspec(naked) main()
{
	_asm
	{
		;print some messages
		push ebp
		mov ebp,esp
		push str1
		call dword ptr[printf]
		push str2
		call dword ptr[printf]
		add esp,8

		;generation of an array of bytes,saving ptr in esi
		mov eax,N
		inc eax
		mov edx,2
		mul edx
		dec eax
		inc edx
		push edx
		push eax
		call dword ptr[calloc]
		mov esi,eax
		add esp,8

		;save place for result and zero the memory
		xor edx,edx
		push edx
		push edx
		push edx
		
		;1-st stage
		;we will use CFG (control flow flattering)
		;at the end of each basic blok we will pass execution flow
		;to our exception-filter
		;it will take different branches according on the exception code
		;generates access_violation

		nop
		push FILTER
		push edx
		jmp dword ptr[SetUnhandledExceptionFilter]

		;4-nd stage
		;also uses edx
		;generates zero_division
cycle:
		dec edx
		xor ecx,ecx
		cmp al,0
		jz pass
		adc byte ptr[edx],al
		setz al
pass:
		div ecx
		
		;2-nd stage
		;first basic block-initialization;
		;sets esi to point to the beginning of arr
		;ebx-points to middle, edi - points to end
		;generates single_step
initialization:
		mov ebx,N
		mov eax,ebx
		lea ebx,[esi+ebx]
		mov edx,2
		imul edx
		lea edi,[esi+eax]
end:
		pushfd
		or dword ptr [esp],0x100
		popfd
		xor ecx,ecx

		
		;6-th stage
final2:
		mov eax,ecx
		pop ecx
		cmp eax,ecx
		jnz end
		xor eax,eax
		inc dword ptr [ebp-0xc]
		adc [ebp-0x8],eax
		adc [ebp-0x4],eax
		nop
		int 3

		;3-rd stage
		;uses edx to point to current position in arr
		;generates illegal_instruction
start:
		mov edx,edi
		xor eax,eax
		inc byte ptr [edx]
		setz al
		__emit(0x0f)
		__emit(0x3f)
		__emit(0x07)


		;5-th stage
		;eax and ecx are counters here
		;generates priv_instruction

sum_start:
		mov edx,ebx
		;summing block
sum1:
		movzx eax,byte ptr [edx]
		add ecx,eax
		dec edx
		cmp edx,esi
		invd
exit:
		push esi
		call dword ptr[free]
		lea eax,[ebp-0xc]
		mov edx,4
		push [eax]
		add eax,edx
		push [eax]
		add eax,edx
		push [eax]
		push str3
		call dword ptr[printf]
		call dword ptr[getchar]
		add esp,0x20
		pop ebp
		retn
	}
}
				

