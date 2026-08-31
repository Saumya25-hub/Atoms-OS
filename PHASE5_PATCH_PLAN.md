# PHASE 5 PATCH PLAN: PRODUCTION CSS ENGINE

**Document ID:** ATRIX-PHASE5-PLAN-001  
**Target Subsystem:** ABE CSS Engine, CSSOM, Selector Matching, Cascade & Computed Styles  
**Task Phase:** TASK 2 — ARCHITECT TEAM (NO CODE MODIFICATIONS)  
**Input:** `PHASE5_FORENSIC_REPORT.md`  
**Date:** 2026-08-25  

---

## 1. Objective & Scope

Upgrade the authoritative ABE CSS engine (`kernel/browser_engine/css/`) into a full production-grade CSS pipeline. The engine must tokenize, parse, match selectors, resolve specificity, compute the cascade with `!important`, propagate inheritance, substitute custom properties (`var()`), evaluate media queries, and produce deterministic computed styles for all DOM nodes according to the contract required by Phase 6 Layout.

---

## 2. Authorized File Modifications

Modifications are strictly authorized ONLY for the following files:

1. `kernel/browser_engine/css/abe_css_tokenizer.h` & `abe_css_tokenizer.c`
2. `kernel/browser_engine/css/abe_css_parser.h` & `abe_css_parser.c`
3. `kernel/browser_engine/css/abe_css_selector.h` & `abe_css_selector.c`
4. `kernel/browser_engine/css/abe_css_cascade.h` & `abe_css_cascade.c`
5. `kernel/browser_engine/css/abe_css_inherit.h` & `abe_css_inherit.c`
6. `kernel/browser_engine/css/abe_css_computed.h` & `abe_css_computed.c`
7. `kernel/browser_engine/css/abe_css_test.c` & `abe_css_test.h`
8. `kernel/apps/atrix/atrix_browser.c` (Wired `about:csstest`)
9. `docs/architecture/atrix_browser_phase5_css_engine.md`

All other kernel subsystems and files are strictly locked.

---

## 3. Detailed Component Architecture Plan

### 3.1 Component 1: CSS Tokenizer Engine (`abe_css_tokenizer.c` / `.h`)
- **What to modify:**
  - Implement full token taxonomy: `CSSTOKEN_IDENT`, `CSSTOKEN_STRING`, `CSSTOKEN_NUMBER`, `CSSTOKEN_DIMENSION`, `CSSTOKEN_PERCENTAGE`, `CSSTOKEN_HASH`, `CSSTOKEN_DELIM`, `CSSTOKEN_COLON`, `CSSTOKEN_SEMICOLON`, `CSSTOKEN_LBRACE`, `CSSTOKEN_RBRACE`, `CSSTOKEN_LPAREN`, `CSSTOKEN_RPAREN`, `CSSTOKEN_LBRACKET`, `CSSTOKEN_RBRACKET`, `CSSTOKEN_COMMA`, `CSSTOKEN_FUNCTION`, `CSSTOKEN_AT_RULE`, `CSSTOKEN_IMPORTANT`, `CSSTOKEN_EOF`.
  - Comprehensive color parser: `#RGB`, `#RGBA`, `#RRGGBB`, `#RRGGBBAA`, `rgb()`, `rgba()`, and standard named colors (`red`, `blue`, `green`, `black`, `white`, `yellow`, `cyan`, `magenta`, `gray`, `silver`, `maroon`, `purple`, `navy`, `teal`, `olive`, `orange`, `transparent`).
  - Unit recognizers: `px`, `%`, `em`, `rem`, `vw`, `vh`, `vmin`, `vmax`, `pt`.
  - Custom property tokens: `--var-name` and `var(...)`.
- **Expected Result:** Clean, non-crashing token stream for any standard or malformed CSS input.

### 3.2 Component 2: CSS Parser & CSSOM (`abe_css_parser.c` / `.h`)
- **What to modify:**
  - Expand capacities: `ABE_MAX_CSS_RULES 512`, `ABE_MAX_CSS_DECLARATIONS 64`, `ABE_MAX_STYLESHEETS 32`.
  - Store `is_important` boolean per declaration.
  - Store and lookup custom properties (`--atoms-color`).
  - Parse `@media` query blocks and `@import` at-rules.
  - Shorthand property parser for `margin`, `padding`, `border`, `background`.
  - Safe error recovery on missing semicolons, unknown properties, and bad values.
- **Expected Result:** Robust CSSOM tree representing all author, UA, and inline rules.

### 3.3 Component 3: Selector Engine & Specificity (`abe_css_selector.c` / `.h`)
- **What to modify:**
  - Universal (`*`), Tag/Type (`div`), Class (`.class`), ID (`#id`), Compound (`div.foo#main`), Lists (`h1, h2, h3`).
  - Attribute selectors: `[attr]`, `[attr="val"]`, `[attr~="val"]`.
  - Combinators: Descendant (`A B`), Child (`A > B`), Adjacent sibling (`A + B`), General sibling (`A ~ B`).
  - Pseudo-classes: `:hover`, `:active`, `:focus`, `:first-child`, `:last-child`, `:nth-child()`.
  - Specificity calculation $(A, B, C)$ with exact lexicographical comparison.
- **Expected Result:** Accurate rule matching against DOM element nodes.

### 3.4 Component 4: Cascade & Inheritance (`abe_css_cascade.c` / `abe_css_inherit.c`)
- **What to modify:**
  - Cascade ordering: Importance (`!important`) ➔ Origin (Inline > Author > UA) ➔ Specificity $(A, B, C)$ ➔ Source Order.
  - Inherited property whitelist: `color`, `font-family`, `font-size`, `font-weight`, `line-height`, `text-align`, `visibility`.
  - Non-inherited properties strictly isolated from child propagation.
- **Expected Result:** Spec-compliant cascading and property inheritance.

### 3.5 Component 5: Computed Style Engine (`abe_css_computed.c` / `.h`)
- **What to modify:**
  - Expand node store to 4,096 nodes (`ABE_MAX_COMPUTED_NODES 4096`).
  - Substitute custom properties (`var(--name)`).
  - Filter `@media` queries using actual BWE screen viewport dimensions.
  - Resolve unit lengths (`em`, `rem`, `px`, `%`).
  - Provide stable computed style struct contract for Phase 6 Layout.
- **Expected Result:** 100% computed style coverage for all DOM nodes without synthetic mocks.

### 3.6 Component 6: Deterministic Test Suite (`abe_css_test.c` / `.h`)
- **What to modify:**
  - Implement 15 deterministic test cases (Basic rule, Specificity, !important, Inheritance, Shorthands, Units, Colors, Attribute selectors, Combinators, Pseudo-classes, Custom properties, Media queries, Malformed recovery, Large stylesheets, Real HTML document pipeline).
- **Expected Result:** Automated boot verification and interactive `about:csstest` Omnibox inspection.

---

## 4. Rollback Plan

If regressions occur, Git version control allows instantaneous rollback of the CSS directory without affecting Phase 1–4 binaries.

---

## 5. Approval & Transition

Patch plan is approved. Transitioning to **Task 3 (Patch Team)** to apply the code changes.
