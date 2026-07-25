# Telemetry

Bounded counters record allocations, successful bytes, frees, failures, expansions, reclaim runs, and diagnostic events. Counters allocate no memory, are not persisted, and may wrap. Free-byte attribution is limited where the public free API lacks the original requested size.

