ENTRY(mrk_entry)

PHDRS {
  none   PT_NULL FLAGS(0);
  text   PT_LOAD FLAGS((1 << 2) | (1 << 0)  /* Readable | Executable */);
  rodata PT_LOAD FLAGS((1 << 2)             /* Readable */);
  data   PT_LOAD FLAGS((1 << 2) | (1 << 1)  /* Readable | Writeable  */);
}

/* For aarch64, it is required that the kernel is aligned on 64KB bounds
 * Whereas on x86-64, bounds are only 4KB */

SECTIONS {
  . = 0xFFFFFFFF80200000;

  . = ALIGN(64K);
  .text . : {
    __ktext_begin = .;
    *(.text*)
    __ktext_end = .;
  }:text
  
  . = ALIGN(64K);
  .rodata . : {
    __krdata_begin = .;
    *(.rodata*)
    __krdata_end = .;
  }:rodata
  
  . = ALIGN(64K);
  .stivale2hdr : {
    KEEP(*(.stivale2hdr))
  }:rodata

  . = ALIGN(64K);
  .data . : {
    __kdata_begin = .;
    *(.data*)
    __kdata_end = .;
  }:data

  . = ALIGN(64K);
  .bss . : {
    __kbss_begin = .;
    __stack_low = .;
    . += 16K;
    __stack_high = .;
    *(.bss*)
    __kbss_end = .;
  }
}

