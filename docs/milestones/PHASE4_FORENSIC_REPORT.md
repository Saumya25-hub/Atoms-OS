# PHASE 4 FORENSIC REPORT: HTML5 / DOM ENGINE

**Document ID:** ATRIX-PHASE4-FORENSIC-001  
**Target Subsystem:** ABE HTML5 Parser, Tokenizer, Tree Builder & DOM Subsystem  
**Task Phase:** TASK 1 — FORENSIC TEAM (NO CODE MODIFICATIONS)  
**Date:** 2026-08-25  

---

## 1. Executive Summary

A comprehensive repository-wide forensic audit was conducted on all HTML, DOM, and tree builder implementations across ATOMS OS. The audit traced the active runtime call path from ATRIX Omnibox navigation down to the ABE rendering pipeline, audited tokenization states, tree construction rules, entity decoders, DOM node slab allocators, and identified all legacy, duplicate, and active components.

---

## 2. Inventory of Discovered HTML & DOM Implementations

Three distinct HTML/DOM implementations exist in the repository:

### 2.1 Implementation A: `kernel/browser_engine/html/` (AUTHORITATIVE & ACTIVE)
- **Status:** **ACTIVE, COMPILED, LINKED, CALLED**
- **Files:**
  - `abe_html_tokenizer.c` / `.h` — State-machine tokenizer
  - `abe_html_parser.c` / `.h` — Stack-based tree constructor
  - `abe_dom_node.c` / `.h` — DOM slab pool allocator and hierarchy pointers
  - `abe_html_element.c` / `.h` — Element definitions, attributes, void elements
  - `abe_html_text.c` / `.h` — Text node helpers and whitespace detection
  - `abe_html_document.c` / `.h` — Document struct and lifecycle management
  - `abe_html_api.c` — Public ABE HTML entry points (`ABE_ParseHTML`, `ABE_ParseHTMLStream`)
  - `abe_html_test.c` / `.h` — HTML engine test harness
- **Call Trace:** `atrix_execute_browser_pipeline()` ➔ `ABE_ParseHTML()` ➔ `ABE_HTMLParser_ParseDocument()` ➔ `ABE_HTMLTokenizer_NextToken()` ➔ `ABE_DOM_CreateNode()` ➔ `ABE_BuildRenderTree()` ➔ `atrix_paint_node_recursive()`.

### 2.2 Implementation B: `browser/html/` (LEGACY STANDALONE SUITE)
- **Status:** **COMPILED, LINKED, BUT NOT CALLED IN ATRIX RUNTIME PATH**
- **Files:** `browser/html/bos_html.c`, `browser/html/tokenizer/*`, `browser/html/tree_builder/*`, `browser/html/node/*`, `browser/html/attributes/*`, `browser/html/comment/*`, `browser/html/doctype/*`, `browser/html/document/*`, `browser/html/element/*`, `browser/html/fragment/*`, `browser/html/mutation/*`, `browser/html/parser/*`, `browser/html/serialization/*`, `browser/html/tests/*`.
- **Classification:** Standalone test suite for earlier BOS prototype. Does not feed into ABE Render Tree or BWE window surface.

### 2.3 Implementation C: `kernel/browser/engine/html/` (EARLY PRECURSOR)
- **Status:** **COMPILED, LINKED, DEAD**
- **Files:** `html_parser.c`, `html_document.c`.
- **Classification:** Superseded by Implementation A during Phase 1 Unification.

---

## 3. Subsystem Forensic Audit & Identified Gaps

### 3.1 HTML5 Tokenizer (`abe_html_tokenizer.c`)
- **Current State:** Implements basic states (`STATE_DATA`, `STATE_TAG_OPEN`, `STATE_END_TAG_OPEN`, `STATE_TAG_NAME`, `STATE_BEFORE_ATTR_NAME`, `STATE_ATTR_NAME`, `STATE_AFTER_ATTR_NAME`, `STATE_BEFORE_ATTR_VAL`, `STATE_ATTR_VAL_DOUBLE_QUOTED`, `STATE_ATTR_VAL_SINGLE_QUOTED`, `STATE_ATTR_VAL_UNQUOTED`, `STATE_SELF_CLOSING_TAG`, `STATE_COMMENT`).
- **Gaps:**
  1. **DOCTYPE States:** Missing dedicated `STATE_DOCTYPE`, `STATE_BEFORE_DOCTYPE_NAME`, `STATE_DOCTYPE_NAME`, `STATE_AFTER_DOCTYPE_NAME` to parse `<!DOCTYPE html>` and extract document standards mode.
  2. **Raw Text / RCDATA Contexts (`<script>`, `<style>`, `<textarea>`, `<title>`):** Characters like `<` inside raw text regions are incorrectly treated as tag delimiters unless the exact matching closing tag (e.g. `</script>`) is encountered.
  3. **Character Reference Decoding:** Only decodes 6 hardcoded named entities (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`, `&nbsp;`). Numeric decimal (`&#65;`), hexadecimal (`&#x41;`, `&#X41;`), and standard named entities (`&copy;`, `&reg;`, `&trade;`, `&mdash;`, `&ndash;`, `&bull;`) are unhandled.
  4. **Void Elements:** Needs a complete list of void self-closing tags (`area`, `base`, `br`, `col`, `embed`, `hr`, `img`, `input`, `link`, `meta`, `param`, `source`, `track`, `wbr`).

### 3.2 HTML5 Tree Construction (`abe_html_parser.c`)
- **Current State:** Stack-based open element tracker with rudimentary tag matching.
- **Gaps:**
  1. **Implied Elements:** Omitting `<html>`, `<head>`, or `<body>` must automatically synthesize standard document hierarchy according to HTML5 insertion mode state machines.
  2. **Comprehensive Tag Support:** Explicit handling for `html`, `head`, `body`, `title`, `meta`, `link`, `style`, `script`, `div`, `span`, `p`, `a`, `h1`-`h6`, `ul`, `ol`, `li`, `table`, `thead`, `tbody`, `tfoot`, `tr`, `td`, `th`, `form`, `input`, `button`, `textarea`, `select`, `option`, `img`, `br`, `hr`, `pre`, `code`, `section`, `article`, `header`, `footer`, `nav`, `main`.
  3. **Optional End Tags & Auto-Closing:** Block tags (`p`, `div`, `h1`-`h6`, `table`, `ul`, `ol`) must auto-close open `<p>`. List items (`<li>`) must auto-close preceding open `<li>`. Table cells (`<td>`, `<th>`) must auto-close open cells. Table rows (`<tr>`) must auto-close open rows. Options (`<option>`) must auto-close open options.
  4. **Table Structure:** Implied `<tbody>` insertion when `<tr>` is encountered directly under `<table>`.
  5. **Misnesting & Error Recovery:** Deterministic stack search and popping without crashing or leaking nodes.

### 3.3 DOM Model & Slab Node Allocator (`abe_dom_node.c` / `abe_dom_node.h`)
- **Current State:** Fixed static array pool of 1024 nodes (`ABE_DOM_POOL_SLOTS 1024`).
- **Gaps:**
  1. **Pool Capacity:** Needs expansion from 1024 to 4096 nodes to accommodate rich modern web documents without truncation.
  2. **Tree Invariants:** Maintain strict bidirectional parent, first child, last child, previous sibling, next sibling, child count links during mutations.
  3. **DOM Traversal & Query Primitives:** Provide `ABE_DOM_GetElementById()`, `ABE_DOM_GetElementsByTagName()`, `ABE_DOM_GetElementsByClassName()`.
  4. **Attribute Operations:** `ABE_HTMLAttr_Get()`, `ABE_HTMLAttr_Set()`, `ABE_HTMLAttr_Remove()`, `ABE_HTMLAttr_Has()`.
  5. **Text & Serialization:** `ABE_DOM_GetTextContent()`, `ABE_DOM_SetTextContent()`, `ABE_DOM_GetInnerHTML()`, `ABE_DOM_GetOuterHTML()`.
  6. **Security & Resource Limits:** Strict depth limit (64), attribute length limits (256), tag length limits (64), and bounds checks to guarantee zero Ring-0 kernel memory corruption.

---

## 4. Risk Analysis & Suspected Fix Strategy

1. **Risk:** Expanding DOM structures could increase BSS/stack usage.
   - **Mitigation:** Static memory sizing within verified kernel limits (`4096` nodes x ~600 bytes = ~2.4 MB heap/BSS).
2. **Risk:** CSSOM / StyleManager dependency on tag names.
   - **Mitigation:** Preserve exact lowercase normalized tag names and attribute strings expected by `ABE_StyleManager_LoadDocumentStyles()`.
3. **Risk:** Malformed web HTML triggering Ring-0 stack overflow or heap corruption.
   - **Mitigation:** Bound tokenizer buffer index increments, enforce maximum stack depth of 64, clamp entity decoders, and validate all node handles before pointer dereferences.

---

## 5. Next Phase Transition

Forensic investigation is complete. Proceeding to **Task 2 (Architect Team)** to draft `PHASE4_PATCH_PLAN.md`.
