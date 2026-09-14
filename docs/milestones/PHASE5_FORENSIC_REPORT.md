# PHASE 5 FORENSIC REPORT: PRODUCTION CSS ENGINE

**Document ID:** ATRIX-PHASE5-FORENSIC-001  
**Target Subsystem:** ABE CSS Engine, CSSOM, Selector Matcher, Cascade, Inheritance & Computed Style Subsystem  
**Task Phase:** TASK 1 — FORENSIC TEAM (NO CODE MODIFICATIONS)  
**Date:** 2026-08-25  

---

## 1. Executive Summary

A repository-wide forensic audit was conducted on all CSS, stylesheet, selector, cascade, specificity, and computed style implementations across ATOMS OS. The audit analyzed the active runtime call chain from ATRIX Omnibox navigation, HTML `<style>` extraction, through CSS parsing, selector matching, specificity calculation, cascade resolution, property inheritance, to the computed style contract consumed by the Phase 6 Layout Engine.

---

## 2. Inventory of Discovered CSS Implementations

Three distinct CSS implementations exist in the repository:

### 2.1 Implementation A: `kernel/browser_engine/css/` (AUTHORITATIVE & ACTIVE)
- **Status:** **ACTIVE, COMPILED, LINKED, CALLED**
- **Files:**
  - `abe_css_tokenizer.c` / `.h` — State-machine CSS tokenizer
  - `abe_css_parser.c` / `.h` — Stylesheet & declaration parser, CSSOM
  - `abe_css_selector.c` / `.h` — Selector engine and specificity calculator
  - `abe_css_cascade.c` / `.h` — Cascade resolver (Origin, Importance, Specificity, Source Order)
  - `abe_css_inherit.c` / `.h` — Property inheritance engine
  - `abe_css_computed.c` / `.h` — Computed style engine and DOM style applicator
  - `abe_css_style_manager.c` / `.h` — Document style manager
  - `abe_css_api.c` — Public ABE CSS C API
  - `abe_css_test.c` / `.h` — CSS engine verification suite
- **Call Trace:** `atrix_execute_browser_pipeline()` ➔ `ABE_StyleManager_LoadDocumentStyles()` ➔ `ABE_CSSParser_ParseStylesheet()` ➔ `ABE_CSSComputed_ComputeDocumentStyles()` ➔ `ABE_BuildRenderTree()` ➔ `atrix_paint_node_recursive()`.

### 2.2 Implementation B: `browser/css/` (LEGACY STANDALONE SUITE)
- **Status:** **COMPILED, LINKED, BUT NOT CALLED IN ATRIX RUNTIME PATH**
- **Files:** `browser/css/bos_css.c`, `browser/css/css_parser.c`, `browser/css/css_rule.c`, `browser/css/css_selector.c`, `browser/css/css_specificity.c`, `browser/css/css_style.c`, `browser/css/css_stylesheet.c`, `browser/css/css_tests.c`, `browser/css/css_tokenizer.c`, `browser/css/css_value.c`.
- **Classification:** Standalone test suite for earlier BOS prototype. Does not feed into ABE Render Tree or BWE window surface.

### 2.3 Implementation C: `kernel/browser/engine/css/` (EARLY PRECURSOR)
- **Status:** **COMPILED, LINKED, DEAD**
- **Files:** `css_parser.c`, `css_layout.c`.
- **Classification:** Superseded by Implementation A during Phase 1 Unification.

---

## 3. Subsystem Forensic Audit & Identified Gaps

### 3.1 CSS Tokenizer (`abe_css_tokenizer.c`)
- **Current State:** Implements simple scanner for ident, string, number, dimension, percentage, hash, colon, semicolon, braces, parens.
- **Gaps:**
  1. **Functions:** Missing explicit function tokenization for `rgb()`, `rgba()`, `calc()`, `var()`, `url()`.
  2. **Colors:** Only parses `#RGB`, `#RRGGBB`, `#RRGGBBAA`. Missing `rgb(...)`, `rgba(...)`, and standard CSS named colors.
  3. **Brackets & Commas:** Missing `CSSTOKEN_LBRACKET`, `CSSTOKEN_RBRACKET`, and `CSSTOKEN_COMMA` for attribute selectors and lists.
  4. **At-Rules:** Missing parsing for `@media` and `@import`.
  5. **Important Token:** Missing `!important` detection.

### 3.2 CSS Parser & CSSOM (`abe_css_parser.c` / `abe_css_parser.h`)
- **Current State:** Parses basic rules and declarations into a fixed struct array.
- **Gaps:**
  1. **Capacity Limits:** `ABE_MAX_CSS_RULES 128`, `ABE_MAX_CSS_DECLARATIONS 32`, `ABE_MAX_STYLESHEETS 16`. Needs expansion to 512 rules, 64 declarations, and 32 stylesheets.
  2. **!important Storage:** Declarations need `is_important` boolean flag to participate in cascade overriding.
  3. **Custom Properties:** Missing CSS Variable (`--var-name: value`) storage and retrieval.
  4. **At-Rules:** Missing `@media` and `@import` rule parsing.
  5. **Shorthands:** Needs expansion for multi-value `margin`, `padding`, `border`, `background`.

### 3.3 Selector Engine & Specificity (`abe_css_selector.c`)
- **Current State:** Matches single tag, id (`#`), class (`.`), descendant (`A B`), child (`A > B`).
- **Gaps:**
  1. **Attribute Selectors:** Missing `[attr]`, `[attr=val]`, `[attr~=val]`.
  2. **Sibling Combinators:** Missing adjacent sibling (`A + B`) and general sibling (`A ~ B`).
  3. **Selector Lists:** Missing comma-separated lists (`h1, h2, h3`).
  4. **Compound Selectors:** Missing compound chains (`div.foo#main`).
  5. **Pseudo-Classes:** Missing `:hover`, `:active`, `:focus`, `:first-child`, `:last-child`, `:nth-child()`.
  6. **Pseudo-Elements:** Missing representation of `::before`, `::after`.

### 3.4 Cascade & Inheritance (`abe_css_cascade.c` / `abe_css_inherit.c`)
- **Current State:** Basic origin and specificity comparison.
- **Gaps:**
  1. **!important Cascade Rule:** Must override all normal declarations regardless of origin.
  2. **Initial & Inherited Values:** Complete initial value specification for all CSS box model, visual, and typography properties.

### 3.5 Computed Style Engine (`abe_css_computed.c`)
- **Current State:** Sized for 1024 nodes; iterates user-agent stylesheet and author `<style>` blocks.
- **Gaps:**
  1. **Node Pool Capacity:** Sized at 1024 nodes (`ABE_MAX_COMPUTED_NODES 1024`). Needs expansion to 4096 nodes to match the Phase 4 DOM node pool.
  2. **Variable Substitution:** Support `color: var(--var-name)` resolution.
  3. **Media Query Evaluation:** Filter `@media` rules against actual BWE viewport dimensions (`min-width`, `max-width`, `min-height`, `max-height`, `screen`).
  4. **Relative Units:** Resolve `em`, `rem`, `vw`, `vh`, `%` against parent/root font sizes and viewport.

---

## 4. Risk Analysis & Suspected Fix Strategy

1. **Risk:** Buffer overflows on large or malformed CSS stylesheets.
   - **Mitigation:** Strict bounds checking on all token loops, string buffers clamped to 64/256 bytes, safe delimiter recovery on unrecognized syntax.
2. **Risk:** Ring-0 stack overflow during deep selector recursion.
   - **Mitigation:** Iterative ancestor and sibling traversal bounded by DOM tree depth (64 levels).
3. **Risk:** Phase 1/4 layout and DOM regressions.
   - **Mitigation:** Preserve exact `ABE_ComputedStyle` layout contract consumed by `ABE_BuildRenderTree()`.

---

## 5. Next Phase Transition

Forensic investigation is complete. Proceeding to **Task 2 (Architect Team)** to draft `PHASE5_PATCH_PLAN.md`.
