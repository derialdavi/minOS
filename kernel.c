#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

/* Linker script symbols
 *   __bss[]: start address of the `.bss` section
 *   __bss_end[]: final address of the `.bss` section
 *   __stack_top[]: first available address after kernel code and data
 *   __free_ram[]: first available address for allocated memeory
 *   __free_ram_end[]: last available address for allocated memeory
 */
extern char __bss[], __bss_end[], __stack_top[], __free_ram[], __free_ram_end[];

/* Defined as a macro so that `__FILE__` and `__LINE__` gets the value of the
 * file where `PANIC` is called, not where it's defined (here). */
#define PANIC(fmt, ...)                                                        \
    do                                                                         \
    {                                                                          \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
                                                                               \
        while (1) {}                                                           \
    } while (0)

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

__attribute__((naked))
__attribute__((aligned(4)))
void kernel_entry(void)
{
    __asm__ __volatile__(
        // Saves state of registers into memory, including stack pointer.
        "csrw sscratch, sp\n"
        "addi sp, sp, -4 * 31\n"
        "sw ra,  4 * 0(sp)\n"
        "sw gp,  4 * 1(sp)\n"
        "sw tp,  4 * 2(sp)\n"
        "sw t0,  4 * 3(sp)\n"
        "sw t1,  4 * 4(sp)\n"
        "sw t2,  4 * 5(sp)\n"
        "sw t3,  4 * 6(sp)\n"
        "sw t4,  4 * 7(sp)\n"
        "sw t5,  4 * 8(sp)\n"
        "sw t6,  4 * 9(sp)\n"
        "sw a0,  4 * 10(sp)\n"
        "sw a1,  4 * 11(sp)\n"
        "sw a2,  4 * 12(sp)\n"
        "sw a3,  4 * 13(sp)\n"
        "sw a4,  4 * 14(sp)\n"
        "sw a5,  4 * 15(sp)\n"
        "sw a6,  4 * 16(sp)\n"
        "sw a7,  4 * 17(sp)\n"
        "sw s0,  4 * 18(sp)\n"
        "sw s1,  4 * 19(sp)\n"
        "sw s2,  4 * 20(sp)\n"
        "sw s3,  4 * 21(sp)\n"
        "sw s4,  4 * 22(sp)\n"
        "sw s5,  4 * 23(sp)\n"
        "sw s6,  4 * 24(sp)\n"
        "sw s7,  4 * 25(sp)\n"
        "sw s8,  4 * 26(sp)\n"
        "sw s9,  4 * 27(sp)\n"
        "sw s10, 4 * 28(sp)\n"
        "sw s11, 4 * 29(sp)\n"

        "csrr a0, sscratch\n"
        "sw a0, 4 * 30(sp)\n"

        // Calls `handle_trap` method with parameter `a0`, pointer to the last
        // memory address of the stack.
        "mv a0, sp\n"
        "call handle_trap\n"

        // Restores register values from memory
        "lw ra,  4 * 0(sp)\n"
        "lw gp,  4 * 1(sp)\n"
        "lw tp,  4 * 2(sp)\n"
        "lw t0,  4 * 3(sp)\n"
        "lw t1,  4 * 4(sp)\n"
        "lw t2,  4 * 5(sp)\n"
        "lw t3,  4 * 6(sp)\n"
        "lw t4,  4 * 7(sp)\n"
        "lw t5,  4 * 8(sp)\n"
        "lw t6,  4 * 9(sp)\n"
        "lw a0,  4 * 10(sp)\n"
        "lw a1,  4 * 11(sp)\n"
        "lw a2,  4 * 12(sp)\n"
        "lw a3,  4 * 13(sp)\n"
        "lw a4,  4 * 14(sp)\n"
        "lw a5,  4 * 15(sp)\n"
        "lw a6,  4 * 16(sp)\n"
        "lw a7,  4 * 17(sp)\n"
        "lw s0,  4 * 18(sp)\n"
        "lw s1,  4 * 19(sp)\n"
        "lw s2,  4 * 20(sp)\n"
        "lw s3,  4 * 21(sp)\n"
        "lw s4,  4 * 22(sp)\n"
        "lw s5,  4 * 23(sp)\n"
        "lw s6,  4 * 24(sp)\n"
        "lw s7,  4 * 25(sp)\n"
        "lw s8,  4 * 26(sp)\n"
        "lw s9,  4 * 27(sp)\n"
        "lw s10, 4 * 28(sp)\n"
        "lw s11, 4 * 29(sp)\n"
        "lw sp,  4 * 30(sp)\n"
        "sret\n"
    );
}

// I could comment the parameter and the passing of it when doing `mv a0, sp` in `kernel_entry`
// since it is unused now, but it will be useful when implementing syscalls
void handle_trap(struct trap_frame *f)
{
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    PANIC("unexpected trap scause=0x%x, stval=0x%x, sepc=0x%x\n", scause, stval, user_pc);
}

// Set values to registers to invoke `Console Putchar` to print to the debug console
// the passed character `c`
void putchar(char c)
{
    sbi_call(c, 0, 0, 0, 0, 0, 0, 1);
}

paddr_t alloc_pages(uint32_t n)
{
    static paddr_t next_available_paddr = (paddr_t) __free_ram;
    paddr_t paddr = next_available_paddr;
    next_available_paddr += n * PAGE_SIZE;

    if (next_available_paddr > (paddr_t) __free_ram_end)
        PANIC("Out of memory");

    memset((void *) paddr, 0, n * PAGE_SIZE);
    return paddr;
}

void kernel_main(void)
{
    // Writes into .bss section value 0, some bootloaders do it automatically
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    // Sets register `stvec` to point at `kernel_entry`, so when an exception
    // is generated the next instructions will be those strarting from address
    // stored in `stvec`
    WRITE_CSR(stvec, (uint32_t) kernel_entry);

    printf("Starting address: 0x%x\n", __free_ram);
    printf("Last address: 0x%x\n", __free_ram_end);

    paddr_t addr = alloc_pages(2);
    printf("Allocated 2 pages starting from: 0x%x\n", addr);
    addr = alloc_pages(1);
    printf("Allocated new page starting from: 0x%x\n", addr);

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