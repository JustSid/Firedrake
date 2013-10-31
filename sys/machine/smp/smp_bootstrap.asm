; //
; //  smp_bootstrap.asm
; //  Firedrake
; //
; //  Created by Sidney Just
; //  Copyright (c) 2013 by Sidney Just
; //  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
; //  documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
; //  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
; //  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
; //  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
; //  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
; //  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
; //  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
; //  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
; //  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
; //

[bits 16]
global smp_bootstrap_begin
smp_bootstrap_begin:

smp_entry:
	mov esi, 0xff00
	lgdt [esi]

	mov eax, cr0
	or  eax, 1
	mov cr0, eax

	jmp dword 0x8:smp_entry_protected_mode

[bits 32]
smp_entry_protected_mode:

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	mov eax, 0x1234
	jmp eax

smp_gdtr:
	dw 24
	dd smp_gdt

smp_gdt:
	dw 0x0
	dw 0x0
	dw 0x0
	dw 0x0

	dw 0xffff
	dw 0x0000
	dw 0x9800
	dw 0x00cf

	dw 0xffff 
	dw 0x0000
	dw 0x9800
	dw 0x00cf

global smp_bootstrap_end
smp_bootstrap_end:
