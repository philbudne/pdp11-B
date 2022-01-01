.globl cksto
cksto:	mov	(sp)+,r1
	mov	2f,-(r1)
	mov	1f,-(r1)
	jmp	(r1)
1:	mov	-(r5),r1
2:	asl	r1
