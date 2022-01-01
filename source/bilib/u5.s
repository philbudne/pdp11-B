/ unary ++x

.globl u5
u5:	jsr	pc,cksto
	inc	(r1)
	mov	(r1),(r5)+
	jmp	*(r3)+
