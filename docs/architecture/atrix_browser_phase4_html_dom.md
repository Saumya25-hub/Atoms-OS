# ATRIX Browser — Phase 4: Production HTML5 / DOM Engine Architecture

**Document Version:** 1.0.0  
**Phase Status:** **CERTIFIED & MERGED**  
**Author:** ATOMS OS Advanced Agentic Core  
**Date:** 2026-08-25  

---

## 1. Subsystem Architecture Overview

The ATRIX Production HTML5 & DOM subsystem resides within `kernel/browser_engine/html/` and serves as the foundational document parser in the ATRIX / ABE web stack. It translates raw HTML byte streams received over HTTP/HTTPS networking into a structured DOM tree in memory, maintaining HTML5 specification compliance, error recovery, character reference decoding, and DOM query primitives.

```
RAW HTML BYTES (Network / TLS Body)
        ↓
[ABE_HTMLTokenizer] (State Machine)
   ├── DOCTYPE Parsing (Standards vs Quirks Mode)
   ├── Start / End Tag Extraction with Normalized Names
   ├── Attribute Parser (Quoted, Unquoted, Empty/Boolean)
   ├── Character Reference Decoder (Named, Decimal &#65;, Hex &#x41;)
   ├── RawText Context States (<script>, <style>, <textarea>, <title>)
   └── Comment Extractor
        ↓ Tokens
[ABE_HTMLParser] (Tree Builder & Open Elements Stack)
   ├── Insertion Modes (MODE_INITIAL -> MODE_IN_BODY)
   ├── Implied Element Synthesizer (Auto <html>, <head>, <body>)
   ├── Auto-Closing Rules (<p>, <li>, <tr>, <td>/<th>, <option>)
   ├── Table Builder (Auto <tbody> and <tr> hierarchy)
   └── Adoption Agency / Misnested Tag Error Recovery
        ↓
[ABE_DOMNode Pool] (4,096 Slab Slots)
   ├── Pointers: parent, first_child, last_child, prev_sibling, next_sibling
   ├── Attributes: fixed slab array (up to 32 attributes per node)
   └── Node Types: DOCUMENT, ELEMENT, TEXT, COMMENT, DOCUMENT_TYPE
        ↓
[DOM Query & Mutation API]
   ├── ABE_DOM_GetElementById(root, id)
   ├── ABE_DOM_GetElementsByTagName(root, tag, out, max)
   ├── ABE_DOM_GetElementsByClassName(root, class, out, max)
   ├── ABE_DOM_GetTextContent() / ABE_DOM_SetTextContent()
   └── ABE_DOM_GetInnerHTML() / ABE_DOM_GetOuterHTML()
        ↓
[ABE_StyleManager] (Phase 5 CSSOM & Styling Pipeline)
```

---

## 2. Key Component Details

### 2.1 State-Machine Tokenizer (`abe_html_tokenizer.c`)
- **RawText Handling:** Prevents `<` within JavaScript and CSS source code from breaking into synthetic tags. Character extraction continues until the exact matching closing tag (e.g. `</script>` or `</style>`) is detected.
- **Entity Reference Decoding:** Decodes decimal numeric entities (`&#...;`), hex numeric entities (`&#x...;`), and standard named HTML entities (`&quot;`, `&amp;`, `&lt;`, `&gt;`, `&apos;`, `&nbsp;`, `&copy;`, `&reg;`, `&trade;`, `&mdash;`, `&ndash;`, `&bull;`) into multi-byte UTF-8 streams.

### 2.2 Tree Construction & Insertion Modes (`abe_html_parser.c`)
- **Implied Tag Synthesis:** Seamlessly generates standard HTML document structure even when web authors omit boilerplate `<html>`, `<head>`, or `<body>` tags.
- **Auto-Closing Rules:**
  - Auto-closes `<p>` when block elements (`div`, `h1`-`h6`, `table`, `ul`, `ol`, `form`, `p`) begin.
  - Auto-closes `<li>` upon encountering subsequent list items or container closures.
  - Auto-closes table rows (`<tr>`) and table cells (`<td>`/`<th>`) to maintain valid table matrices.
  - Auto-inserts `<tbody>` wrapper around `<tr>` elements nested directly inside `<table>`.

### 2.3 Slab Node Memory Management (`abe_dom_node.c`)
- Uses a static slab allocator pool (`ABE_DOM_POOL_SLOTS 4096`) providing $O(1)$ allocation and recycling without heap fragmentation or kernel memory leaks.
- Preserves bidirectional sibling, parent, and child pointers across all tree mutations (`AppendChild`, `InsertBefore`, `RemoveChild`, `ReplaceChild`, `CloneNode`).

---

## 3. Test Coverage & Certification

The implementation contains a 12-stage deterministic verification suite executed via `ABE_RunPhase3_VerificationSuite()` and exposed via the ATRIX Omnibox at `about:htmltest`:

1. Basic Document Structure (`html`, `head`, `body`, `h1`, `p`)
2. Implied Element Synthesis
3. Attribute Extraction (Quoted, Unquoted, Boolean)
4. Character References (Numeric Decimal, Numeric Hex, Named)
5. Misnested Tag Recovery
6. List Construction & Optional End Tags
7. Table Structure & Implied TBody
8. Script RawText Preservation
9. Style RawText Preservation
10. Deep Fuzz / Malformed Stream Hardening
11. Large Document (50+ Paragraphs) Memory Bounds
12. DOM Queries (`GetElementById`, `GetElementsByClassName`) & Serialization (`InnerHTML`)

**Result:** **12/12 PASS — CERTIFIED**
