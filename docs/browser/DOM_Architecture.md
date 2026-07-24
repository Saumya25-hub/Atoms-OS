# ATRIX Browser Engine v1.0 — DOM Architecture

## DOM Node Lifecycle

- Node Types: `DOCUMENT`, `ELEMENT`, `TEXT`, `COMMENT`.
- N-ary Tree: Each node maintains child pointers up to 16 children.
- Memory: Managed via ATOMS heap `kmalloc` / `kfree` with recursive cleanup `ATRIX_DOM_FreeNode`.
