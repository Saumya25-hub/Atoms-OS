# Design Goals

1. Preserve source compatibility for tracked and aligned heap entry points.
2. Prevent allocation recursion through strict public/backend layering.
3. Recover from pressure through bounded reclaim and contiguous VMM-backed growth.
4. Keep all AMSSS metadata statically allocated.
5. Preserve corruption fail-stop behavior while making ordinary OOM recoverable.
6. Avoid claims about paging, NUMA, or compaction facilities absent from ATOMS OS.
