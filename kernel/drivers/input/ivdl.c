#include "ivdl.h"
#include "input_abstraction.h"
#include "bovisual/Include/drawing.h"
#include "bovisual/Include/text.h"
#include "kernel/wm/surface/surface.h"

static void ivdl_itoa(int32_t val, char* buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    bool neg = false;
    if (val < 0) {
        neg = true;
        val = -val;
    }
    char temp[16];
    int i = 0;
    while (val > 0 && i < 15) {
        temp[i++] = '0' + (val % 10);
        val /= 10;
    }
    int j = 0;
    if (neg) buf[j++] = '-';
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

static void ivdl_strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

static void ivdl_strcat(char* dest, const char* src) {
    while (*dest) dest++;
    while ((*dest++ = *src++));
}

void IVDL_DrawOverlay(void) {
    const BVFontMetrics* font = BV_GetDefaultFont();
    if (!font) return;

    InputState state = input_get_latest_state();
    uint32_t hover_id = BWE_GetHoverSurfaceID();
    uint32_t focus_id = BOS_GetFocus();
    bool dragging = BWE_IsDragging();
    uint32_t drag_id = BWE_GetDragSurfaceID();

    int32_t hud_x = 10;
    int32_t hud_y = 10;
    int32_t hud_w = 260;
    int32_t hud_h = 116;

    // Draw HUD Background & Border
    BOVISUAL_Draw_FilledRectangle(hud_x, hud_y, hud_w, hud_h, 0xDD0F172A); // Dark slate
    BOVISUAL_Draw_Rectangle(hud_x, hud_y, hud_w, hud_h, 0xFF38BDF8);       // Sky blue border

    char line[64];
    char num[16];

    // Line 1: Header
    BOVISUAL_Draw_String(hud_x + 10, hud_y + 10, "[IVDL DEBUG HUD - 60FPS]", 0xFFFACC15, 0x00000000, true, font);

    // Line 2: Position
    ivdl_strcpy(line, "POS: X=");
    ivdl_itoa(state.mouse_x, num);
    ivdl_strcat(line, num);
    ivdl_strcat(line, " Y=");
    ivdl_itoa(state.mouse_y, num);
    ivdl_strcat(line, num);
    BOVISUAL_Draw_String(hud_x + 10, hud_y + 30, line, 0xFFFFFFFF, 0x00000000, true, font);

    // Line 3: Buttons
    ivdl_strcpy(line, "BTN: ");
    ivdl_strcat(line, (state.buttons & 1) ? "[L] " : " .  ");
    ivdl_strcat(line, (state.buttons & 4) ? "[M] " : " .  ");
    ivdl_strcat(line, (state.buttons & 2) ? "[R] " : " .  ");
    ivdl_strcat(line, "Raw:");
    ivdl_itoa(state.buttons, num);
    ivdl_strcat(line, num);
    BOVISUAL_Draw_String(hud_x + 10, hud_y + 50, line, (state.buttons != 0) ? 0xFF4ADE80 : 0xFF94A3B8, 0x00000000, true, font);

    // Line 4: Hover & Focus
    ivdl_strcpy(line, "HOVER: ");
    ivdl_itoa(hover_id, num);
    ivdl_strcat(line, num);
    ivdl_strcat(line, " | FOCUS: ");
    ivdl_itoa(focus_id, num);
    ivdl_strcat(line, num);
    BOVISUAL_Draw_String(hud_x + 10, hud_y + 70, line, 0xFF22D3EE, 0x00000000, true, font);

    // Line 5: Drag State
    ivdl_strcpy(line, "DRAG: ");
    if (dragging) {
        ivdl_strcat(line, "ACTIVE (ID ");
        ivdl_itoa(drag_id, num);
        ivdl_strcat(line, num);
        ivdl_strcat(line, ")");
    } else {
        ivdl_strcat(line, "IDLE");
    }
    BOVISUAL_Draw_String(hud_x + 10, hud_y + 90, line, dragging ? 0xFFFB923C : 0xFF94A3B8, 0x00000000, true, font);
}
