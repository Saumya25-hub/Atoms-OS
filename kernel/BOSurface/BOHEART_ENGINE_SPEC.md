# 🫀 BOHEART ENGINE — FINAL CLEAN SPEC (SIGNATURESOS)

## 🎯 CORE PRINCIPLE

> “OS is not event-driven chaos. OS is a deterministic heartbeat system.”

---

# 🧠 SYSTEM GOAL

* Stable 60 FPS rendering
* Zero jitter input feel
* No race conditions
* No dual rendering paths
* Fully deterministic frame output

---

# 🏗️ CLEAN SYSTEM TREE

```
BOHEART ENGINE (MASTER CLOCK)
│
├── INPUT LAYER (RAW CAPTURE ONLY)
│   ├── mouse raw state
│   ├── keyboard raw state
│   └── NO processing allowed here
│
├── INPUT COALESCER
│   ├── mouse position → latest only
│   ├── scroll → accumulated delta
│   └── discrete events → queue
│
├── FRAME CLOCK (16.6ms HARD BEAT)
│   ├── waits for next tick
│   └── triggers frame pipeline
│
├── SNAPSHOT LOCK
│   ├── freeze input state
│   ├── freeze UI state
│   └── immutable frame context
│
├── COMPOSITOR (FULL FRAME ONLY)
│   ├── draw background
│   ├── draw windows (Z-order)
│   ├── draw controls
│   └── NO partial rendering allowed
│
├── CURSOR OVERLAY (LAST STEP)
│   └── hardware/software cursor draw
│
└── SWAP ENGINE
    └── atomic buffer flip to screen
```

---

# ⚡ STRICT RULES (NO EXCEPTIONS)

## ❌ FORBIDDEN

* No partial framebuffer updates
* No mixed dirty + full rendering
* No event-triggered rendering
* No mid-frame state mutation
* No dual render paths

---

## 🟢 ALLOWED ONLY

* One frame = one complete render
* One snapshot per frame
* One swap per frame
* One clock source (BOHEART)

---

# ⏱️ FRAME BEHAVIOR

```
Every 16.6ms:

BOHEART PULSE →
    INPUT COALESCE →
    SNAPSHOT FREEZE →
    FULL COMPOSITE →
    SWAP →
END
```

---

# 🧠 INPUT RULE

* Mouse movement = overwrite only (no processing)
* Click / key = queued event (must not be lost)
* All processing happens ONLY inside frame tick

---

# 💀 PERFORMANCE RULE

> “No work is done twice in same frame.”

---

# 🎮 VISUAL RULE

* Always render full scene
* Never reuse partial pixels
* Never depend on previous frame pixels

---

# 🧱 DESIGN PHILOSOPHY

## BOHEART is:

* NOT renderer
* NOT compositor
* NOT input system

👉 It is:

> “The timing authority that forces everything into one rhythm”

---

# 🚀 FINAL GUARANTEE MODEL

If BOHEART is correct:

✔ no tearing
✔ no ghosting
✔ no jitter
✔ stable motion
✔ predictable UI behavior

---

# 💬 ONE LINE SUMMARY

> “BOHEART ENGINE = Single clock that forces entire OS into deterministic frame rhythm.”
