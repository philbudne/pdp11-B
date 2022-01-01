/ =| operator

.globl b102
b102:	mov	-(r5),r0
	jsr	pc,cksto
	bis	r0,(r1)
	mov	(r1),(r5)+
	jmp	*(r3)+
