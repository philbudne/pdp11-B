/ =+ operator

.globl b114
b114:	mov	-(r5),r0
	jsr	pc,cksto
	add	r0,(r1)
	mov	(r1),(r5)+
	jmp	*(r3)+
