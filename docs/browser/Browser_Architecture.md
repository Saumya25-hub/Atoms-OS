# ATRIX Browser Engine v1.0 — Architecture Specification

## Architecture Overview

ATRIX Engine v1.0 is a modular native browser platform for ATOMS OS built without third-party web frameworks (no Chromium, Gecko, or WebKit).

### Core Components
1. **Engine Core (`kernel/browser/engine/`):** Session management, multi-tab coordination, history, settings, security policy.
2. **HTML5 Parser & DOM (`kernel/browser/engine/html/`):** Tokenizer, DOM node allocation, document tree lifecycle.
3. **CSS Engine & Flexbox (`kernel/browser/engine/css/`):** Rule parser, selector matching, box model computation, Flexbox solver.
4. **Render Pipeline (`kernel/browser/engine/render/`):** Render tree generation, layout flow, software rasterization, BWE compositor integration.
5. **JavaScript Stack VM (`kernel/browser/engine/javascript/`):** Execution engine, Mark-Sweep GC, DOM binding bridge.
6. **Enterprise Networking (`kernel/browser/engine/networking/`):** Native HTTP 1.1/HTTPS integration, Cookie store, DNS cache.
