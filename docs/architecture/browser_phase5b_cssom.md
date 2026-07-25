# ATOMS OS / ATRIX Browser — Phase 5B: CSS Parser & CSSOM Engine

## Overview

The **Phase 5B CSS Subsystem** provides a production-grade CSS Tokenizer, Recursive Descent Parser, CSS Object Model (CSSOM), Specificity Engine, and Cascading Style Resolver for the ATRIX Browser in ATOMS OS. 

Designed following modern browser architectural patterns (Blink/WebKit/Gecko), the engine parses raw CSS text into structured object models (`CSSStyleSheet`, `CSSRule`, `CSSSelector`, `CSSDeclaration`, `CSSValue`) and computes resolved styles (`css_computed_style_t`) for DOM tree nodes (`bos_node_t`).

---

## Component Architecture

```
                       Raw CSS Input String
                                │
                                ▼
                       ┌──────────────────┐
                       │  CSS Tokenizer   │
                       └────────┬─────────┘
                                │ Stream of CSS Tokens
                                ▼
                       ┌──────────────────┐
                       │    CSS Parser    │
                       └────────┬─────────┘
                                │ Builds CSSOM
                                ▼
  ┌───────────────────────────────────────────────────────────┐
  │                       CSSOM Tree                          │
  │  CSSStyleSheet ──► CSSRule ──► CSSSelector                │
  │                                └──► CSSDeclaration ──► Value │
  └─────────────────────────────┬─────────────────────────────┘
                                │
               DOM Node         │   CSS Stylesheet(s)
              (bos_node_t)      │
                   │            │
                   ▼            ▼
             ┌──────────────────────────┐
             │  Selector Matcher &      │
             │  Specificity Engine      │
             └──────────┬───────────────┘
                        │ Specificity (A, B, C)
                        ▼
             ┌──────────────────────────┐
             │ Cascading Style Resolver │
             └──────────┬───────────────┘
                        │
                        ▼
              css_computed_style_t (For Phase 5C Layout Engine)
```

---

## File Hierarchy (`browser/css/`)

| File Path | Description |
| :--- | :--- |
| `browser/css/include/css_types.h` | Core types, enums, color, token, rule, selector, stylesheet & computed style structures. |
| `browser/css/include/bos_css.h` | Public Facade API header exposing subsystem lifecycles and tests. |
| `browser/css/bos_css.c` | Implementation of public facade, diagnostic dumps, and test triggers. |
| `browser/css/css_tokenizer.h` / `.c` | Lexical scanner producing CSS tokens, handling whitespace and comment stripping. |
| `browser/css/css_value.h` / `.c` | Hex, RGB, RGBA, named color parsing, dimension, percent, and keyword value handling. |
| `browser/css/css_specificity.h` / `.c` | Implementation of standard CSS specificity `(a, b, c)` calculation & ordering. |
| `browser/css/css_selector.h` / `.c` | Selector parsing (Tag, Class, ID, Universal) and DOM node matching engine (Combinators: `>`, `+`, `~`, descendant). |
| `browser/css/css_rule.h` / `.c` | Rule creation, declaration list management, `!important` flag parsing, and destruction. |
| `browser/css/css_stylesheet.h` / `.c` | Stylesheet container management and recursive destruction without memory leaks. |
| `browser/css/css_parser.h` / `.c` | Recursive descent CSS parser with fault-tolerant error recovery. |
| `browser/css/css_style.h` / `.c` | Cascading style resolver evaluating element style matches against stylesheet rules. |
| `browser/css/css_tests.h` / `.c` | Certification test suite validating all 9 Phase 5B requirements. |

---

## Key Data Structures

### 1. `css_selector_t`
Represents simple and complex selectors with specificity triplets:
```c
typedef struct css_selector {
    css_selector_item_t* items_head;
    uint32_t spec_a; // ID count
    uint32_t spec_b; // Class / attribute count
    uint32_t spec_c; // Tag count
    struct css_selector* next_group; // Selector groups (e.g. h1, h2)
} css_selector_t;
```

### 2. `css_declaration_t`
Represents key-value style properties:
```c
typedef struct css_declaration {
    css_property_id_t prop_id;
    char name[64];
    css_value_t value;
    bool is_important;
    struct css_declaration* next;
} css_declaration_t;
```

### 3. `css_computed_style_t`
Final resolved style structure passed to the Layout Engine:
```c
typedef struct {
    css_color_t color;
    css_color_t background_color;
    float width;
    float height;
    float margin_top, margin_right, margin_bottom, margin_left;
    float padding_top, padding_right, padding_bottom, padding_left;
    float font_size;
    float opacity;
    char display[32];
    char visibility[32];
    char position[32];
    char font_family[64];
    char text_align[32];
    char overflow[32];
} css_computed_style_t;
```

---

## Specificity & Cascade Algorithm

1. **Specificity Triplet $(a, b, c)$**:
   - $a$: Count of `#id` selectors.
   - $b$: Count of `.class`, attribute, and pseudo-class selectors.
   - $c$: Count of `tag` and pseudo-element selectors.
   - Universal selector `*` contributes $(0, 0, 0)$.

2. **Cascade Resolution Order**:
   1. `!important` declarations override normal declarations.
   2. Higher specificity $(a, b, c)$ overrides lower specificity.
   3. For equal specificity, the rule declared later in the stylesheet wins.

---

## Fault Tolerance & Error Recovery

- **Comment Stripping**: Automatically consumes `/* ... */` multi-line comments.
- **Malformed Rules**: If an unclosed declaration or unknown property is encountered, the parser skips tokens up to the next `;` or `}` without panicking, crashing, or throwing kernel exceptions.
- **Memory Safety**: Every CSS object has an explicit destructor (`css_rule_destroy`, `css_stylesheet_destroy`) ensuring zero heap memory leaks.

---

## Certification Suite

| Test Name | Validation Objective | Result |
| :--- | :--- | :--- |
| `Tokenizer` | Correct tokenization of identifiers, symbols, dimensions, and comments. | **PASS** |
| `Selectors` | Matching element tags, IDs, classes, and combinators against DOM nodes. | **PASS** |
| `Declarations` | Correct parsing of property names, values, and dimensions. | **PASS** |
| `Specificity` | Correct calculation and comparison of $(a, b, c)$ specificity triplets. | **PASS** |
| `Cascade` | Proof that higher specificity and rule order resolve correctly. | **PASS** |
| `Colors` | Hex (`#RGB`, `#RRGGBB`), `rgb()`, `rgba()`, and named colors parsing. | **PASS** |
| `Malformed CSS` | Recovery from invalid properties and syntax errors without panic. | **PASS** |
| `Memory Leak` | Iterative creation and destruction cycles with zero leaked heap blocks. | **PASS** |
| `Large Stylesheet` | Parsing 100 KB CSS stylesheet containing 1000+ rules under 50 ms inside QEMU. | **PASS** |

---

## Future Extensibility (Phase 5C Integration)

The `css_computed_style_t` object produced by `bos_css_compute_style()` directly feeds the Phase 5C Layout Engine to generate box models, flex layouts, inline formatting contexts, and render trees.
