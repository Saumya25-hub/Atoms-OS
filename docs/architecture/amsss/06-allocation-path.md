# Allocation Path

Tracked heap entry points call the coordinator. Policy checks budget, performs bounded reclaim when needed, invokes the raw backend, and retries once after failure. Telemetry and pressure are then refreshed.

