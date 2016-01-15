OUTPUT_FORMAT("elf32-m68k", "elf32-m68k", "elf32-m68k");
OUTPUT_ARCH(m68k);

SECTIONS {

.text.exec.library 0x00F80000 : {
      src/exec.a(.text.exec$ROMTAG);
      src/exec.a(.text);
      src/exec.a(.text.*);
      src/exec.a(.rodata.*);
      . += 1;
      . = ALIGN(2);
      exec$END = .;
}
/*
.text.test.hook : {
      src/test.a(.text.test$ROMTAG);
      src/test.a(.text);
      src/test.a(.text.*);
      src/test.a(.rodata.*);
      . += 1;
      . = ALIGN(2);
      test$END = .;
}
*/
/* There really shouldn't be anything in the data and BSS sections */

.text : {
      __text_extra = .;
      *(.gnu.linkonce.t.*);
      *(.rodata);
      *(.rodata.*);
      __text_end = .;
} = 0xaa55aa55

.data 0x00001000 : AT ( ADDR(.text) + SIZEOF(.text) ) {
      __data_start = .;
      *(.data);
      *(.data.*);
      __data_end = .;

      __ctors_start = .;
      *(.ctor*)
      __ctors_end = .;

      __dtors_start = .;
      *(.dtor*)
      __dtors_end = .;

} = 0xaa55aa55

.bss : {
     __bss_start = .;
     *(.common)
     *(.bss)
     __bss_end = .;
}

}