#include "../Include/drawing.h"
#include "../Include/graphics.h"
#include "../Include/geometry.h"

// Bresenham's Line Algorithm
void BOVISUAL_Draw_Line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, BOVISUAL_Color color) {
    int32_t dx = BV_Math_Abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int32_t dy = -BV_Math_Abs(y1 - y0), sy = y0 < y1 ? 1 : -1; 
    int32_t err = dx + dy, e2; 

    while (1) {
        BOVISUAL_Graphics_PutPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void BOVISUAL_Draw_Rectangle(int32_t x, int32_t y, int32_t width, int32_t height, BOVISUAL_Color color) {
    if (width <= 0 || height <= 0) return;
    
    BOVISUAL_Draw_Line(x, y, x + width - 1, y, color);
    BOVISUAL_Draw_Line(x, y + height - 1, x + width - 1, y + height - 1, color);
    BOVISUAL_Draw_Line(x, y, x, y + height - 1, color);
    BOVISUAL_Draw_Line(x + width - 1, y, x + width - 1, y + height - 1, color);
}

void BOVISUAL_Draw_FilledRectangle(int32_t x, int32_t y, int32_t width, int32_t height, BOVISUAL_Color color) {
    // Rely on Graphics optimized fill
    BOVISUAL_Graphics_Fill(x, y, width, height, color);
}

// Midpoint Circle Algorithm
void BOVISUAL_Draw_Circle(int32_t x0, int32_t y0, int32_t radius, BOVISUAL_Color color) {
    int32_t x = radius;
    int32_t y = 0;
    int32_t err = 0;

    while (x >= y) {
        BOVISUAL_Graphics_PutPixel(x0 + x, y0 + y, color);
        BOVISUAL_Graphics_PutPixel(x0 + y, y0 + x, color);
        BOVISUAL_Graphics_PutPixel(x0 - y, y0 + x, color);
        BOVISUAL_Graphics_PutPixel(x0 - x, y0 + y, color);
        BOVISUAL_Graphics_PutPixel(x0 - x, y0 - y, color);
        BOVISUAL_Graphics_PutPixel(x0 - y, y0 - x, color);
        BOVISUAL_Graphics_PutPixel(x0 + y, y0 - x, color);
        BOVISUAL_Graphics_PutPixel(x0 + x, y0 - y, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

// Midpoint Ellipse Algorithm
void BOVISUAL_Draw_Ellipse(int32_t x0, int32_t y0, int32_t rx, int32_t ry, BOVISUAL_Color color) {
    int32_t rxSq = rx * rx;
    int32_t rySq = ry * ry;
    int32_t x = 0, y = ry;
    int32_t px = 0, py = 2 * rxSq * y;

    // Region 1
    int32_t p = (int32_t)(rySq - (rxSq * ry) + (0.25 * rxSq)); // Initial decision parameter (cast to integer equivalent if avoiding float, effectively avoiding float by scaling could be done, but let's stick to standard integer Midpoint ellipse for now)
    // To strictly avoid float, p1 = rySq - rxSq*ry + rxSq/4. 
    int32_t p1 = rySq - (rxSq * ry) + (rxSq / 4);

    while (px < py) {
        BOVISUAL_Graphics_PutPixel(x0 + x, y0 + y, color);
        BOVISUAL_Graphics_PutPixel(x0 - x, y0 + y, color);
        BOVISUAL_Graphics_PutPixel(x0 + x, y0 - y, color);
        BOVISUAL_Graphics_PutPixel(x0 - x, y0 - y, color);

        x++;
        px += 2 * rySq;
        if (p1 < 0) {
            p1 += rySq + px;
        } else {
            y--;
            py -= 2 * rxSq;
            p1 += rySq + px - py;
        }
    }

    // Region 2
    int32_t p2 = (rySq) * (x + 1) * (x + 1) + (rxSq) * (y - 1) * (y - 1) - (rxSq * rySq); // roughly equivalent

    while (y >= 0) {
        BOVISUAL_Graphics_PutPixel(x0 + x, y0 + y, color);
        BOVISUAL_Graphics_PutPixel(x0 - x, y0 + y, color);
        BOVISUAL_Graphics_PutPixel(x0 + x, y0 - y, color);
        BOVISUAL_Graphics_PutPixel(x0 - x, y0 - y, color);

        y--;
        py -= 2 * rxSq;
        if (p2 > 0) {
            p2 += rxSq - py;
        } else {
            x++;
            px += 2 * rySq;
            p2 += rxSq - py + px;
        }
    }
}
