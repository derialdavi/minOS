ENTRY(boot)

SECTIONS {
    . = 0x80200000; /* Base address for OpenSBI */

    /* Defining memory areas for code, read only data, initialized data and uninitialized data.
       When assining a new area the location `.` is automatically updated adding the size of the
       new area, so in this case `.rodata` will start at the address immediatley after the last
       data of `.text`.
       `*(...);` means to consider every specified section of every input file. */
    .text :{
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    . = ALIGN(4); /* Padding */
    . += 128 * 1024;
    __stack_top = .;
}