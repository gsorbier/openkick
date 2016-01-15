/* -*- mode: asm; coding: latin-1 -*- */
        .section ".text.exec$ROMTAG"

        /*
        This is the first bit of code that appears in the Kickstart ROM, which appears at 0xF80000
	(and 0xFC0000 if it's a 256kB ROM). On reset, the OVL pin of CIAA is also low, and this also
	causes the ROM to appear at location 0 instead of Chip RAM. The 680x0 CPU takes its initial
	%sp and %pc from locations 0 and 4 on reset, and thus the values below cause execution from
	location 0xf800d2.

        MapROM will refuse to shadow this images that don't start with the folowing 12 byte
	signature:
        */

        .word   0x1114           /*               1114 */
        .word   0x4ef9,0xf8,0xd2 /* jmp 0xF800D2  4EF9 00F8 00D2 */
        .long   0xffff           /*               0000 FFFF */
        
        /*
        The following four bytes are the version number of the Kickstart ROM.

        MapROM requires this version to be different to that of your real Kickstart ROM or it will
	decline to load it (because it assumes it's already mapped.) For my Kickstart 3.0 A4000, the
	values here are 39,106,39,47.

        UAE requires the version numbers to be at least 34 (Kickstart 1.3) or it will decline to
	provide emulated AutoConfig(TM) hardware.
        */

        .word   34,0,34,0


        /* Official Commodore ROMs contain a copyright message here, followed by the exec.library
	RomTag structure. We interleave these a bit. */

                                /* drop in our RomTag structure */ 
exec$ROMTAG: .global exec$ROMTAG
        .word 0x4afc            /* RomTag "matchword" (magic number) */
        .long exec$ROMTAG       /* pointer to the matchword */
        .long exec$END          /* pointer to end of module */
        .byte 0                 /* flags */
        .byte 34                /* library version */
        .byte 9                 /* type, NT_LIBRARY */
        .byte 120               /* priority */
        .long exec$NAME         /* library name */
        .long exec$IDSTRING     /* library ID string */
        .long 0f                /* library init */
        .asciz "\nOpenkick\n(c) 2013 Peter Corlett\nAll Rights Reserved\n"
        .align 2

        .org 0x64               
        /* initialise CIAA to map the Chip RAM */
0:      move.b  #3, 0xbfe201    /* DDRA: /LED and OVL set to outputs */
        move.b  #2, 0xbfe001    /* PRA: clear /LED and OVL to enable RAM and dim LED */

        /* Turn off interrupts and DMA */
        lea 0xdff180, %a5
        move.l #0x7fff7fff, %d0
        move.w %d0, 0x96-0x180(%a5)
        move.l %d0, 0x9a-0x180(%a5)

        /* Now set up the display so that it just displays the solid colour stuffed into the COLOR00
	register */

        move.w #0x200, 0x100-0x180(%a5) /* BPLCON0: no bitplanes */
        clr.w 0x110-0x180(%a5)          /* BPLDAT0: no data */
        move.w #0x333, (%a5)            /* COLOR00: set dark grey background */

        lea 0x1000.w, %sp       /* stick SP at top of first page of RAM for now */
        
        /* initialise exception vectors to point to failure code */
        move.w #253, %d0
        lea 2f(%pc), %a0
        lea 8.w, %a1
1:      move.l %a0, (%a1)+
        dbra %d0, 1b

        /* Now we call _init(), which returns an allocated chunk of memory for the supervisor stack,
	and set %ssp to that value. _init() needs to setup %usp. */
        bsr _init
        move.l %d0, %a0
        move.l %a0, %ssp
        and.w #0, %sr
        move.w #0x666, (%a5)    /* set middling grey background */
        bra _init2

        /* A CPU exception occurred, so put up a yellow screen. */
2:      move.w #0xff0, 0xdff180
        stop #0x2700
        bra.s 2b

        /* When MapROM has mapped the ROM image, it jumps to 0xF800D0, rather than following the
	"official" Commodore reset code. The Kickstart ROM has a RESET instruction here, so that's
	what we do as well. */
        .org 0xd0
        reset

        /* On reset, the CPU will end up at 0xF800D2 either due to falling through from the RESET
	instruction above, or because that's what the CPU reset vector pointed to. We branch to the
	early startup code above. */
        .org 0xd2
        bra.s 0b
