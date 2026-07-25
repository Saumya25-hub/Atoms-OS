# Heap Expansion

Growth starts at the current heap end and maps contiguous page-aligned virtual addresses through VMM. Partial mapping failures roll back in reverse order. The managed virtual extent is capped at 256 MiB; heap code never calls PMM directly.

