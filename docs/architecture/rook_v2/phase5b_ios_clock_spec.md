# ♜ ATOMS OS — Architecture Specification
## Phase 5B: Apple iOS 17 SF Pro Display Heavy Bold Lock Screen Typography & Dual-Layer Compositing Engine

**Document ID:** `ARCH-ROOK-V2-PHASE5B-CLOCK`  
**Status:** `APPROVED FOR IMPLEMENTATION`  
**Target Subsystem:** `kernel/shell/rook/pages/page_login.c`  
**Hardware Verification:** Intel Haswell LGA1150 / H81 Motherboard / 1080p Display  

---

### 1. Architectural Philosophy & Executive Summary

Standard hobby OS implementations render lock screen clocks using crude 8x16 bitmap fonts scaled with box nearest-neighbor filters, or thin geometric wireframes lacking optical mass. This results in staircasing ("jagged pixels"), visual clipping, and complete loss of contrast against complex, high-dynamic-range wallpapers (such as the snowy sunset mountain asset).

Production operating systems (Apple iOS 17 Lock Screen, macOS Sonoma, Windows 11 Segoe Variable UI) solve this via:
1. **Optical Weight & Proportions:** SF Pro Display Heavy / Heavy Bold geometry ($160\text{px}$ height, $20\text{px}$ stroke width, flat-cut terminals, circular dots).
2. **4x Multi-Sampled Anti-Aliasing (MSAA) Alpha Coverage:** Calculating 16 subpixel samples per boundary pixel to guarantee continuous $\alpha \in [0, 255]$ edge gradient.
3. **Dual-Layer Glass Ambient Compositing:** A soft ambient occlusion drop shadow ($+1\text{px} X, +3\text{px} Y$, $\alpha \approx 110$) rendered beneath an Ice-White ($0\text{xFFF8FAFC}$) foreground glyph.
4. **Hierarchical Vertical Alignment:** Optical balancing of Lock Icon $\rightarrow$ Date Subtext $\rightarrow$ Large Time.

---

### 2. Spatial Layout & Optical Hierarchy

In a $1920 \times 1080$ dense canvas ($cx = 960, cy = 540$):

```
                        [ Lock Icon 🔒 (24x24px) ]           y = cy - 200 (340px)
                                    │
                               (24px gap)
                                    │
                     [ Date Text: "Saturday, August 15" ]   y = cy - 152 (388px)
                                    │
                               (20px gap)
                                    │
                ╔═══════════════════════════════════════╗
                ║          1  8   :   5  4              ║    y = cy - 110 .. cy + 50 (430px..590px)
                ╚═══════════════════════════════════════╝    Height: 160px, Width per digit: ~84px
                                                             Colon: Dual Spherical Dots (16x16px)
```

---

### 3. Glyph Geometry & 4x MSAA Rasterization Model

Each digit is represented by its mathematical parametric contours:
* **Height:** $H = 160\text{px}$
* **Digit Width:** $W = 84\text{px}$
* **Stroke Width:** $T = 20\text{px}$
* **Corner Radius:** $R = 14\text{px}$
* **Colon Dot Diameter:** $D = 18\text{px}$, vertical centers at $y = 52\text{px}$ and $y = 108\text{px}$.

#### 4x Subpixel Coverage Equation:
For every pixel $(x, y) \in [0, W) \times [0, H)$:
$$\alpha(x, y) = \frac{1}{16} \sum_{i=0}^{3} \sum_{j=0}^{3} \mathbb{I}\left(\text{InsideContour}\left(x + \frac{2i+1}{8}, y + \frac{2j+1}{8}\right)\right) \times 255$$

---

### 4. Dual-Layer Ambient Compositing Engine

#### Layer 1: Ambient Drop Shadow
* **Offset:** $\Delta x = +1, \Delta y = +3$
* **Shadow Color:** $0\text{xFF000000}$ (Black)
* **Shadow Opacity:** $\alpha_{\text{shadow}} = (\alpha_{\text{glyph}} \times 110) / 255$
* **Blend Equation:**
  $$C_{\text{out}} = \text{AlphaBlend}(C_{\text{bg}}, 0\text{x000000}, \alpha_{\text{shadow}})$$

#### Layer 2: Core Frost-White Glyphs
* **Foreground Color:** $0\text{xFFF8FAFC}$ (Crisp Ice-White)
* **Blend Equation:**
  $$C_{\text{out}} = \text{AlphaBlend}(C_{\text{shadow}}, 0\text{xFFF8FAFC}, \alpha_{\text{glyph}})$$

---

### 5. Date Calculation (Hardware Gregorian Day-of-Week)

Zeller’s Congruence algorithm implementation:
$$h = \left(q + \left\lfloor\frac{13(m+1)}{5}\right\rfloor + K + \left\lfloor\frac{K}{4}\right\rfloor + \left\lfloor\frac{J}{4}\right\rfloor - 2J\right) \pmod 7$$
Where:
* $q = \text{day of month}$
* $m = \text{month (March=3 .. Jan=13, Feb=14)}$
* $K = \text{year} \pmod{100}$
* $J = \lfloor\text{year} / 100\rfloor$

Produces exact English Day string: `"Monday" .. "Sunday"` and Month `"January" .. "December"`.

---

### 6. Zero-Allocation Guarantee
* **Static Memory:** All glyph coverage tables pre-evaluated into static read-only buffers or computed via analytic fixed-point math.
* **Heap Usage:** $0$ bytes (`kmalloc` / `kfree` strictly forbidden).
* **Frame Time:** $< 0.8\text{ms}$ rendering budget on Intel Haswell Core i3.
