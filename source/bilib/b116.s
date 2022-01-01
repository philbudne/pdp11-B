/ =% operator

.globl b116
b116:	mov	-(r5),r0
	jsr	pc,cksto
	mov	(r1),(r2)
	mov	r0,DIV
	mov	-(r2),(r1)
	mov	(r2)+,(r5)+
	jmp	*(r3)+
