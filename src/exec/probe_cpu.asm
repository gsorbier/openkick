        .section ".text.exec$probe_cpu"
exec$probe_cpu: .global exec$probe_cpu
        /* preamble: save registers and CPU vectors on the stack, replacing vectors with a function
	that jumps to the next test. We generally assign %a6 as the link register, %a5 as the
	next-test address pointer, and %d0 for the return code. */
        
        movem.l %d2-%d7/%a2-%a5, -(%sp)
        link %a6, #(64*4)
        move.l %a6, %a0
        lea 0.w, %a1
        lea .Lexcept_handler(%pc), %a2
        move.w #64, %d1
        bra.s 2f
1:      move.l (%a1),-(%a0)
        move.l %a2, (%a1)+
2:      dbra %d1, 1b

        /* this code currently just bails on first exception, so we put the postamble pointer here */
        lea .Lpostamble(%pc), %a5

        /* we start with %d0 = 0, i.e. a plain 68000. Long clear, because although we're returning a
	16 bit value, we recycle %d0's value for some other purposes. */
        clr.l %d0

        /* let's try to put 0 into the VBR */
        movec %d0, %vbr
        /* if that didn't throw an exception, we're at least a 68010 */
        moveq #1, %d0

        /* now try to enable the 68020 CPU cache */
        movec %d0, %cacr
        /* if that didn't throw an exception, we're at least a 68020 */
        moveq #3, %d0

        /* Now try a FPU instruction */
        fmove.l %fpcr, %d1
        /* This appears to unconditionally throw an exception if no FPU is present, at least under
	UAE. Kickstart 1.2 also tests the value in %d1 is not zero, although given it already
	contained 1, it's hard to see how it gets zeroed by a nonexistent FPU. */
        bset #4.b, %d0
        
        /* postamble: unwind preamble */
.Lpostamble:
        move.l %a6, %a0
        lea 0.w, %a1
        move.w #64, %d1
        bra.s 2f
1:      move.l (%a1)+,-(%a0)
2:      dbra %d1, 1b
        unlk %a6
        movem.l (%sp)+, %d2-%d7/%a2-%a5
        rts

        /* This is the exception handler, which just calls the next-test pointer in %a5 */
.Lexcept_handler:
        jmp (%a5)
        
