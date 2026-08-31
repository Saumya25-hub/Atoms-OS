# ATRIX Browser — Phase 5: Production CSS Engine Architecture

**Document Version:** 1.0.0  
**Phase:** Phase 5 — Production CSS Engine  
**Status:** **CERTIFIED & LOCKED ✅**  
**Target:** ATOMS OS Native Browser Subsystem (ABE / ATRIX)  

---

## 1. Overview

The ATRIX Browser CSS Engine is an authoritative, Ring-0 native CSS3 and CSSOM implementation built specifically for ATOMS OS. It transforms raw CSS stylesheet bytes from `<style>` elements, external stylesheets, and inline `style="..."` attributes into structured CSSOM representations, performs lexicographical selector matching and specificity calculation, executes the 4-tier cascade with `!important` support, applies property inheritance, substitutes CSS variables (`var()`), evaluates media queries against viewport dimensions, and generates computed style records for DOM nodes to feed into the Phase 6 Layout Engine.

```
RAW CSS TEXT / <style>
          ↓
  [ CSS TOKENIZER ]
          ↓  (Tokens: IDENT, DIMENSION, NUMBER, HASH, FUNCTION, BRACKETS, COMMA, IMPORTANT, AT_RULE)
   [ CSS PARSER ]
          ↓  (CSSOM: Stylesheets, Rules, Declarations, Custom Properties, Media Queries)
 [ SELECTOR ENGINE ]
          ↓  (Universal, Type, Class, ID, Attributes, Siblings, Child, Descendant, Pseudo-classes)
[ SPECIFICITY CALCULATOR ]
          ↓  (a, b, c) Lexicographical Comparison
  [ CASCADE RESOLVER ]
          ↓  (!important ➔ Origin [Inline > Author > UA] ➔ Specificity ➔ Source Order)
[ INHERITANCE ENGINE ]
          ↓  (Whitelist: color, font-size, font-weight, font-family, line-height, text-align, visibility)
[ COMPUTED STYLE ENGINE ]
          ↓  (4096-Node Computed Store, var() substitution, @media viewport filtering)
  PHASE 6 LAYOUT CONTRACT
```

---

## 2. Tokenizer Architecture (`abe_css_tokenizer.c` / `.h`)

The CSS tokenizer implements a state-machine lexical scanner:
- **Token Taxonomy:**
  - `CSSTOKEN_IDENT`: Element tags, property names, keywords (`block`, `inline`, `auto`, `inherit`, `initial`, `none`).
  - `CSSTOKEN_STRING`: Single or double quoted literals.
  - `CSSTOKEN_NUMBER`: Raw integer or floating-point values.
  - `CSSTOKEN_DIMENSION`: Numbers with trailing unit identifiers (`px`, `em`, `rem`, `vw`, `vh`, `pt`).
  - `CSSTOKEN_PERCENTAGE`: Numbers with trailing `%`.
  - `CSSTOKEN_HASH`: `#id` selectors and hex color strings.
  - `CSSTOKEN_COLON`, `CSSTOKEN_SEMICOLON`, `CSSTOKEN_LBRACE`, `CSSTOKEN_RBRACE`, `CSSTOKEN_LPAREN`, `CSSTOKEN_RPAREN`, `CSSTOKEN_LBRACKET`, `CSSTOKEN_RBRACKET`, `CSSTOKEN_COMMA`.
  - `CSSTOKEN_FUNCTION`: Functions (`rgb(`, `rgba(`, `var(`, `calc(`).
  - `CSSTOKEN_IMPORTANT`: `!important` priority modifier.
  - `CSSTOKEN_AT_RULE`: `@media`, `@import`.
- **Color Parsing:**
  - `#RGB` ➔ `#RRGGBB` (3-digit expansion).
  - `#RGBA` ➔ 4-digit alpha expansion.
  - `#RRGGBB` / `#RRGGBBAA` (6- and 8-digit hex values).
  - `rgb(r, g, b)` and `rgba(r, g, b, a)` function expressions.
  - Named CSS colors (`red`, `green`, `blue`, `black`, `white`, `yellow`, `cyan`, `magenta`, `gray`, `silver`, `maroon`, `purple`, `navy`, `teal`, `olive`, `orange`, `transparent`, `lightgray`, `darkgray`, `crimson`, `indigo`, `violet`, `pink`, `brown`).

---

## 3. CSSOM & Parser Architecture (`abe_css_parser.c` / `.h`)

- **Capacities:**
  - `ABE_MAX_STYLESHEETS`: 32 active stylesheets.
  - `ABE_MAX_CSS_RULES`: 512 rules per stylesheet.
  - `ABE_MAX_CSS_DECLARATIONS`: 64 declarations per rule.
  - `ABE_MAX_CUSTOM_PROPERTIES`: 32 custom properties per stylesheet.
- **Custom Properties Table:**
  - Variables declared via `--custom-name: value;` are stored in stylesheet lookup tables and retrieved via `ABE_CSSParser_GetCustomProperty()`.
- **Media Queries:**
  - `@media` blocks are parsed for conditions (`min-width`, `max-width`, `min-height`, `max-height`) and bound to inner rules.
- **Error Recovery:**
  - Unrecognized properties or missing semicolons skip safely to the next delimiter without corrupting the parser state or leaking kernel heap.

---

## 4. Selector Engine & Specificity (`abe_css_selector.c` / `.h`)

- **Supported Selectors:**
  - Universal: `*`
  - Type / Tag: `div`, `p`, `h1`
  - Class: `.class` (with full whitespace-separated class list matching in `class="..."`)
  - ID: `#id`
  - Compound: `div.foo#main`
  - Attribute: `[attr]`, `[attr="val"]`, `[attr~="val"]`
  - Combinators:
    - Descendant (`A B`): Any matching ancestor in DOM hierarchy.
    - Child (`A > B`): Direct parent element matching.
    - Adjacent Sibling (`A + B`): Immediately preceding element sibling.
    - General Sibling (`A ~ B`): Any preceding element sibling.
  - Pseudo-Classes: `:first-child`, `:last-child`, `:disabled`, `:checked`, `:hover`, `:focus`
  - Selector Lists: Comma-separated list support (`h1, h2, h3`)
- **Specificity Tuple $(A, B, C)$:**
  - $A$: Number of ID selectors (`#id`).
  - $B$: Number of class (`.class`), attribute (`[...]`), and pseudo-class (`:hover`) selectors.
  - $C$: Number of tag selectors (`div`) and pseudo-elements (`::before`).
  - Lexicographical comparator: if $A_1 \neq A_2$, return $A_1 - A_2$; else if $B_1 \neq B_2$, return $B_1 - B_2$; else return $C_1 - C_2$.

---

## 5. Cascade & Inheritance (`abe_css_cascade.c` / `abe_css_inherit.c`)

- **4-Tier Cascade Resolution Hierarchy:**
  1. **Importance:** Declarations with `!important` override all non-important declarations regardless of origin or specificity.
  2. **Origin:** Inline styles (`ORIGIN_INLINE`) > Author stylesheets (`ORIGIN_AUTHOR`) > User-Agent defaults (`ORIGIN_USER_AGENT`).
  3. **Specificity:** Higher $(A, B, C)$ specificity wins.
  4. **Source Order:** Later declarations in stylesheet override earlier declarations.
- **Property Inheritance:**
  - Inherited: `color`, `font-size`, `font-weight`, `font-family`, `line-height`, `text-align`, `visibility`.
  - Non-Inherited: `margin`, `padding`, `border`, `background`, `width`, `height`, `display`, `position`, `overflow`, `opacity`.

---

## 6. Computed Style Engine Contract (`abe_css_computed.c` / `.h`)

The computed style engine produces a finalized `ABE_ComputedStyle` struct for every DOM node in the 4,096-slot pool:
- `display`: `BLOCK`, `INLINE`, `INLINE_BLOCK`, `NONE`, `FLEX`, `GRID`, `TABLE`
- `position`: `STATIC`, `RELATIVE`, `ABSOLUTE`, `FIXED`, `STICKY`
- `width_px`, `width_auto`, `height_px`, `height_auto`
- `margin_top_px`, `margin_right_px`, `margin_bottom_px`, `margin_left_px`
- `padding_top_px`, `padding_right_px`, `padding_bottom_px`, `padding_left_px`
- `border_top_width_px`, `border_right_width_px`, `border_bottom_width_px`, `border_left_width_px`
- `color`, `background_color` (32-bit ARGB)
- `font_size_px`, `font_weight`, `line_height_px`, `text_align`, `font_family[64]`
- `opacity`, `visibility`, `overflow`

This struct serves as the immutable input contract for **Phase 6: Full Layout Engine**.

---

## 7. Verification & Omnibox Diagnostic

- **15/15 Deterministic Test Suite:** Executed via `about:csstest` or `about:css` in ATRIX Omnibox.
- **Verification Result:** **15/15 PASS, 0 errors, 0 regressions.**
