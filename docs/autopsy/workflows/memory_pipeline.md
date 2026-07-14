# Memory Pipeline (Heap Manager V1)

`kmalloc` (Frontend)
↓ [PASS] (Correctly aligns and sizes)

`allocate_block` (Backend)
↓ [PASS] (Correctly requests pages via VMM)

`set_canary` (Metadata)
↓ [PASS] (Writes front/rear canaries properly)

`track_caller` (Diagnostics)
↓ [FAIL] (Fails to reliably record `__builtin_return_address(0)` when inline/relocated)

`validate_block_or_panic` (Auditor)
↓ [PASS] (Correctly detects smashed canaries and triggers halts)

`kfree` / `krealloc` (Reclaimer)
↓ [FAIL] (Panics unconditionally without graceful recovery)
