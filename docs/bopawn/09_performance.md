# Performance

Performance is optimized by:
1. Minimizing heap allocations.
2. Caching decoded images.
3. Fast path surface conversions directly into BOSSurface pixel layouts.