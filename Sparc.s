! Diag info offsets	
	
#define D_ATTACKS	 0
#define D_MOBILITY	 4
#define D_WHICH_ATTACK	 8
#define D_SHIFT		12
#define D_MASK		13					
#define AD_SHIFT	14
#define AD_MASK		15
#define AD_WHICH_ATTACK	16
#define AD_MOBILITY	20
#define AD_ATTACKS	24				

! position offsets
	
#define W_OCCUPIED	  0
#define B_OCCUPIED	  8		
#define RL90		 16	
#define RL45		 24
#define RR45		 32		
	
! struct at offsets	
#define WHICH_ATTACK	   0
#define FILE_ATTACKS	 512
#define RANK_ATTACKS	1280
#define LEN8_MOBILITY	1376
#define SHRT_MOBILITY	1472				

#define diag_info	o0
#define boardp		o1	
#define temp		o2
#define shift		o3
#define mask		o4
#define which_attack	o5	
#define attack_index	g2	
#define attack_vector	g3
#define mobility_vector	g3	
#define occupied	g4	
#define occupied_hi	g4	
#define occupied_lo	g5	
	
.text
	.align 4
	.global AttacksDiaga1Func
AttacksDiaga1Func:	
	ldub	[%diag_info + AD_SHIFT], %shift
	
	ldub	[%diag_info + AD_MASK], %mask
	cmp	%shift, 32
	blt,a	1f
	add	%boardp, 4, %boardp
1:	
	ld	[%boardp + RL45], %occupied
	
	! srl implicitly ands the shift with 0x1f
	srl	%occupied, %shift, %temp
	ld	[%diag_info + AD_WHICH_ATTACK], %which_attack
	
	ld	[%diag_info + AD_ATTACKS], %attack_vector
	and	%temp, %mask, %temp
	
	ldub	[%which_attack + %temp], %attack_index
	
	sll	%attack_index, 3, %attack_index
	
	retl	
	
	ldd	[%attack_vector + %attack_index], %o0
	
	
	.align 4
	.global AttacksDiagh1Func
AttacksDiagh1Func:	
	ldub	[%diag_info + D_SHIFT], %shift
	
	ldub	[%diag_info + D_MASK], %mask
	cmp	%shift, 32
	blt,a	1f
	add	%boardp, 4, %boardp
1:
	ld	[%boardp + RR45], %occupied
	
	! srl implicitly ands the shift with 0x1f
	srl	%occupied, %shift, %temp
	ld	[%diag_info + D_WHICH_ATTACK], %which_attack
	
	and	%temp, %mask, %temp
	ld	[%diag_info + D_ATTACKS], %attack_vector
	
	ldub	[%which_attack + %temp], %attack_index
	
	sll	%attack_index, 3, %attack_index
	
	retl	
	
	ldd	[%attack_vector + %attack_index], %o0
	
	
	.align 4
	.global AttacksBishopFunc
AttacksBishopFunc:	
	ldub	[%diag_info + AD_SHIFT], %shift
	mov	%boardp, %temp

	ldub	[%diag_info + AD_MASK], %mask
	cmp	%shift, 32
	blt,a	1f
	add	%temp, 4, %temp
	
1:	
	ld	[%temp + RL45], %occupied
	
	! srl implicitly ands the shift with 0x1f
	srl	%occupied, %shift, %temp
	ld	[%diag_info + AD_WHICH_ATTACK], %which_attack
	
	and	%temp, %mask, %temp
	ld	[%diag_info + AD_ATTACKS], %attack_vector
	
	ldub	[%which_attack + %temp], %attack_index
	
	sll	%attack_index, 3, %attack_index
	ldub	[%diag_info + D_SHIFT], %shift
	
	ldd	[%attack_vector + %attack_index], %occupied
	
	ldub	[%diag_info + D_MASK], %mask
	cmp	%shift, 32	
	blt,a	1f
	add	%boardp, 4, %boardp
1:	
	ld	[%boardp + RR45], %temp
	
	! srl implicitly ands the shift with 0x1f
	srl	%temp, %shift, %temp
	ld	[%diag_info + D_WHICH_ATTACK], %which_attack
	
	and	%temp, %mask, %temp
	ld	[%diag_info + D_ATTACKS], %attack_vector
	
	ldub	[%which_attack + %temp], %attack_index
	
	sll	%attack_index, 3, %attack_index
	
	ldd	[%attack_vector + %attack_index], %o0
	
	or	%occupied_hi, %o0, %o0
	retl
	or	%occupied_lo, %o1, %o1
	
	.align 4
	.global MobilityDiaga1Func
MobilityDiaga1Func:	
	ldub	[%diag_info + AD_SHIFT], %shift
	
	ldub	[%diag_info + AD_MASK], %mask
	cmp	%shift, 32
	blt,a	1f
	add	%boardp, 4, %boardp
1:	
	ld	[%boardp + RL45], %occupied
	
	! srl implicitly ands the shift with 0x1f
	srl	%occupied, %shift, %temp
	ld	[%diag_info + AD_WHICH_ATTACK], %which_attack
	
	and	%temp, %mask, %temp
	ld	[%diag_info + AD_MOBILITY], %mobility_vector
	
	ldub	[%which_attack + %temp], %attack_index
	
	retl	
	
	ldub	[%mobility_vector + %attack_index], %o0
	
	
	.align 4
	.global MobilityDiagh1Func
MobilityDiagh1Func:	
	ldub	[%diag_info + D_SHIFT], %shift
	
	ldub	[%diag_info + D_MASK], %mask
	cmp	%shift, 32
	blt,a	1f
	add	%boardp, 4, %boardp
1:
	ld	[%boardp + RR45], %occupied
	
	! srl implicitly ands the shift with 0x1f
	srl	%occupied, %shift, %temp
	ld	[%diag_info + D_WHICH_ATTACK], %which_attack
	
	and	%temp, %mask, %temp
	ld	[%diag_info + D_MOBILITY], %mobility_vector
	
	ldub	[%which_attack + %temp], %attack_index
	
	retl	
	
	ldub	[%mobility_vector + %attack_index], %o0
	
#undef diag_info
#undef boardp
#undef temp
#undef shift
#undef mask
#undef which_attack
#undef attack_index
#undef attack_vector
#undef mobility_vector
#undef occupied
#undef occupied_hi
#undef occupied_lo


#define	sq		o0
#define att		o0
#define boardp		o1
#define file		o2
#define rankc		o3
#define filec		o4
#define rank		o5
#define	rank12		g1
#define file12		g1
#define occupied	g2
#define occupied_hi	g2
#define occupied_lo	g3
#define occupied2	g4
#define attack_index	g4
#define attack		g4
#define temp3		g4
#define temp		g5
	
	.align	4
	.global AttacksRankFunc
AttacksRankFunc:	
	and	%sq, 7, %file
	xnor	%sq, 0, %rankc
	
	and	%rankc, (7 << 3), %rankc
	sll	%file, 1, %temp
	
	add	%file, %temp, %file12
	add 	%rankc, 1, %temp
	
	sll	%file, 6, %file
	cmp	%rankc, 32
	blt,a	1f
	add	%boardp, 4, %boardp
1:	
	sll	%file12, 2, %file12
	ld	[%boardp + W_OCCUPIED], %occupied
	
	sethi	%hi(at), %att
	ld	[%boardp + B_OCCUPIED], %occupied2
	
	or	%occupied, %occupied2, %occupied
	or	%att, %lo(at), %att
	
	! srl implicitly ands the shift with 0x1f
	srl	%occupied, %temp, %temp
	add	%file, %att, %file
	
	and	%temp, 0x3f, %temp
	add	%file12, %att, %file12
	
	! add	%temp, %file, %temp
	! ldub	[%temp + WHICH_ATTACK], %attack_index
	ldub	[%temp + %file], %attack_index
	add	%file12, RANK_ATTACKS, %file12

	clr	%o0
	clr	%o1
	bge	1f
	
	ldub	[%attack_index + %file12], %attack

	retl
	sll	%attack, %rankc, %o1

1:	retl
	sll	%attack, %rankc, %o0

	
	.global	AttacksFileFunc
AttacksFileFunc:	
	srl	%sq, 3, %rank
	xnor	%sq, 0, %filec
	
	sll	%rank, 1, %temp
	and	%filec, 7, %filec
	
	add	%rank, %temp, %rank12
	sll	%filec, 3, %temp

	add	%temp, 1, %temp
	sll	%rank12, 5, %rank12
	
	sll	%rank, 6, %rank
	cmp	%temp, 32
	blt,a	1f
	add	%boardp, 4, %boardp
1:	
	sethi	%hi(at), %att
	ld	[%boardp + RL90], %occupied
	
	! srl implicitly ands the shift with 0x1f
	srl	%occupied, %temp, %temp
	or	%att, %lo(at), %att
	
	and	%temp, 0x3f, %temp
	add	%rank, %att, %rank
	
	! add	%temp, %rank, %temp
	! ldub	[%temp + WHICH_ATTACK], %attack_index
	add	%rank12, FILE_ATTACKS, %rank12
	ldub	[%temp + %rank], %attack_index

	sll	%attack_index, 3, %attack_index
	add	%rank12, %att, %rank12
	
	ldd	[%attack_index + %rank12], %o0
	
	sll	%o0, %filec, %o0
	retl
	sll	%o1, %filec, %o1

	
	.global AttacksRookFunc	
AttacksRookFunc:
	and	%sq, 7, %file
	xnor	%sq, 0, %rankc
	
	srl	%sq, 3, %rank
	xnor	%sq, 0, %filec
	
	and	%rankc, (7 << 3), %rankc
	sll	%file, 1, %temp
	
	add	%file, %temp, %file12
	sethi	%hi(at), %att

	mov	%boardp, %temp3
	cmp	%rankc, 32
	blt,a	1f
	add	%temp3, 4, %temp3
1:		
	sll	%file, 6, %file
	ld	[%temp3 + W_OCCUPIED], %occupied
	
	or	%att, %lo(at), %att
	ld	[%temp3 + B_OCCUPIED], %occupied2

	sll	%file12, 2, %file12
	add 	%rankc, 1, %temp

	add	%file, %att, %file
	or	%occupied, %occupied2, %occupied
	
	! srl implicitly ands the shift with 0x1f
	srl	%occupied, %temp, %temp
	add	%file12, %att, %file12
	
	and	%temp, 0x3f, %temp
	add	%file12, RANK_ATTACKS, %file12
	
	! add	%temp, %file, %temp
	! ldub	[%temp + WHICH_ATTACK], %attack_index
	ldub	[%temp + %file], %attack_index
	
	and	%filec, 7, %filec
	sll	%rank, 1, %temp
	bge	1f
	
	ldub	[%attack_index + %file12], %attack

	clr	%occupied_hi
	b 2f
	sll	%attack, %rankc, %occupied_lo

1:	sll	%attack, %rankc, %occupied_hi
	clr	%occupied_lo
	
2:
	add	%rank, %temp, %rank12
	sll	%filec, 3, %temp
	
	sll	%rank12, 5, %rank12
	add	%temp, 1, %temp

	sll	%rank, 6, %rank
	cmp	%temp, 32
	blt,a	1f
	add	%boardp, 4, %boardp
1:
	ld	[%boardp + RL90], %temp3
	add	%rank12, %att, %rank12
	
	! srl implicitly ands the shift with 0x1f
	srl	%temp3, %temp, %temp
	add	%rank, %att, %rank
	
	and	%temp, 0x3f, %temp
	
	! add	%temp, %rank, %temp
	! ldub	[%temp + WHICH_ATTACK], %attack_index
	ldub	[%temp + %rank], %attack_index

	sll	%attack_index, 3, %attack_index
	add	%rank12, FILE_ATTACKS, %rank12
	
	ldd	[%attack_index + %rank12], %o0

	sll	%o1, %filec, %o1
	or	%o1, %occupied_lo, %o1
	
	sll	%o0, %filec, %o0
	retl
	or	%o0, %occupied_hi, %o0

	.align	4
	.global MobilityRankFunc
MobilityRankFunc:	
	and	%sq, 7, %file
	xnor	%sq, 0, %rankc
	
	and	%rankc, (7 << 3), %rankc
	sll	%file, 1, %temp
	
	add	%file, %temp, %file12
	add 	%rankc, 1, %temp
	
	sll	%file, 6, %file
	cmp	%rankc, 32
	blt,a	1f
	add	%boardp, 4, %boardp
1:	
	sll	%file12, 2, %file12
	ld	[%boardp + W_OCCUPIED], %occupied

	sethi	%hi(at), %att
	ld	[%boardp + B_OCCUPIED], %occupied2
	
	or	%occupied, %occupied2, %occupied
	or	%att, %lo(at), %att
	
	! srl implicitly ands the shift with 0x1f
	srl	%occupied, %temp, %temp
	add	%file, %att, %file
	
	and	%temp, 0x3f, %temp
	add	%file12, %att, %file12
	
	! add	%temp, %file, %temp
	! ldub	[%temp + WHICH_ATTACK], %attack_index
	ldub	[%temp + %file], %attack_index
	add	%file12, LEN8_MOBILITY, %file12
	
	retl
	
	ldub	[%attack_index + %file12], %o0

	
	.global	MobilityFileFunc
MobilityFileFunc:	
	srl	%sq, 3, %rank
	xnor	%sq, 0, %filec
	
	sll	%rank, 1, %temp
	and	%filec, 7, %filec
	
	add	%rank, %temp, %rank12
	sll	%filec, 3, %temp

	add	%temp, 1, %temp
	sll	%rank12, 2, %rank12
	
	sll	%rank, 6, %rank
	cmp	%temp, 32
	blt,a	1f
	add	%boardp, 4, %boardp
1:	
	sethi	%hi(at), %att
	ld	[%boardp + RL90], %occupied
	
	! srl implicitly ands the shift with 0x1f
	srl	%occupied, %temp, %temp
	or	%att, %lo(at), %att
	
	and	%temp, 0x3f, %temp
	add	%rank, %att, %rank
	
	! add	%temp, %rank, %temp
	! ldub	[%temp + WHICH_ATTACK], %attack_index
	ldub	[%temp + %rank], %attack_index
	add	%rank12, %att, %rank12

	add	%rank12, LEN8_MOBILITY, %rank12
	retl
	
	ldub	[%attack_index + %rank12], %o0
	
#undef	sq
#undef boardp
#undef file
#undef temp2
#undef rankc
#undef filec
#undef rank
#undef temp
#undef at
#undef occupied
#undef occupied_hi
#undef occupied_lo
#undef occupied2
#undef attack_index
#undef attack
#undef temp3
