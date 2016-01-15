        .section ".text.__mulsi3"
__mulsi3:       .global __mulsi3
        move.w 4(%sp), %d0
        mulu.w 10(%sp), %d0
        move.w 6(%sp), %d1
        mulu.w 8(%sp), %d1
        add.w %d1, %d0
        swap %d0
        clr.w %d0
        move.w 6(%sp), %d1
        mulu.w 10(%sp), %d1
        add.l %d1, %d0
        rts

        .section ".text.__umodsi3"
__umodsi3:      .global __umodsi3
        move.l 8(%sp), %d1
        move.l 4(%sp), %d0
        move.l %d1, -(%sp)
        move.l %d0, -(%sp)
        bsr __udivsi3
        addq.l #8, %sp

        move.l 8(%sp), %d1
        move.l %d1, -(%sp)
        move.l %d0, -(%sp)
        bsr __mulsi3
        addq.l #8, %sp

        move.l 4(%sp), %d1
        sub.l %d0, %d1
        move.l %d1, %d0
        rts

/*
        .section ".text.__umodsi3"
__umodsi3:      .global __umodsi3
        movem.l 4(%sp), %d0/%d1
        movem.l %d0/%d1, -(%sp)
        bsr __udivsi3
        addq.l #8, %sp

        move.l 8(%sp), %d1
        movem.l %d0/%d1, -(%sp)
        bsr __mulsi3
        addq.l #8, %sp

        neg.l %d0
        add.l 4(%sp), %d0
        rts
*/

        .section ".text.__udivsi3"
__udivsi3:      .global __udivsi3
        move.l %d2, -(%sp)
        move.l 12(%sp), %d1
        movel. 8(%sp), %d0
        cmp.l #0x10000, %d1
        bcc.s 3f
        move.l %d0, %d2
        clr.w %d2
        swap %d2
        divu %d1, %d2
        move.w %d0, %d0
        swap %d0
        move.w 10(%sp), %d2
        divu %d1, %d2
        move.w %d2, %d0
        bra.s 6f
3:      move.l %d1, %d2
4:      lsr.l #1, %d0
        lsr.l #1, %d0
        cmp.l #0x10000, %d1
        bcc.s 4b
        divu %d1, %d0
        and.l #0xffff, %d0
        move.l %d2, %d1
        mulu %d0, %d1
        swap %d2
        mulu %d0, %d2
        swap %d2
        tst.w %d2
        bne.s 5f
        add.l %d2, %d1
        bcs.s 5f
        cmp.l 8(%sp), %d1
        bls.s 6f
5:      subq.l #1, %d0
6:      move.l (%sp)+, %d2
        rts
