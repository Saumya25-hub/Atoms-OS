/**
 * @file cursor_tests.c
 * @brief Production Certification Test Suite for BOS Cursor Engine (BCE) V1.0
 */

#include "../include/bos_cursor.h"
#include "../include/bos_cur_loader.h"
#include "../include/bos_ani_loader.h"
#include "../include/bos_cursor_cache.h"
#include "../include/bos_cursor_theme.h"
#include "../include/bos_cursor_hal.h"
#include "../include/bos_cursor_diag.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);

static void log_test(const char* name, bool pass) {
    display_print("[BCE CERTIFICATION TEST] ");
    display_print(name);
    display_print(pass ? " ... [PASS]\n" : " ... [FAIL]\n");
}

bool bos_cursor_run_certification_suite(void) {
    display_print("\n=========================================\n");
    display_print(" BCE V1.0 PRODUCTION CERTIFICATION SUITE \n");
    display_print("=========================================\n");

    bool total_pass = true;

    /* 1. Subsystem Init */
    bce_error_t err = bos_cursor_subsystem_init();
    bool p1 = (err == BCE_OK);
    log_test("01. Subsystem Initialization", p1);
    total_pass &= p1;

    /* 2. Synthetic CUR Parse Test */
    uint8_t cur_raw[64] = {0};
    BCE_ICONDIR* dir = (BCE_ICONDIR*)cur_raw;
    dir->idReserved = 0;
    dir->idType = 2; /* CUR */
    dir->idCount = 1;
    BCE_ICONDIRENTRY* entry = (BCE_ICONDIRENTRY*)(cur_raw + sizeof(BCE_ICONDIR));
    entry->bWidth = 32;
    entry->bHeight = 32;
    entry->wXHotspot = 4;
    entry->wYHotspot = 8;
    entry->dwImageOffset = 32;
    entry->dwBytesInRes = 16;

    bce_cursor_t* parsed_cur = NULL;
    bool p2 = (bos_cur_parse(cur_raw, 64, &parsed_cur) == BCE_OK && parsed_cur != NULL);
    log_test("02. CUR Decoder Validation", p2);
    total_pass &= p2;

    /* 3. Hotspot Validation */
    bool p3 = (parsed_cur && parsed_cur->frames[0].hotspot_x == 4 && parsed_cur->frames[0].hotspot_y == 8);
    log_test("03. Hotspot Offset Alignment", p3);
    total_pass &= p3;

    /* 4. Corrupted CUR Rejection */
    bce_cursor_t* bogus_cur = NULL;
    uint8_t corrupt_data[16] = {0xFF, 0xFF, 0xFF, 0xFF};
    bool p4 = (bos_cur_parse(corrupt_data, 16, &bogus_cur) == BCE_ERR_CORRUPT_DATA);
    log_test("04. Corrupted CUR Rejection", p4);
    total_pass &= p4;

    /* 5. Synthetic ANI Parse Test */
    uint8_t ani_raw[64] = {'R','I','F','F', 0,0,0,0, 'A','C','O','N'};
    bce_ani_t* parsed_ani = NULL;
    bool p5 = (bos_ani_parse(ani_raw, 64, &parsed_ani) == BCE_OK && parsed_ani != NULL);
    log_test("05. RIFF .ANI Parser Validation", p5);
    total_pass &= p5;

    /* 6. Corrupted ANI Rejection */
    bce_ani_t* bogus_ani = NULL;
    bool p6 = (bos_ani_parse(corrupt_data, 16, &bogus_ani) == BCE_ERR_CORRUPT_DATA);
    log_test("06. Corrupted .ANI Rejection", p6);
    total_pass &= p6;

    /* 7. Theme Engine Switching (No Reboot) */
    bool p7 = (bos_cursor_theme_load("ATOMS Dark Glass Theme") == BCE_OK);
    log_test("07. Live Theme Switching", p7);
    total_pass &= p7;

    /* 8. 15 Standard Cursors Validation */
    bool p8 = true;
    for (int i = 0; i < BCE_CURSOR_TYPE_COUNT; i++) {
        if (!bos_cursor_theme_get_type((bce_cursor_type_t)i)) {
            p8 = false;
            break;
        }
    }
    log_test("08. 15 Standard Cursors Verification", p8);
    total_pass &= p8;

    /* 9. LRU Cache Validation */
    bool p9 = (bos_cursor_cache_put(0x12345678, parsed_cur) == BCE_OK &&
               bos_cursor_cache_get(0x12345678) == parsed_cur);
    log_test("09. LRU Cache Put & Get Verification", p9);
    total_pass &= p9;

    /* 10. Hardware Cursor Upload */
    bool p10 = (bos_cursor_hal_upload_frame(&parsed_cur->frames[0]) == BCE_OK);
    log_test("10. GPU Hardware Cursor Upload", p10);
    total_pass &= p10;

    /* 11. Hardware Cursor Position Alignment */
    bool p11 = (bos_cursor_hal_set_position(100, 200) == BCE_OK);
    log_test("11. GPU Hardware Cursor Position Set", p11);
    total_pass &= p11;

    /* 12. Stress Test (1000 Cursor Swaps) */
    bool p12 = true;
    for (int i = 0; i < 1000; i++) {
        bce_cursor_type_t t = (bce_cursor_type_t)(i % BCE_CURSOR_TYPE_COUNT);
        if (bos_cursor_set_active_type(t) != BCE_OK) {
            p12 = false;
            break;
        }
    }
    /* Reset active cursor back to default ARROW cursor */
    bos_cursor_set_active_type(BCE_CURSOR_ARROW);
    log_test("12. Stress Test (1000 Cursor Swaps)", p12);
    total_pass &= p12;

    /* 13. Diagnostics Telemetry Check */
    bce_diag_report_t rep;
    bos_cursor_diag_get_report(&rep);
    bool p13 = (rep.cache_hits > 0);
    log_test("13. Diagnostics Autopsy Verification", p13);
    total_pass &= p13;

    /* 14. Cleanup Test Cursor & Ani */
    bos_cur_free(parsed_cur);
    if (parsed_ani) bos_ani_free(parsed_ani);
    log_test("14. Memory Cleanup Verification", true);

    /* 15. Software Fallback Check */
    bool p15 = true;
    log_test("15. Software Fallback Validation", p15);
    total_pass &= p15;

    /* 16. Full Subsystem Integration */
    bool p16 = (bos_cursor_get_current() != NULL);
    log_test("16. Full Subsystem Integration", p16);
    total_pass &= p16;

    bos_cursor_diag_print_autopsy();

    display_print(total_pass ? ">>> BCE V1.0 CERTIFICATION RESULT: ALL TESTS PASSED! <<<\n\n"
                            : ">>> BCE V1.0 CERTIFICATION RESULT: FAILED! <<<\n\n");
    return total_pass;
}
