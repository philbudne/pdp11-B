/ B library -- char

.globl    .char
.globl    n7

.text
.char:	.+2
	.+2
	mov	r4,r5
	cmp	(r5)+,(r5)+
	mov	(r5)+,r1
	asl	r1
	add	(r5),r1
	movb	(r1),r0
	mov	r0,(r5)+
	jmp	n7
