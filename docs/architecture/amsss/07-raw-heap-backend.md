# Raw Heap Backend

Raw allocation, reallocation, free, expansion, and statistics hooks bridge to the existing linked-block heap without re-entering AMSSS. Existing caller-RIP metadata, canaries, diagnostics, and BMLE accounting remain authoritative.

