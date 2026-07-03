# Debug Points

## Telemetry Injection Sites
To effectively debug Input and UX without flooding the console, diagnostic counters should be placed strategically:

### 1. `mouse_irq_handler` (`mouse.c`)
- **Metric:** IRQ hit rate, invalid packet rate, bytes dropped.
- **Why:** To diagnose mouse stutter. If IRQs fire but the bytes desync, it indicates a virtualization or timeout glitch.

### 2. `push_event` (`input.c`)
- **Metric:** `queue_head == queue_tail` collision count.
- **Why:** Detects silent event dropping. If the buffer (64) is too small, this counter will spike during rapid mouse sweeping.

### 3. `BWE_PumpEvents` (`bwe_core.c`)
- **Metric:** Time spent inside the `for` loop executing `BWE_HitTest`.
- **Why:** Diagnoses dispatch latency. This O(N*M) loop is the prime suspect for input delay as the window count grows.

### 4. `BWE_UpdateZOrders` (`bwe_window.c`)
- **Metric:** Execution time.
- **Why:** Bubble sort. Monitoring this prevents catastrophic slowdowns as Z-stack height increases.

### 5. `BWE_ComposeFrame` (Compositor)
- **Metric:** Total damage area per frame vs total framebuffer size.
- **Why:** Window dragging invalidates entire windows. Measuring damage overhead will mathematically prove why dragging lags the system.
