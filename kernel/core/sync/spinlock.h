#ifndef ATOMS_KERNEL_SPINLOCK_H
#define ATOMS_KERNEL_SPINLOCK_H

#include <stdbool.h>
#include <stdint.h>

#define ATOMS_LOCK_NO_OWNER UINT32_MAX

typedef struct {
  volatile uint32_t value;
  uint32_t owner_cpu;
  uint16_t rank;
  uint16_t flags;
  uint64_t acquisitions;
  uint64_t contentions;
  uint64_t spins;
} atoms_spinlock_t;

typedef struct {
  uint64_t interrupt_flags;
} atoms_irq_lock_state_t;

void atoms_spinlock_init(atoms_spinlock_t *lock, uint16_t rank);
bool atoms_spin_try_lock(atoms_spinlock_t *lock);
void atoms_spin_lock(atoms_spinlock_t *lock);
void atoms_spin_unlock(atoms_spinlock_t *lock);
atoms_irq_lock_state_t atoms_spin_lock_irqsave(atoms_spinlock_t *lock);
void atoms_spin_unlock_irqrestore(atoms_spinlock_t *lock,
                                  atoms_irq_lock_state_t state);
bool atoms_spin_is_owned_by_current_cpu(const atoms_spinlock_t *lock);

#endif
