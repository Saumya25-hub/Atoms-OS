/*
 * =====================================================================
 * ATOMS OS — FULL CHROMIUM DESKTOP APPLICATION ENTRY POINT
 * =====================================================================
 * Standalone Ring 3 Executable (chromium_browser.elf)
 * Powered by Google Chromium (net/, base/, mojo/) + Blink + Skia + APAL
 * Copyright (C) 2026 ATOMS OS Project / Chromium Authors
 * =====================================================================
 */

#include "../include/browser_app.h"
#include "../include/chromium_window.h"
#include "atoms/userspace/apal/include/apal.h"

// Genuine Upstream Chromium Base
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/version.h"

// Genuine Upstream Chromium Mojo
#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/system/message_pipe.h"

extern "C" {
    int printf(const char* format, ...);
    int strcmp(const char* s1, const char* s2);
}

int chromium_browser_main(int argc, char** argv) {
    // 0. Initialize Genuine Upstream Chromium Base Infrastructure
    base::AtExitManager exit_manager;
    base::CommandLine::Init(argc, argv);
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    (void)command_line;

    base::Version chromium_version("130.0.6723.0");

    printf("[CHROMIUM] Starting Google Chromium Desktop Browser on ATOMS OS...\n");
    printf("[CHROMIUM] CPL=3 Ring 3 Isolated User Mode Active\n");
    printf("[CHROMIUM_BASE] REAL UPSTREAM CHROMIUM BASE ACTIVE: AtExitManager=OK, CommandLine=OK, Version=%s\n",
           chromium_version.IsValid() ? chromium_version.GetString().c_str() : "INVALID");

    // Phase 2A: Initialize Genuine Upstream Chromium Mojo Core
    mojo::core::Init();
    mojo::ScopedMessagePipeHandle pipe0, pipe1;
    MojoResult pr = mojo::CreateMessagePipe(nullptr, &pipe0, &pipe1);

    const std::string mojo_payload = "ATOMS_MOJO_UPSTREAM_CORE_VERIFIED";
    MojoResult wr = mojo::WriteMessageRaw(
        pipe0.get(), mojo_payload.data(), mojo_payload.size(), nullptr, 0, MOJO_WRITE_MESSAGE_FLAG_NONE);

    std::vector<uint8_t> read_bytes;
    MojoResult rr = mojo::ReadMessageRaw(
        pipe1.get(), &read_bytes, nullptr, MOJO_READ_MESSAGE_FLAG_NONE);
    std::string read_str(read_bytes.begin(), read_bytes.end());

    if (pr == MOJO_RESULT_OK && wr == MOJO_RESULT_OK && rr == MOJO_RESULT_OK && read_str == mojo_payload) {
        printf("[CHROMIUM_MOJO] REAL UPSTREAM MOJO CORE ACTIVE: Init=OK, Pipe=OK, Echo=%s\n", read_str.c_str());
    } else {
        printf("[CHROMIUM_MOJO] ERROR: Mojo verification failed: pr=%d wr=%d rr=%d echo=%s\n",
               pr, wr, rr, read_str.c_str());
    }

    const char* target_url = "https://www.google.com/";
    if (argc > 1 && argv[1] && argv[1][0] != '\0') {
        target_url = argv[1];
    }

    // 1. Initialize APAL (Platform Adaptation Layer)
    apal_init();

    // 2. Create Native Chromium Desktop Window (1200x800)
    chromium::ChromiumWindow window(1200, 800);
    if (!window.Create()) {
        printf("[CHROMIUM][FATAL] Failed to create native Chromium desktop window!\n");
        return 1;
    }

    printf("[CHROMIUM] Native Window created successfully (ID: %u)\n", window.GetWindowId());
    window.SetURL(target_url);
    window.SetPageTitle("Google - Google Chrome");
    window.Present();

    // 3. Enter Interactive Event Loop
    bool running = true;
    apal_input_event_t event;

    while (running) {
        if (apal_input_poll_event(window.GetWindowId(), &event) == APAL_OK) {
            switch (event.type) {
                case APAL_EVENT_WINDOW_CLOSE:
                    running = false;
                    break;

                case APAL_EVENT_MOUSE_DOWN:
                    window.HandleMouseClick(event.mouse_x, event.mouse_y, event.mouse_button);
                    break;

                case APAL_EVENT_KEY_DOWN:
                    if (event.key_code == 27) { // ESC key closes
                        running = false;
                    } else {
                        window.HandleKeyInput(event.key_code, (char)event.char_code, event.modifiers);
                    }
                    break;

                default:
                    break;
            }
        }

        apal_thread_yield();
    }

    printf("[CHROMIUM] Chromium Browser shutting down cleanly.\n");
    window.Destroy();
    apal_shutdown();
    return 0;
}

extern "C" int main(int argc, char** argv) {
    return chromium_browser_main(argc, argv);
}
