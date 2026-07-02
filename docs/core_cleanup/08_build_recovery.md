# Build Recovery

## Problem
During Phase 3, the build will break spectacularly. Moving hundreds of files out of their legacy locations will invalidate every `#include` path in the OS.

## Cause
The transition to the Foundation v2 folder structure physically disrupts the paths expected by the C preprocessor.

## Solution
1. **Move Phase**: Physically move all files to their target directories (`core`, `wm`, `ui`, etc.). Do not attempt to compile.
2. **Search & Replace Phase**: Use regex tools to globally update include paths. For example, `#include "../BOSurface/Core/surface.h"` becomes `#include "kernel/wm/surface.h"`.
3. **Linker/Makefile Update**: Update the build system (Makefiles, linker scripts, or whatever build tool ATOMS uses) to include the new source directories.
4. **Iterative Fixing**: Run the compiler. Fix the first error. Recompile. Repeat until the build succeeds.

## Risk
A missed file or an incorrectly updated include path will result in a "File not found" compilation error, preventing the OS from booting.

## Verification
The compiler must exit with code 0 (Success). All symbols must be linked successfully without undefined reference errors.

## Next Step
Summarize the successful execution of Phase 3 in `09_phase3_report.md`.
