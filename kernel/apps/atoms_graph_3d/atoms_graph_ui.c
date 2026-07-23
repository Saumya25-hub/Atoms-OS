#include "atoms_graph_ui.h"
#include "kernel/core/lib/include/string.h"

static void format_dec(char* buf, uint32_t val) {
    if (val == 0) {
        buf[0] = '0'; buf[1] = '\0';
        return;
    }
    char temp[16];
    int i = 0;
    while (val > 0) {
        temp[i++] = '0' + (val % 10);
        val /= 10;
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

static void format_float1(char* buf, float val) {
    if (val < 0.0f) {
        buf[0] = '-';
        format_float1(buf + 1, -val);
        return;
    }
    uint32_t int_part = (uint32_t)val;
    uint32_t dec_part = (uint32_t)((val - (float)int_part) * 10.0f);
    char s_int[16];
    format_dec(s_int, int_part);
    size_t len = strlen(s_int);
    memcpy(buf, s_int, len);
    buf[len] = '.';
    buf[len + 1] = '0' + (dec_part % 10);
    buf[len + 2] = '\0';
}

static void draw_stat_row(const BVFramebuffer* fb, int x, int y, const char* label, const char* val, uint32_t val_color) {
    BWE_DrawText(fb, label, x, y, 0xFFA6ADC8, 0);
    BWE_DrawText(fb, val, x + 120, y, val_color, 0);
}

void atoms_graph_ui_render_panel(const BVFramebuffer* fb, BWE_Rect win_bounds,
                                  const AtomsGraphMetrics* m, bool is_finished, uint32_t score) {
    if (!fb || !m) return;

    int32_t panel_w = 260;
    int32_t panel_x = win_bounds.x + win_bounds.width - panel_w;
    int32_t panel_y = win_bounds.y;
    int32_t panel_h = win_bounds.height;

    // Side panel background container
    BWE_FillRect(fb, panel_x, panel_y, panel_w, panel_h, 0xFF181825);
    BWE_FillRect(fb, panel_x, panel_y, 2, panel_h, 0xFF313244); // Left border divider

    // Header
    BWE_FillRect(fb, panel_x + 2, panel_y, panel_w - 2, 40, 0xFF1E1E2E);
    BWE_DrawText(fb, "ATOMS GRAPH 3D", panel_x + 16, panel_y + 12, 0xFF89B4FA, 0);

    const char* status_str = is_finished ? "COMPLETE" : "BENCHMARKING";
    uint32_t status_color = is_finished ? 0xFFA6E3A1 : 0xFFF9E2AF;
    BWE_DrawText(fb, status_str, panel_x + 160, panel_y + 12, status_color, 0);

    int cur_y = panel_y + 55;
    char str_buf[32];

    // LIVE METRICS Header
    BWE_DrawText(fb, "--- LIVE METRICS ---", panel_x + 16, cur_y, 0xFF585B70, 0);
    cur_y += 24;

    // Current FPS
    format_float1(str_buf, m->current_fps);
    draw_stat_row(fb, panel_x + 16, cur_y, "Current FPS:", str_buf, 0xFF89DCEB);
    cur_y += 20;

    // Average FPS
    format_float1(str_buf, m->avg_fps);
    draw_stat_row(fb, panel_x + 16, cur_y, "Average FPS:", str_buf, 0xFFA6E3A1);
    cur_y += 20;

    // Min / Max FPS
    format_float1(str_buf, m->min_fps);
    draw_stat_row(fb, panel_x + 16, cur_y, "Min FPS:", str_buf, 0xFFF38BA8);
    cur_y += 20;

    // Frame Time
    format_float1(str_buf, m->current_frame_time_ms);
    strcat(str_buf, " ms");
    draw_stat_row(fb, panel_x + 16, cur_y, "Frame Time:", str_buf, 0xFFF9E2AF);
    cur_y += 20;

    // Worst Frame Time
    format_float1(str_buf, m->worst_frame_time_ms);
    strcat(str_buf, " ms");
    draw_stat_row(fb, panel_x + 16, cur_y, "Worst FT:", str_buf, 0xFFF38BA8);
    cur_y += 24;

    // WORKLOAD Header
    BWE_DrawText(fb, "--- WORKLOAD ---", panel_x + 16, cur_y, 0xFF585B70, 0);
    cur_y += 24;

    // Triangles
    format_dec(str_buf, m->current_triangles);
    draw_stat_row(fb, panel_x + 16, cur_y, "Triangles:", str_buf, 0xFFCAD3F5);
    cur_y += 20;

    // Draw Calls
    format_dec(str_buf, m->draw_calls);
    draw_stat_row(fb, panel_x + 16, cur_y, "Draw Calls:", str_buf, 0xFFCAD3F5);
    cur_y += 20;

    // Resolution
    format_dec(str_buf, m->res_width);
    strcat(str_buf, "x");
    char s_h[16];
    format_dec(s_h, m->res_height);
    strcat(str_buf, s_h);
    draw_stat_row(fb, panel_x + 16, cur_y, "Resolution:", str_buf, 0xFF89B4FA);
    cur_y += 24;

    // SYSTEM & STAGE Header
    BWE_DrawText(fb, "--- STAGE & SYSTEM ---", panel_x + 16, cur_y, 0xFF585B70, 0);
    cur_y += 24;

    // Current Stage
    char stage_num[16];
    format_dec(stage_num, m->current_stage + 1);
    strcpy(str_buf, "Stage ");
    strcat(str_buf, stage_num);
    strcat(str_buf, "/10");
    draw_stat_row(fb, panel_x + 16, cur_y, "Stage:", str_buf, 0xFFF5C2E7);
    cur_y += 20;

    // Stage Name
    const char* stage_names[10] = {
        "Baseline Geometry", "Geometry Scaling", "Depth Complexity",
        "Texture Workload", "Multi-Object Scene", "Render-to-Texture",
        "RTT Stress", "High Geometry Stress", "Combined Stress", "Stability Run"
    };
    if (m->current_stage < 10) {
        BWE_DrawText(fb, stage_names[m->current_stage], panel_x + 16, cur_y, 0xFFB4BEFE, 0);
    }
    cur_y += 24;

    // Elapsed Time
    format_dec(str_buf, (uint32_t)(m->total_elapsed_ms / 1000));
    strcat(str_buf, " sec");
    draw_stat_row(fb, panel_x + 16, cur_y, "Elapsed:", str_buf, 0xFFCAD3F5);
    cur_y += 20;

    // Heap Memory
    format_dec(str_buf, m->mem_current_kb);
    strcat(str_buf, " KB");
    draw_stat_row(fb, panel_x + 16, cur_y, "Heap Mem:", str_buf, 0xFFA6E3A1);
    cur_y += 20;

    // Memory Delta
    if (m->mem_delta_kb >= 0) {
        strcpy(str_buf, "+");
        char d_str[16];
        format_dec(d_str, (uint32_t)m->mem_delta_kb);
        strcat(str_buf, d_str);
    } else {
        strcpy(str_buf, "-");
        char d_str[16];
        format_dec(d_str, (uint32_t)(-m->mem_delta_kb));
        strcat(str_buf, d_str);
    }
    strcat(str_buf, " KB");
    draw_stat_row(fb, panel_x + 16, cur_y, "Mem Delta:", str_buf, (m->mem_delta_kb > 64) ? 0xFFF38BA8 : 0xFFA6E3A1);
    cur_y += 30;

    // Score box
    if (is_finished) {
        BWE_FillRect(fb, panel_x + 16, cur_y, panel_w - 32, 45, 0xFF303446);
        BWE_DrawRect(fb, panel_x + 16, cur_y, panel_w - 32, 45, 0xFF89B4FA, 1);
        BWE_DrawText(fb, "SCORE:", panel_x + 28, cur_y + 14, 0xFFCAD3F5, 0);

        char score_str[16];
        format_dec(score_str, score);
        BWE_DrawText(fb, score_str, panel_x + 90, cur_y + 14, 0xFFA6E3A1, 0);
    }
}

void atoms_graph_ui_render_results_screen(const BVFramebuffer* fb, BWE_Rect win_bounds,
                                           const AtomsGraphMetrics* m, uint32_t score) {
    if (!fb || !m) return;

    int32_t view_w = win_bounds.width - 260;
    int32_t view_h = win_bounds.height;
    int32_t vx = win_bounds.x;
    int32_t vy = win_bounds.y;

    // Overlay Card Background
    BWE_FillRect(fb, vx + 20, vy + 20, view_w - 40, view_h - 40, 0xFF1E1E2E);
    BWE_DrawRect(fb, vx + 20, vy + 20, view_w - 40, view_h - 40, 0xFF45475A, 2);

    // Title Banner
    BWE_DrawText(fb, "ATOMS GRAPH 3D BENCHMARK RESULT", vx + 40, vy + 40, 0xFF89B4FA, 0);

    char score_str[32] = "OVERALL SCORE: ";
    char num_buf[16];
    format_dec(num_buf, score);
    strcat(score_str, num_buf);
    strcat(score_str, " ATOMS");
    BWE_DrawText(fb, score_str, vx + 40, vy + 68, 0xFFA6E3A1, 0);

    int cur_y = vy + 105;
    char str_buf[64];

    format_float1(str_buf, m->avg_fps);
    BWE_DrawText(fb, "Avg FPS:", vx + 40, cur_y, 0xFFA6ADC8, 0);
    BWE_DrawText(fb, str_buf, vx + 140, cur_y, 0xFF89DCEB, 0);

    format_float1(str_buf, m->min_fps);
    BWE_DrawText(fb, "Min FPS:", vx + 220, cur_y, 0xFFA6ADC8, 0);
    BWE_DrawText(fb, str_buf, vx + 310, cur_y, 0xFFF38BA8, 0);

    format_float1(str_buf, m->avg_frame_time_ms);
    strcat(str_buf, " ms");
    BWE_DrawText(fb, "Avg FT:", vx + 380, cur_y, 0xFFA6ADC8, 0);
    BWE_DrawText(fb, str_buf, vx + 460, cur_y, 0xFFF9E2AF, 0);

    cur_y += 30;

    // Stage breakdown header
    BWE_DrawText(fb, "STAGE RESULTS:", vx + 40, cur_y, 0xFF89B4FA, 0);
    cur_y += 24;

    const char* stage_names[10] = {
        "1. Baseline Geometry", "2. Geometry Scaling", "3. Depth Complexity",
        "4. Texture Workload", "5. Multi-Object Scene", "6. Render-to-Texture",
        "7. RTT Stress", "8. High Geometry Stress", "9. Combined Stress", "10. Stability Run"
    };

    for (int i = 0; i < 5; i++) {
        BWE_DrawText(fb, stage_names[i], vx + 40, cur_y, 0xFFCAD3F5, 0);
        BWE_DrawText(fb, m->stages[i].passed ? "PASS" : "FAIL", vx + 230, cur_y, m->stages[i].passed ? 0xFFA6E3A1 : 0xFFF38BA8, 0);
        format_float1(str_buf, m->stages[i].avg_fps);
        strcat(str_buf, " FPS");
        BWE_DrawText(fb, str_buf, vx + 290, cur_y, 0xFF89DCEB, 0);
        cur_y += 20;
    }

    cur_y = vy + 159;
    for (int i = 5; i < 10; i++) {
        BWE_DrawText(fb, stage_names[i], vx + 380, cur_y, 0xFFCAD3F5, 0);
        BWE_DrawText(fb, m->stages[i].passed ? "PASS" : "FAIL", vx + 570, cur_y, m->stages[i].passed ? 0xFFA6E3A1 : 0xFFF38BA8, 0);
        format_float1(str_buf, m->stages[i].avg_fps);
        strcat(str_buf, " FPS");
        BWE_DrawText(fb, str_buf, vx + 630, cur_y, 0xFF89DCEB, 0);
        cur_y += 20;
    }

    cur_y = vy + 285;
    BWE_DrawText(fb, "BOTTLENECK ANALYSIS:", vx + 40, cur_y, 0xFF89B4FA, 0);
    cur_y += 24;

    BWE_DrawText(fb, "Largest FPS Drop Stage:", vx + 40, cur_y, 0xFFA6ADC8, 0);
    if (m->largest_fps_drop_stage >= 0 && m->largest_fps_drop_stage < 10) {
        BWE_DrawText(fb, stage_names[m->largest_fps_drop_stage], vx + 240, cur_y, 0xFFF9E2AF, 0);
    }
    cur_y += 20;

    BWE_DrawText(fb, "Worst Frame-Time Spike:", vx + 40, cur_y, 0xFFA6ADC8, 0);
    format_float1(str_buf, m->worst_ft_spike_val);
    strcat(str_buf, " ms");
    BWE_DrawText(fb, str_buf, vx + 240, cur_y, 0xFFF38BA8, 0);
    cur_y += 20;

    BWE_DrawText(fb, "Peak Triangle Workload:", vx + 40, cur_y, 0xFFA6ADC8, 0);
    format_dec(str_buf, m->highest_workload_triangles);
    BWE_DrawText(fb, str_buf, vx + 240, cur_y, 0xFFB4BEFE, 0);
    cur_y += 20;

    BWE_DrawText(fb, "Memory Stability:", vx + 40, cur_y, 0xFFA6ADC8, 0);
    if (m->mem_delta_kb <= 64) {
        BWE_DrawText(fb, "EXCELLENT (No continuous growth detected)", vx + 240, cur_y, 0xFFA6E3A1, 0);
    } else {
        BWE_DrawText(fb, "WARN (Memory growth observed)", vx + 240, cur_y, 0xFFF38BA8, 0);
    }
}
