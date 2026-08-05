# Cb Plane vs Cr Plane Divergence Audit Report

**Target Files**:
- [`mjpeg_decoder.c`](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c)
- [`software_backend.c`](file:///D:/Signatures_OS/kernel/media/bospectra/render/backends/software/software_backend.c)

**Audit Scope**:
Verification of Cb (chrominance blue) vs Cr (chrominance red) plane handling across JPEG parsing, Huffman/Quantization table mapping, MCU block decoding order, plane buffer allocation, and YUV420-to-ARGB32 color space conversion.

---

## Executive Summary

An in-depth forensic code audit of `mjpeg_decoder.c` and `software_backend.c` was performed to detect potential functional, structural, or mathematical divergences between the Cb and Cr chroma planes.

### Key Finding & Verdict
- **Overall Parity**: Both `mjpeg_decoder.c` and `software_backend.c` maintain high symmetry in quantization table selection, Huffman table mapping, IDCT transformations, bilinear chroma filtering, and BT.601 color matrix equations.
- **Architectural Vulnerability Identified**:
  - **Implicit Component Order Assumption**: `mjpeg_decoder.c` decodes components into frame plane buffers `frame->data[1]` (Cb) and `frame->data[2]` (Cr) based on SOF0 component array index (`ci = 1` vs `ci = 2`). If an incoming MJPEG stream defines SOF0/SOS components out of standard sequence (e.g. ID 1=Y, ID 3=Cr, ID 2=Cb), the decoder will route Cr data into `frame->data[1]` and Cb data into `frame->data[2]`, resulting in a red/blue color channel swap.

---

## Detailed Audit Findings

### 1. SOF0 Component IDs (`comp[1]` Cb vs `comp[2]` Cr)

- **Source Code Reference**: [`mjpeg_decoder.c` L362-L376](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c#L362-L376), [`L383-L404`](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c#L383-L404), [`L452-L488`](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c#L452-L488).
- **Trace Details**:
  - During **SOF0** parsing, header components are read sequentially into `ctx->comp[c]` for $c = 0 \dots \text{num\_components}-1$:
    - `ctx->comp[0]`: Luminance (Y), expected ID = 1 ($0x01$).
    - `ctx->comp[1]`: Chrominance Blue (Cb), expected ID = 2 ($0x02$).
    - `ctx->comp[2]`: Chrominance Red (Cr), expected ID = 3 ($0x03$).
  - During **SOS** parsing, `comp_id` is matched against `ctx->comp[ci].id` to assign `dc_huff_idx` and `ac_huff_idx`.
  - In the **MCU Decoding Loop**, the bitstream is parsed using array indices `ci = 0, 1, 2`, writing output blocks directly to `frame->data[ci]`.
- **Divergence Assessment**:
  - **Symmetric Code Logic**: `comp[1]` (Cb) and `comp[2]` (Cr) utilize identical structure fields (`h_samp`, `v_samp`, `quant_tbl_idx`, `dc_huff_idx`, `ac_huff_idx`, `dc_pred`).
  - **Risk**: Hardcoded mapping of array index `ci=1` to `frame->data[1]` (Cb) and `ci=2` to `frame->data[2]` (Cr) assumes standard component ordering in the SOF0 marker. If a non-standard JPEG stream specifies Cr before Cb in SOF0, color channels will be inverted.

---

### 2. Quantization Table Selection

- **Source Code Reference**: [`mjpeg_decoder.c` L323-L336](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c#L323-L336), [`L375`](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c#L375), [`L465-L469`](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c#L465-L469).
- **Trace Details**:
  - `JPEG_DQT` segment populates `ctx->quant_tables[qt_idx][64]` for up to 4 quantization tables.
  - `SOF0` sets `ctx->comp[c].quant_tbl_idx` from the third component parameter byte.
  - In the MCU decoding loop:
    ```c
    uint8_t q_idx = ctx->comp[ci].quant_tbl_idx;
    const int16_t* qtbl = ctx->quant_tables[q_idx];
    if (qtbl[0] == 0 && ctx->quant_tables[0][0] != 0) {
        qtbl = ctx->quant_tables[0];
    }
    ```
- **Divergence Assessment**:
  - **Complete Parity**: Both `ci=1` (Cb) and `ci=2` (Cr) dynamically fetch their respective table index `quant_tbl_idx` from `ctx->comp[ci]`. Both share the exact same fallback guard to `quant_tables[0]` when the designated chrominance quantization table is missing or uninitialized (`qtbl[0] == 0`).

---

### 3. Huffman Table Mapping

- **Source Code Reference**: [`mjpeg_decoder.c` L338-L360](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c#L338-L360), [`L383-L404`](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c#L383-L404), [`L471-L476`](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c#L471-L476).
- **Trace Details**:
  - `JPEG_DHT` stores DC tables in slots 0–3 and AC tables in slots 4–7 (`tbl_slot = ht_type * 4 + ht_idx`).
  - `JPEG_SOS` parses table selections for each scan component:
    - `dc_idx = (ht_sel >> 4) & 0xF`
    - `ac_idx = 4 + (ht_sel & 0xF)`
    - Updates `ctx->comp[ci].dc_huff_idx` and `ctx->comp[ci].ac_huff_idx` by matching component ID `ctx->comp[ci].id == comp_id`.
  - During block decoding:
    - `dc_ht` selected via `ctx->comp[ci].dc_huff_idx` (fallback to `huff[0]` if invalid).
    - `ac_ht` selected via `ctx->comp[ci].ac_huff_idx` (fallback to `huff[4]` if invalid).
- **Divergence Assessment**:
  - **Complete Parity**: Huffman table resolution is symmetric between Cb and Cr. Fallback guards prevent decoder crashes when single-table baseline JPEGs are encountered.

---

### 4. MCU Block Order (`Y0, Y1, Y2, Y3, Cb, Cr`)

- **Source Code Reference**: [`mjpeg_decoder.c` L420-L489](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.c#L420-L489), [`idct.c` L17-L116](file:///D:/Signatures_OS/kernel/media/bospectra/decoder/common/idct.c#L17-L116).
- **Trace Details**:
  - Sampling factors for 4:2:0 YUV baseline:
    - Component 0 (Y): `h_samp = 2`, `v_samp = 2` $\rightarrow$ 4 blocks ($Y_0, Y_1, Y_2, Y_3$).
    - Component 1 (Cb): `h_samp = 1`, `v_samp = 1` $\rightarrow$ 1 block.
    - Component 2 (Cr): `h_samp = 1`, `v_samp = 1` $\rightarrow$ 1 block.
  - Interleaved bitstream order per MCU:
    1. $Y_0$ at $(vy=0, hx=0)$
    2. $Y_1$ at $(vy=0, hx=1)$
    3. $Y_2$ at $(vy=1, hx=0)$
    4. $Y_3$ at $(vy=1, hx=1)$
    5. $Cb$ at $(vy=0, hx=0)$
    6. $Cr$ at $(vy=0, hx=0)$
  - Output plane coordinate calculations:
    - `p_w = (ci == 0) ? dec_w : (dec_w / max_h)` $\rightarrow$ `dec_w / 2` for chroma.
    - `p_h = (ci == 0) ? dec_h : (dec_h / max_v)` $\rightarrow$ `dec_h / 2` for chroma.
    - `bx = mx * (hs * 8) + hx * 8` $\rightarrow$ `mx * 8` for Cb and Cr.
    - `by = my * (vs * 8) + vy * 8` $\rightarrow$ `my * 8` for Cb and Cr.
- **Divergence Assessment**:
  - **Complete Parity**: Block traversal order ($Y_0 \to Y_1 \to Y_2 \to Y_3 \to Cb \to Cr$) conforms strictly to ISO/IEC 10918-1 section A.2.3. Cb and Cr coordinates inside their respective half-resolution planes are identical.

---

### 5. YUV420 to ARGB Conversion

- **Source Code Reference**: [`software_backend.c` L78-L149](file:///D:/Signatures_OS/kernel/media/bospectra/render/backends/software/software_backend.c#L78-L149).
- **Trace Details**:
  - Plane Bindings:
    - `y_plane = frame->data[0]`
    - `u_plane = frame->data[1]` (Cb)
    - `v_plane = frame->data[2]` (Cr)
  - Strides:
    - `y_stride = frame->linesize[0]`
    - `u_stride = frame->linesize[1]`
    - `v_stride = frame->linesize[2]`
  - 2D Bilinear Chroma Filtering:
    - Cb sample `cb_val`: Bilinear interpolation of 4 adjacent Cb texels (`u00`, `u01`, `u10`, `u11`).
    - Cr sample `cr_val`: Bilinear interpolation of 4 adjacent Cr texels (`v00`, `v01`, `v10`, `v11`).
    - Fixed-point fractional weighting (`h_frac`, `v_frac`) normalized by shift `>> 20`.
    - Zero-point offset subtraction: `Cb = cb_val - 128`, `Cr = cr_val - 128`.
  - Full-Range BT.601 Fixed-Point Matrix Equations:
    $$\begin{aligned}
    R &= Y + \left\lfloor \frac{1436 \cdot Cr + 512}{1024} \right\rfloor \\
    G &= Y - \left\lfloor \frac{352 \cdot Cb + 731 \cdot Cr + 512}{1024} \right\rfloor \\
    B &= Y + \left\lfloor \frac{1815 \cdot Cb + 512}{1024} \right\rfloor
    \end{aligned}$$
  - ARGB32 Packing:
    - `dst_row[col] = 0xFF000000U | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;`
- **Divergence Assessment**:
  - **Complete Parity**: `u_plane` (Cb) and `v_plane` (Cr) are processed symmetrically. Color conversion coefficients implement standard JFIF BT.601 conversions ($1.402, -0.34414, -0.71414, 1.772$).

---

## Summary Matrix

| Trace Point | `mjpeg_decoder.c` Status | `software_backend.c` Status | Divergence Detected? |
| :--- | :--- | :--- | :--- |
| **SOF0 Component IDs** | Component array index `ci=1` mapped to Cb, `ci=2` mapped to Cr. | Expects `data[1]`=Cb, `data[2]`=Cr. | **Potential Vulnerability** (Out-of-order SOF0 stream component IDs lead to R/B swap) |
| **Quant Table Selection** | Dynamic per component (`comp[ci].quant_tbl_idx`); identical fallback to table 0. | N/A (Frame already dequantized) | **No Divergence** |
| **Huffman Table Mapping** | Dynamic per component via `comp_id`; identical fallbacks (slot 0/4). | N/A (Frame already entropy-decoded) | **No Divergence** |
| **MCU Block Order** | Strict $Y_0, Y_1, Y_2, Y_3, Cb, Cr$ bitstream sequence. | N/A (Raster planar input) | **No Divergence** |
| **YUV420 to ARGB Conversion**| Generates `YUV420P` planar output (`data[0..2]`). | Bilinear Cb/Cr sampling & BT.601 matrix equation. | **No Divergence** |

---

## Recommendations for Hardening

1. **SOF0 Component Order Validation**:
   In `mjpeg_decoder.c`, map output frame planes by checking `ctx->comp[ci].id` rather than assuming `ci=1` is Cb and `ci=2` is Cr:
   ```c
   uint8_t plane_idx = 0;
   if (ctx->comp[ci].id == 2)      plane_idx = 1; /* Cb */
   else if (ctx->comp[ci].id == 3) plane_idx = 2; /* Cr */
   else                            plane_idx = ci;
   uint8_t* plane = frame->data[plane_idx];
   ```
