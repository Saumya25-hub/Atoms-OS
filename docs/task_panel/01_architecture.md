# Task Panel Architecture

## Objective
The Task Panel handles visual tracking and focus switching for all top-level desktop applications.

## Issues Addressed
Prior to Phase 6.1.1, the Task Panel utilized a naïve one-pass layout loop. It linearly stacked buttons from left to right, assuming infinite width.

## The New Dynamic Pipeline
The panel now performs:
1. **Pass 1:** Valid Window Discovery. It collects all `BWE_TYPE_WINDOW` components belonging to the Desktop.
2. **Pass 2:** Width Calculation. It computes the available mathematical bounds between the Start Button and the System Clock, dividing the space symmetrically among all active tasks.
3. **Pass 3:** Rendering. It dynamically scales buttons, truncates their text with ellipsis (`...`), and draws them. If limits are reached, it initiates Overflow Mode.
