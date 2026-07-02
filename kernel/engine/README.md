# Engine Module

## Purpose
The central execution engine (Horse Engine) of ATOMS OS. It handles high-performance lookups, application launching, and resource management without becoming a God Object. It strictly decouples UI actions from Kernel primitives.

## Public API
- `horse_init()`
- `horse_dispatch()`
- `horse_search()`
- `horse_launch()`

## Dependencies
- Scheduler (for task creation)
- VFS (for fast path lookups)

## TODO
- Implement binary lookup table for APP_ID mapping.
- Establish memory/cpu resource limits per application.
