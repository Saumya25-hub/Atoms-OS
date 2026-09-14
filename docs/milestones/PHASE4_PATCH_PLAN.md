# PHASE 4 PATCH PLAN: PRODUCTION HTML5 / DOM ENGINE

**Document ID:** ATRIX-PHASE4-PLAN-001  
**Target Subsystem:** ABE HTML5 Tokenizer, Tree Builder & DOM Subsystem  
**Task Phase:** TASK 2 — ARCHITECT TEAM (NO CODE MODIFICATIONS)  
**Input:** `PHASE4_FORENSIC_REPORT.md`  
**Date:** 2026-08-25  

---

## 1. Objective & Scope

Upgrade the authoritative ABE HTML engine (`kernel/browser_engine/html/`) into a production-capable, HTML5-compliant document pipeline capable of safely tokenizing, constructing, querying, and mutating real-world web documents received via Phase 2/3 HTTP/HTTPS networking, without modifying CSSOM, JavaScript, or unrelated subsystems.

---

## 2. Authorized File Modifications

Modifications are strictly authorized ONLY for the following files:

1. `kernel/browser_engine/html/abe_dom_node.h` & `abe_dom_node.c`
2. `kernel/browser_engine/html/abe_html_element.h` & `abe_html_element.c`
3. `kernel/browser_engine/html/abe_html_tokenizer.h` & `abe_html_tokenizer.c`
4. `kernel/browser_engine/html/abe_html_parser.h` & `abe_html_parser.c`
5. `kernel/browser_engine/html/abe_html_test.c`
6. `docs/architecture/atrix_browser_phase4_html_dom.md`

All other kernel subsystems and files are strictly locked.

---

## 3. Detailed Component Architecture Plan

### 3.1 Component 1: DOM Node Representation & Query Subsystem (`abe_dom_node.c` / `.h`)
- **What to modify:**
  - Increase `ABE_DOM_POOL_SLOTS` from 1024 to 4096.
  - Implement `ABE_DOM_GetElementById(root, id)` with recursive tree matching.
  - Implement `ABE_DOM_GetElementsByTagName(root, tag, out_array, max_count)` with wildcard `*` support.
  - Implement `ABE_DOM_GetElementsByClassName(root, class_name, out_array, max_count)` supporting multi-class tokens.
  - Implement `ABE_DOM_GetTextContent(node, out_buf, max_len)` and `ABE_DOM_SetTextContent(node, text)`.
  - Implement `ABE_DOM_GetInnerHTML(node, out_buf, max_len)` and `ABE_DOM_GetOuterHTML(node, out_buf, max_len)`.
  - Enforce strict parent, sibling, child pointer updates during `ABE_DOM_AppendChild`, `ABE_DOM_InsertBefore`, `ABE_DOM_RemoveChild`, `ABE_DOM_ReplaceChild`, and `ABE_DOM_CloneNode`.
- **Expected Result:** Safe, non-corrupting DOM operations with full tree invariants.

### 3.2 Component 2: Element & Attribute Model (`abe_html_element.c` / `.h`)
- **What to modify:**
  - Update `ABE_HTMLElement_IsVoidElement(tag)` to cover all HTML5 void elements: `area`, `base`, `br`, `col`, `embed`, `hr`, `img`, `input`, `link`, `meta`, `param`, `source`, `track`, `wbr`.
  - Implement `ABE_HTMLAttr_Get(elem, name)`, `ABE_HTMLAttr_Set(elem, name, val)`, `ABE_HTMLAttr_Remove(elem, name)`, `ABE_HTMLAttr_Has(elem, name)`.
- **Expected Result:** Robust attribute storage and query without DOM memory leaks.

### 3.3 Component 3: HTML5 Tokenizer Engine (`abe_html_tokenizer.c` / `.h`)
- **What to modify:**
  - Add states: `STATE_DOCTYPE`, `STATE_BEFORE_DOCTYPE_NAME`, `STATE_DOCTYPE_NAME`, `STATE_AFTER_DOCTYPE_NAME`.
  - Add RawText states for `<script>`, `<style>`, `<textarea>`, `<title>`: preserve raw text content until the exact closing tag is reached.
  - Implement complete character reference entity decoder supporting:
    - Named entities: `&quot;`, `&amp;`, `&lt;`, `&gt;`, `&apos;`, `&nbsp;`, `&copy;`, `&reg;`, `&trade;`, `&mdash;`, `&ndash;`, `&bull;`.
    - Decimal entities: `&#65;`, `&#...;` with integer bounds checking.
    - Hexadecimal entities: `&#x41;`, `&#X41;`, `&#x...;`.
- **Expected Result:** Zero tokenizer misinterpretations in script/style tags, robust entity conversion.

### 3.4 Component 4: HTML5 Tree Construction & Insertion Modes (`abe_html_parser.c` / `.h`)
- **What to modify:**
  - Implement insertion mode tracking: `MODE_INITIAL`, `MODE_BEFORE_HTML`, `MODE_BEFORE_HEAD`, `MODE_IN_HEAD`, `MODE_AFTER_HEAD`, `MODE_IN_BODY`, `MODE_IN_TABLE`, `MODE_IN_ROW`, `MODE_IN_CELL`, `MODE_AFTER_BODY`.
  - Implement implied element synthesis: auto-create `<html>`, `<head>`, `<body>` if omitted in source.
  - Implement optional end-tag auto-closing:
    - Auto-close `<p>` when encountering block tags (`p`, `div`, `h1`-`h6`, `table`, `ul`, `ol`, `form`, `blockquote`, `pre`).
    - Auto-close `<li>` when encountering next `<li>` or `</ul>`/`</ol>`.
    - Auto-close `<td>`/`<th>` when encountering next `<td>`/`<th>` or `</tr>`.
    - Auto-close `<tr>` when encountering next `<tr>` or `</table>`/`</tbody>`.
    - Auto-close `<option>` when encountering next `<option>` or `</select>`.
  - Implement Table structure builder: auto-insert `<tbody>` for `<tr>` directly under `<table>`.
  - Implement RawText context switching for `<script>`, `<style>`, `<textarea>`, `<title>`.
  - Implement deterministic stack search and adoption-agency recovery for misnested tags.
- **Expected Result:** Proper DOM hierarchy matching standard HTML5 rules for real-world websites.

### 3.5 Component 5: Deterministic Test Suite & Fuzz Hardening (`abe_html_test.c`)
- **What to modify:**
  - Implement test cases covering all 12 scenarios (Basic doc, Implied elements, Attributes, Entities, Misnesting, Lists, Tables, Script raw text, Style raw text, Deep malformed fuzz, Large document, Real network page).
- **Expected Result:** 100% automated regression verification during boot and testing.

---

## 4. Rollback Plan

If any regression occurs in Phase 1 (UI/Layout), Phase 2 (HTTP/1.1), or Phase 3 (TLS), the modifications can be cleanly reverted via Git without affecting external system subsystems.

---

## 5. Approval & Transition

Patch plan is approved. Transitioning to **Task 3 (Patch Team)** to apply the code changes.
