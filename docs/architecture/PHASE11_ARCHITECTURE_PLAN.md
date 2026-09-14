# PHASE 11 ARCHITECTURE PLAN: CHROMIUM / BLINK CORE INTEGRATION

**Document ID:** ATRIX-PHASE11-PLAN-001  
**Phase:** STEP 2 — ARCHITECTURE DECISION & PATCH PLAN  
**Target Subsystem:** Chromium Blink Core, Blink DOM, V8 ↔ Blink Bindings, Style & Layout Engine, Blink ➔ Skia Painter & ATRIX Browser Shell  
**Standard:** Rule 0 Phase Isolation (Investigate ➔ Plan ➔ Implement ➔ Build ➔ Runtime Verify ➔ Audit ➔ Certify)  
**Date:** 2026-08-26  

---

## 1. Architectural Strategy & ABE Subsystem Transition

As decided in the Phase 6 Architectural Audit and ratified in Phases 7–10:
- **Primary Engine:** Google Chromium / Blink (`third_party/blink/`) is the authoritative primary web platform and rendering engine for ATRIX Browser.
- **ABE Subsystems (Phase 1–5):** The legacy ABE components are preserved in `kernel/browser/` as an internal diagnostic and lightweight fallback layer for kernel health telemetry.
- **Unified Pipeline:** All standard web navigation, HTML rendering, DOM manipulation, CSS styling, JavaScript script execution, and visual painting for ATRIX will execute through the **Blink ➔ V8 ➔ Skia ➔ BWE** pipeline.

---

## 2. End-to-End Architectural Pipeline

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        ATRIX BROWSER USER INTERFACE                    │
│                      (Omnibox, Tabs, Navigation Bar)                   │
├────────────────────────────────────────────────────────────────────────┤
│                       Chromium Blink Core Engine                       │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │ HTML5 Parser ➔ Blink DOM Tree (Document, Element, Node, Text)   │  │
│  │                                                                  │  │
│  │ V8 ↔ Blink Bindings (Window, Document, Element, ScriptController)│  │
│  │                                                                  │  │
│  │ Style Engine (CSSOM, Style Declarations, Computed Styles)        │  │
│  │                                                                  │  │
│  │ Layout Engine (LayoutBlock, LayoutInline, Geometric Box Tree)    │  │
│  │                                                                  │  │
│  │ Blink ➔ Skia Painter (BlinkSkiaPainter)                          │  │
│  └───────────────────────────────────┬──────────────────────────────┘  │
├──────────────────────────────────────┼─────────────────────────────────┤
│                                      ▼                                 │
│                     Google Skia 2D Graphics Engine                     │
│               (SkCanvas, SkSurface, SkPaint, SkRRect, SkPath)          │
├────────────────────────────────────────────────────────────────────────┤
│                      ATOMS Skia Graphics Adapter                       │
│              (AtomsSkiaSurface: Direct BWE Framebuffer Map)            │
├────────────────────────────────────────────────────────────────────────┤
│                           BWE Window Manager                           │
│                      (Compositor, Window Canvas)                       │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Implementation Plan & File Structure

### 3.1 Blink DOM Subsystem (`third_party/blink/renderer/core/dom/`)
- `node.h` & `node.cpp`: Node types, hierarchy, sibling/child navigation, `appendChild`, `removeChild`, `insertBefore`, `replaceChild`, `textContent`.
- `container_node.h` & `container_node.cpp`: Child node list management.
- `element.h` & `element.cpp`: Tag names, attributes (`setAttribute`, `getAttribute`), class list, `id`, `innerHTML`, `querySelector`.
- `document.h` & `document.cpp`: Root document node, `title`, `body`, `head`, `createElement`, `createTextNode`, `getElementById`, `querySelector`.
- `text.h` & `text.cpp`: CharacterData text leaf node.
- `events/event.h` & `events/event_target.h`: Event dispatch and listeners.

### 3.2 Blink HTML Subsystem (`third_party/blink/renderer/core/html/`)
- `html_element.h` & `html_element.cpp`: HTMLElement base class with style declarations.
- `html_div_element.h`, `html_heading_element.h`, `html_paragraph_element.h`, `html_body_element.h`.
- `parser/html_parser.h` & `parser/html_parser.cpp`: HTML5 tokenizer and tree builder.

### 3.3 Blink CSS & Style Subsystem (`third_party/blink/renderer/core/css/`)
- `css_style_declaration.h` & `css_style_declaration.cpp`: Inline styles and parsed CSS properties (`color`, `background-color`, `font-size`, `margin`, `padding`, `display`, `width`, `height`).
- `style_engine.h` & `style_engine.cpp`: Computed style calculation and cascading inheritance.

### 3.4 Blink Layout Subsystem (`third_party/blink/renderer/core/layout/`)
- `layout_object.h` & `layout_object.cpp`: Base layout tree node.
- `layout_block.h` & `layout_block.cpp`: Block-level layout box with x, y, width, height geometry.
- `layout_inline.h` & `layout_inline.cpp`: Inline text layout.
- `layout_tree_builder.h` & `layout_tree_builder.cpp`: Generates LayoutObject tree from DOM Tree + Computed Styles.

### 3.5 Blink ➔ Skia Painter (`third_party/blink/renderer/core/paint/`)
- `blink_skia_painter.h` & `blink_skia_painter.cpp`: Traverses LayoutObject tree and renders backgrounds, borders, text, and geometry to `SkCanvas`.

### 3.6 V8 ↔ Blink Bindings (`third_party/blink/renderer/core/bindings/core/v8/`)
- `v8_binding.h` & `v8_binding.cpp`: V8 isolate and context wrapper registry.
- `v8_window.h` & `v8_window.cpp`: Injects `window`, `document`, `console`, `location` into V8 global context.
- `v8_document.h` & `v8_document.cpp`: Exposes `document.title`, `document.body`, `document.createElement`, `document.getElementById`, `document.querySelector`.
- `v8_element.h` & `v8_element.cpp`: Exposes `element.innerHTML`, `element.textContent`, `element.appendChild`, `element.setAttribute`, `element.style`.
- `script_controller.h` & `script_controller.cpp`: Executes `<script>` elements within document context using Phase 10 V8 engine.

### 3.7 ATOMS Blink Adapter (`third_party/blink/renderer/adapter/`)
- `atoms_blink_adapter.h` & `atoms_blink_adapter.cpp`: High-level page rendering driver: HTML ➔ Blink DOM ➔ V8 Script Execution ➔ Style Engine ➔ Layout Engine ➔ BlinkSkiaPainter ➔ AtomsSkiaSurface ➔ BWE Canvas.

### 3.8 Test Suite & Integration (`third_party/blink/tests/`)
- `blink_test_suite.h` & `blink_test_suite.cpp`: 20 deterministic tests verifying DOM creation, mutation, query, V8 script DOM mutation, style computation, layout geometry, Skia painting, and BWE presentation.
- `blink_test_main.cpp`: Standalone executable entry point.
- `BUILD.gn`, `build.ps1`, and `kernel/apps/atrix/atrix_browser.c`.
