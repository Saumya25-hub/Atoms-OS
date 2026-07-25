# Coordinator

The coordinator owns initialization, recursion guarding, budget checks, retry-after-reclaim, and periodic engine updates. Before initialization or during guarded activity, calls go directly to raw backend hooks. This bootstrap route avoids recursive policy execution while retaining heap diagnostics.

The coordinator is synchronous and bounded. It performs at most one reclaim-and-retry cycle per failed public allocation.
