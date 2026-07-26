#include "spinlock.h"
#include "../../../arch/x86_64/smp/smp.h"
#include "../../debug/phase7_reliability.h"

static uint64_t irq_save(void) {
  uint64_t flags;
  __asm__ volatile("pushfq; pop %0; cli" : "=r"(flags) : : "memory");
  return flags;
}

static void irq_restore(uint64_t flags) {
  __asm__ volatile("push %0; popfq" : : "r"(flags) : "memory", "cc");
}

void atoms_spinlock_init(atoms_spinlock_t *lock, uint16_t rank) {
  if (!lock)
    return;
  lock->value = 0;
  lock->owner_cpu = ATOMS_LOCK_NO_OWNER;
  lock->rank = rank;
  lock->flags = 0;
  lock->acquisitions = 0;
  lock->contentions = 0;
  lock->spins = 0;
}

bool atoms_spin_try_lock(atoms_spinlock_t *lock) {
  if (!lock || __sync_lock_test_and_set(&lock->value, 1U))
    return false;
  __sync_synchronize();
  lock->owner_cpu = atoms_cpu_id();
  ++lock->acquisitions;
  return true;
}

void atoms_spin_lock(atoms_spinlock_t *lock) {
  if (!lock)
    return;
  if (atoms_spin_is_owned_by_current_cpu(lock)) {
    atoms_p7_lock_recursion(lock->rank);
    /* Common kernel locks are intentionally non-recursive. Fail closed rather
       than silently granting recursive ownership. */
    __asm__ volatile("cli");
    while (1)
      __asm__ volatile("hlt");
  }
  (void)atoms_p7_lock_can_acquire(lock->rank);
  bool contended = false;
  uint64_t local_spins = 0;
  while (__sync_lock_test_and_set(&lock->value, 1U)) {
    contended = true;
    while (lock->value) {
      ++lock->spins;
      ++local_spins;
      if (local_spins == atoms_p7_config()->lock_spin_limit)
        atoms_p7_lock_timeout(lock->rank, local_spins);
      __asm__ volatile("pause" : : : "memory");
    }
  }
  __sync_synchronize();
  lock->owner_cpu = atoms_cpu_id();
  ++lock->acquisitions;
  if (contended)
    ++lock->contentions;
  atoms_p7_lock_acquired(lock->rank, contended, local_spins);
}

void atoms_spin_unlock(atoms_spinlock_t *lock) {
  if (!lock || !atoms_spin_is_owned_by_current_cpu(lock))
    return;
  atoms_p7_lock_released(lock->rank);
  lock->owner_cpu = ATOMS_LOCK_NO_OWNER;
  __sync_synchronize();
  __sync_lock_release(&lock->value);
}

atoms_irq_lock_state_t atoms_spin_lock_irqsave(atoms_spinlock_t *lock) {
  atoms_irq_lock_state_t state = {.interrupt_flags = irq_save()};
  atoms_spin_lock(lock);
  return state;
}

void atoms_spin_unlock_irqrestore(atoms_spinlock_t *lock,
                                  atoms_irq_lock_state_t state) {
  atoms_spin_unlock(lock);
  irq_restore(state.interrupt_flags);
}

bool atoms_spin_is_owned_by_current_cpu(const atoms_spinlock_t *lock) {
  return lock && lock->value && lock->owner_cpu == atoms_cpu_id();
}
