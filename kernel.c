typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

/* Linker script symbols
 *   __bss[]: start address of the `.bss` section
 *   __bss_end[]: final address of the `.bss` section
 *   __stack_top[]: first available address after kernel code and data
 */
extern char __bss[], __bss_end[], __stack_top[];

// Writes character `c` in memory addresses `buf`...`buf`+`n`
void *memset(void *buf, char c, size_t n)
{
    uint8_t *p = (uint8_t *) buf;

    while (n--)
        *p++ = c;

    return buf;
}

void kernel_main(void)
{
    // Writes into .bss section value 0, some bootloaders do it automatically
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    for (;;);
}

// Starting point
__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void)
{
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" // Set stack pointer address
        "j kernel_main\n"       // Jump to kernel_main label
        :
        : [stack_top] "r" (__stack_top)
    );
}