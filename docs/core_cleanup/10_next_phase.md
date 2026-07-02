# Next Phase Prep (Phase 4)

## Problem
With the architecture now stabilized and modularized into Foundation v2, the OS lacks features that modern users expect (like a robust input layer, sophisticated task management, or more advanced UI widgets).

## Cause
Phase 3 deliberately froze all feature development to focus entirely on architectural cleanup and technical debt reduction.

## Solution
Phase 4 will transition back into active development mode. Potential targets include:
- Expanding the `engine/` (Horse Engine) to handle system-wide search and dispatching.
- Developing new `ui/` controls (like scrollable text areas or rich list views) utilizing the newly isolated `WMContext` and robust boundaries.
- Upgrading the `shell/` to feature a polished Start Menu and notification system now that it is properly decoupled from `wm/`.

## Risk
Developers might fall back into old habits and start creating circular dependencies or new "God Objects".

## Verification
Strict code review policies must be enforced. Any new feature must abide by the Foundation v2 rules established in this documentation.

## Next Step
Await mission directives for Phase 4 from the lead OS Architect.
