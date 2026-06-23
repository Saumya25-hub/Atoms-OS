#include "libbos/include/bos.h"

void _start() {
    bos_print("\n[TEST.ELF] Hello from a dynamically spawned process!\n");
    bos_print("[TEST.ELF] My job here is done. Exiting...\n");
    bos_exit();
}
