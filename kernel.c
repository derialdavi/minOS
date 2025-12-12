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
extern char __kernel_base[];
struct process procs[PROCS_MAX];
struct process *current_proc; // Currently running process
struct process *idle_proc;    // Idle process

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
        // Retrieve the kernel stack of the running process from sscratch.
        "csrrw sp, sscratch, sp\n"

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

        // Reset the kernel stack.
        "addi a0, sp, 4 * 31\n"
        "csrw sscratch, a0\n"

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

void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags)
{
    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %x", vaddr);

    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %x", paddr);

    // Select first 10 bits starting from left
    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
    if ((table1[vpn1] & PAGE_V) == 0)
    {
        // Create first level page if it doesn't exist
        uint32_t pt_addr = alloc_pages(1);
        table1[vpn1] = ((pt_addr / PAGE_SIZE) << 10) | PAGE_V;
    }

    // Set the 2nd level page table entry to map the physical page
    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
    uint32_t *table0 = (uint32_t *) ((table1[vpn1] >> 10) * PAGE_SIZE);
    table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

__attribute__((naked))
void context_switch(uint32_t *prev_sp, uint32_t *next_sp)
{
    __asm__ __volatile__(
        // Save callee-saved registers onto the current process's stack.
        "addi sp, sp, -13 * 4\n"
        "sw ra,  0  * 4(sp)\n"
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"

        // Switch the stack pointer.
        "sw sp, (a0)\n" // *prev_sp = sp;
        "lw sp, (a1)\n" // sp = *next_sp;

        // Restore callee-saved registers from the next process's stack.
        "lw ra,  0  * 4(sp)\n"
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"
        "ret\n"
    );
}

struct process *create_process(uint32_t pc)
{
    // Find first unused process control structure
    struct process *p = NULL;
    size_t i;
    for (i = 0; i < PROCS_MAX; i++)
    {
        if (procs[i].state == PROC_UNUSED)
        {
            p = &procs[i];
            break;
        }
    }

    if (p == NULL) PANIC("No free process slot!");

    // Stack callee-saved registers. These register values will be restored in
    // the first context switch in switch_context.
    uint32_t *sp = (uint32_t *) &p->stack[sizeof(p->stack)];
    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    *--sp = (uint32_t) pc;          // ra

    // Map kernel pages
    uint32_t *page_table = (uint32_t *) alloc_pages(1);
    for (paddr_t paddr = (paddr_t) __kernel_base; paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE)
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);

    p->pid = i+1;
    p->state = PROC_RUNNABLE;
    p->sp = (uint32_t) sp;
    p->page_table = page_table;

    return p;
}

void delay(void)
{
    for (int i = 0; i < 30000000; i++)
        __asm__ __volatile__("nop"); // do nothing
}

void scheduler(void)
{
    // Search for a runnable process
    struct process *next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++)
    {
        struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0)
        {
            next = proc;
            break;
        }
    }

    // If there's no runnable process other than the current one, return and continue processing
    if (next == current_proc)
        return;

    // Update stack pointer for new process
    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
        "csrw sscratch, %[sscratch]\n"
        :
        : [satp] "r" (SATP_SV32 | ((uint32_t) next->page_table / PAGE_SIZE)),
          [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
    );

    // Context switch
    struct process *prev = current_proc;
    current_proc = next;
    context_switch(&prev->sp, &next->sp);
}

struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        delay();
        scheduler();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        delay();
        scheduler();
    }
}

void kernel_main(void)
{
    // Writes into .bss section value 0, some bootloaders do it automatically
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    // Sets register `stvec` to point at `kernel_entry`, so when an exception
    // is generated the next instructions will be those strarting from address
    // stored in `stvec`
    WRITE_CSR(stvec, (uint32_t) kernel_entry);

    // Process to run when there are no processes available
    idle_proc = create_process((uint32_t) NULL);
    idle_proc->pid = 0; // idle
    current_proc = idle_proc;

    printf("Starting address: 0x%x\n", __free_ram);
    printf("Last address: 0x%x\n", __free_ram_end);

    paddr_t addr = alloc_pages(2);
    printf("Allocated 2 pages starting from: 0x%x\n", addr);
    addr = alloc_pages(1);
    printf("Allocated new page starting from: 0x%x\n", addr);

    // Starting two processes and switching between them
    proc_a = create_process((uint32_t) proc_a_entry);
    proc_b = create_process((uint32_t) proc_b_entry);

    scheduler();

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