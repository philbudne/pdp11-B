/ get constant

.globl c, ic
ic:	mov	(r3)+,-2(r5)
	jmp	*(r3)+
c:	mov	(r3)+,(r5)+
	jmp	*(r3)+
