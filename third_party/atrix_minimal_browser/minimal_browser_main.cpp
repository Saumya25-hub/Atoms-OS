/*
 * ATOMS OS / ATRIX — Minimal Real-Web Browser Probe
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Standalone Executable Main (minimal_real_browser.elf)
 */

#include "minimal_browser.h"
#include "isolation_tests.h"
#include <stdio.h>

extern "C" int main(int argc, char** argv) {
    printf("[MINBROW] Entry point reached\n");
    printf("[MINBROW] CRT initialized\n");
    printf("[MINBROW] main() entered\n");

    const char* target_url = "https://www.google.com/";
    if (argc > 1 && argv[1] && argv[1][0] != '\0') {
        target_url = argv[1];
    }

    // Launch Minimal Browser Probe Instance
    atrix::MinimalBrowser browser(1024, 640);
    if (!browser.Initialize()) {
        printf("[MINBROW][FATAL] Failed to initialize minimal browser.\n");
        return 1;
    }

    // Submit first frame so window is immediately visible on desktop
    browser.SubmitFirstFrame();

    // Navigation starting
    printf("[MINBROW] Navigation starting: %s\n", target_url);
    browser.Navigate(target_url);

    // Enter interactive desktop event loop to keep window rendered
    browser.RunEventLoop();

    return 0;
}
