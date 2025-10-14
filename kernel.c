#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

/* Linker script symbols
 *   __bss[]: start address of the `.bss` section
 *   __bss_end[]: final address of the `.bss` section
 *   __stack_top[]: first available address after kernel code and data
 */
extern char __bss[], __bss_end[], __stack_top[];

/* Sets register value to invoke a sbi call. Registers `a0` and `a1` can be changed when invoking
 * the call because in those registers will be eventually write an error value or the value returned
 * by the call. Those values are stored in this kernel with the structure `sbiret`. */
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3,
    long arg4, long arg5, long fid, long eid)
{
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__(
        "ecall"
        : "=r"(a0), "=r"(a1)                  // Stores "return" values in `a0` and `a1`
        : "r"(a0), "r"(a1), "r"(a2), "r"(a3), /* Loads in every register the value passed */
          "r"(a4), "r"(a5), "r"(a6), "r"(a7)  /* as argument for that register. */
        : "memory"
    );

    return (struct sbiret){.error = a0, .value = a1};
}

// Set values to registers to invoke `Console Putchar` to print to the debug console
// the passed character `c`
void putchar(char c)
{
    sbi_call(c, 0, 0, 0, 0, 0, 0, 1);
}

void kernel_main(void)
{
    // Writes into .bss section value 0, some bootloaders do it automatically
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    printf("\nHello %s\n", "World!");
    printf("1 + 2 = %d\n", 1 + 2);
    printf("Pointer: %x\n", 0x1234abcd);

    for (;;)
        __asm__ __volatile__("wfi");
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