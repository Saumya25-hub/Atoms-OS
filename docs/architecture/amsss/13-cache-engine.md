# Cache Engine

The generic registry holds at most 16 cache records. Reclaim destroys only active zero-reference records. It has no dynamic metadata, LRU clock, background worker, or automatic ownership inference.

