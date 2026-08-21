#ifndef ATOMS_IRQ_FLAGS_H
#define ATOMS_IRQ_FLAGS_H

#include <stdint.h>

typedef uint64_t irq_flags_t;

/**
 * irq_save - Atomically save RFLAGS and disable interrupts
 * @return Saved RFLAGS value containing prior IF state
 */
static inline irq_flags_t irq_save(void) {
    irq_flags_t flags;
    __asm__ volatile (
        "pushfq\n\t"
        "popq %0\n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );
    return flags;
}

/**
 * irq_restore - Restore previously saved RFLAGS (restores IF to previous state)
 * @flags: Saved RFLAGS value from irq_save()
 */
static inline void irq_restore(irq_flags_t flags) {
    __asm__ volatile (
        "pushq %0\n\t"
        "popfq"
        :
        : "r"(flags)
        : "memory", "cc"
    );
}

#endif // ATOMS_IRQ_FLAGS_H
