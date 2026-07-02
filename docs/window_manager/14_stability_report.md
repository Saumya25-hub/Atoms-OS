# Stability Report

**Overall Stability Rating for Legacy WM:** 4.5 / 10

## Category Breakdown

- **Architecture (3/10)**
  - *Why*: Deeply entangled. `BOSurface` knows about `terminal.h`. `BWE` overlaps heavily with `BOSurface`. The boundaries are broken.
  
- **Maintainability (4/10)**
  - *Why*: Code is split across randomly named folders (`BOSurface` vs `bwe`). Monolithic `BWE_Window` struct makes adding new control types very difficult without bloating everything.
  
- **Coupling (2/10)**
  - *Why*: High circular coupling. Low-level UI headers directly `#include` high-level application headers. 
  
- **Rendering (7/10)**
  - *Why*: `BOCompositor` is actually quite solid. The damage tracking and occlusion culling algorithms are mathematically sound and mostly decoupled from UI logic.
  
- **Input / Events (6/10)**
  - *Why*: Ring buffer queue design is safe and standard. However, Hit Testing is somewhat tightly coupled to the UI definitions.
  
- **Memory (5/10)**
  - *Why*: Zero dynamic memory prevents leaks (Good). However, the `BWE_Window` union bloat wastes massive amounts of BSS static memory (Bad).
  
- **Scalability (3/10)**
  - *Why*: Hardcoded static arrays (`1024` windows, `128` surfaces, `128` children limit) means the OS cannot scale to complex desktop environments. O(N^2) insertion sort during render loop limits the max window count.
  
- **Debuggability (5/10)**
  - *Why*: Has basic `bwe_log` traces and the `bodebug_dump()` function, but lacks runtime asserts or safe pointer validation bounds checking.

- **Documentation (0/10)**
  - *Why*: Almost entirely undocumented in the codebase prior to this audit. No `README.md` files existed for the modules.
