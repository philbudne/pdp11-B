/ if !

.globl f
f:	mov	(r3)+,r0	/ get argument
	mov	-(r5),(r5)+	/ check top of stack
	bne	1f
	mov	r0,r3		/ jump to target if == 0
1:	jmp	*(r3)+
