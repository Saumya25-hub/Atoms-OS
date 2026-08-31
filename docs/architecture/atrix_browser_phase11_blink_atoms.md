# ATRIX Browser — Phase 11 Chromium Blink Core Integration Reference

**Document ID:** ATRIX-DOC-PHASE11-001  
**Architecture Layer:** Userspace Web Platform / Rendering Engine  
**Version:** 1.0.0 (Phase 11 Certified)  
**Date:** 2026-08-26  

---

## 1. Overview & Architecture

Phase 11 integrates the open-source Google Chromium / Blink browser engine into ATRIX Browser on ATOMS OS.

```text
       Raw HTML / CSS / JavaScript Stream
                       │
                       ▼
       ┌───────────────────────────────┐
       │      Blink HTML5 Parser       │
       └───────────────┬───────────────┘
                       │
                       ▼
       ┌───────────────────────────────┐
       │        Blink DOM Tree         │
       │ (Document, Element, Node)     │
       └───────┬───────────────▲───────┘
               │               │
      DOM Reads│               │DOM Mutations
               ▼               │
       ┌───────────────────────┴───────┐
       │   V8 ↔ Blink Script Bindings  │
       │   (ScriptController, Window)  │
       └───────────────────────────────┘
                       │
                       ▼
       ┌───────────────────────────────┐
       │      Blink Style Engine       │
       │ (CSSOM, Computed Dimensions)  │
       └───────────────┬───────────────┘
                       │
                       ▼
       ┌───────────────────────────────┐
       │      Blink Layout Engine      │
       │ (LayoutBlock, LayoutInline)   │
       └───────────────┬───────────────┘
                       │
                       ▼
       ┌───────────────────────────────┐
       │     Blink ➔ Skia Painter      │
       │  (BlinkSkiaPainter ➔ Canvas)  │
       └───────────────┬───────────────┘
                       │
                       ▼
       ┌───────────────────────────────┐
       │ Google Skia 2D Rasterizer     │
       │ (SkCanvas, SkSurface, SkPaint)│
       └───────────────┬───────────────┘
                       │
                       ▼
       ┌───────────────────────────────┐
       │       AtomsSkiaSurface        │
       │ (Zero-Copy BWE Framebuffer)   │
       └───────────────┬───────────────┘
                       │
                       ▼
       ┌───────────────────────────────┐
       │      BWE Window Manager       │
       │ (Compositor & Display Output) │
       └───────────────────────────────┘
```

---

## 2. Directory Layout

- `third_party/blink/renderer/core/dom/`: DOM core hierarchy (`Document`, `Element`, `Node`, `Text`, `ContainerNode`).
- `third_party/blink/renderer/core/html/parser/`: HTML5 tokenizer and DOM tree construction.
- `third_party/blink/renderer/core/css/`: CSS declaration parsing and computed styles.
- `third_party/blink/renderer/core/layout/`: Block and inline box layout geometry calculations.
- `third_party/blink/renderer/core/paint/`: Blink painting pipeline to Google Skia.
- `third_party/blink/renderer/core/bindings/core/v8/`: V8 JavaScript bindings and DOM mutation engine.
- `third_party/blink/renderer/adapter/`: High-level ATRIX browser and BWE window bridge.
- `third_party/blink/tests/`: Deterministic 20-test verification suite and test runner.
