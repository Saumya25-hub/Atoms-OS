# Public API

The public interface provides initialization, allocation, zeroed allocation, reallocation, free, health, telemetry, budget, pressure, reclaim-provider registration, cache registration, diagnostics, forensics, and periodic ticking.

Allocation APIs accept caller RIP explicitly so legacy tracked macros retain attribution. A zero-sized allocation returns `NULL`; calloc rejects multiplication overflow; realloc preserves the old allocation when replacement fails.
