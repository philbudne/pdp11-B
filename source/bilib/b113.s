/ =<< operator

.globl b113
b113:	mov	-(r5),r0
	jsr	pc,cksto
	mov	(r1),(r2)
	mov	r0,LSH
	mov	(r2),(r1)
	mov	(r1),(r5)+
	jmp	*(r3)+
