# Queue Design Analysis

## Why 1024?
Previously, queues were 64 or 128 elements deep. A 1000Hz gaming mouse generates 1000 IRQs per second. If the UI thread stalled for 65 milliseconds (e.g., drawing a heavy frame), the 64-event queue would overflow and drop input.

A 1024-event queue provides ~1 second of buffer for the mouse at 1000Hz. This ensures that even during massive system loads, input is recorded perfectly and processed as soon as the pump resumes.

## Memory Footprint
A single `BWE_Event` is 20 bytes. 
1024 events = 20 KB. 
The memory tradeoff is negligible compared to the stability gained.
