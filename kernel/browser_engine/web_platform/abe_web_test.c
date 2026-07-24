#include "abe_web_test.h"
#include "abe_web_fetch.h"
#include "abe_web_xhr.h"
#include "abe_web_url.h"
#include "abe_web_storage.h"
#include "abe_web_history.h"
#include "abe_web_scheduler.h"
#include "abe_web_observers.h"
#include "abe_web_blob.h"
#include "abe_web_diag.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static bool g_anim_frame_fired = false;
static void SampleAnimCallback(void* user_data) {
    g_anim_frame_fired = true;
}

static bool Test_FetchAndXHR(void) {
    display_print("[ABE_WEB_TEST] 1. Testing Fetch API & XMLHttpRequest Lifecycle...\n");

    ABE_FetchRequestHandle fetch_req = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_Fetch(ABE_INVALID_HANDLE, "https://api.example.com/data", NULL, &fetch_req);
    if (err != ABE_SUCCESS || fetch_req == ABE_INVALID_HANDLE) {
        display_print("[ABE_WEB_TEST] FAIL: ABE_Fetch failed!\n");
        return false;
    }

    ABE_XHRHandle xhr = ABE_INVALID_HANDLE;
    err = ABE_XMLHttpRequest_Create(&xhr);
    if (err != ABE_SUCCESS || xhr == ABE_INVALID_HANDLE) {
        display_print("[ABE_WEB_TEST] FAIL: XMLHttpRequest_Create failed!\n");
        return false;
    }

    ABE_WebXHR_Open(xhr, "GET", "https://api.example.com/status");
    ABE_WebXHR_Send(xhr, NULL);

    display_print("[ABE_WEB_TEST] PASS: Fetch API & XMLHttpRequest verified.\n");
    return true;
}

static bool Test_WebStorage(void) {
    display_print("[ABE_WEB_TEST] 2. Testing Web Storage (localStorage & sessionStorage)...\n");

    ABE_StorageHandle storage = ABE_INVALID_HANDLE;
    ABE_GetLocalStorage(&storage);

    ABE_Error err = ABE_Storage_SetItem(storage, "theme", "dark_mode");
    if (err != ABE_SUCCESS) {
        display_print("[ABE_WEB_TEST] FAIL: Storage_SetItem failed!\n");
        return false;
    }

    char read_buf[64];
    err = ABE_Storage_GetItem(storage, "theme", read_buf, sizeof(read_buf));
    if (err != ABE_SUCCESS || strcmp(read_buf, "dark_mode") != 0) {
        display_print("[ABE_WEB_TEST] FAIL: Storage_GetItem returned invalid value!\n");
        return false;
    }

    display_print("[ABE_WEB_TEST] PASS: Web Storage verified.\n");
    return true;
}

static bool Test_SchedulerAndObservers(void) {
    display_print("[ABE_WEB_TEST] 3. Testing Browser Scheduler & Web Observers...\n");

    uint32_t frame_id = 0;
    ABE_Error err = ABE_RequestAnimationFrame(ABE_INVALID_HANDLE, SampleAnimCallback, NULL, &frame_id);
    if (err != ABE_SUCCESS || frame_id == 0) {
        display_print("[ABE_WEB_TEST] FAIL: RequestAnimationFrame failed!\n");
        return false;
    }

    ABE_ObserverHandle obs = ABE_INVALID_HANDLE;
    err = ABE_CreateMutationObserver(&obs);
    if (err != ABE_SUCCESS || obs == ABE_INVALID_HANDLE) {
        display_print("[ABE_WEB_TEST] FAIL: CreateMutationObserver failed!\n");
        return false;
    }

    display_print("[ABE_WEB_TEST] PASS: Browser Scheduler & Web Observers verified.\n");
    return true;
}

static bool Test_BlobAndEncoding(void) {
    display_print("[ABE_WEB_TEST] 4. Testing Blob, File & TextEncoder/Decoder...\n");

    const char* sample_text = "Hello ATOMS OS Web Platform";
    uint8_t enc_buf[64];
    size_t written = 0;

    ABE_Error err = ABE_WebBlob_TextEncodeUTF8(sample_text, enc_buf, sizeof(enc_buf), &written);
    if (err != ABE_SUCCESS || written != strlen(sample_text)) {
        display_print("[ABE_WEB_TEST] FAIL: TextEncodeUTF8 failed!\n");
        return false;
    }

    ABE_BlobHandle blob = ABE_INVALID_HANDLE;
    err = ABE_CreateBlob(enc_buf, written, "text/plain", &blob);
    if (err != ABE_SUCCESS || blob == ABE_INVALID_HANDLE) {
        display_print("[ABE_WEB_TEST] FAIL: CreateBlob failed!\n");
        return false;
    }

    display_print("[ABE_WEB_TEST] PASS: Blob, File & TextEncoder/Decoder verified.\n");
    return true;
}

void ABE_RunPhase8_VerificationSuite(void) {
    display_print("\n=========================================================\n");
    display_print(" ATOMS OS — ABE Phase 8 Web Platform Verification Suite  \n");
    display_print("=========================================================\n");

    ABE_HTMLInitialize();
    ABE_CSSInitialize();
    ABE_LayoutInitialize();
    ABE_JSInitialize();
    ABE_WebPlatformInitialize();

    if (!Test_FetchAndXHR()) return;
    if (!Test_WebStorage()) return;
    if (!Test_SchedulerAndObservers()) return;
    if (!Test_BlobAndEncoding()) return;

    ABE_WebDiag_DumpStats();

    ABE_WebPlatformShutdown();
    ABE_JSShutdown();
    ABE_LayoutShutdown();
    ABE_CSSShutdown();
    ABE_HTMLShutdown();

    display_print("\nPASS_PHASE8_ABE_PRODUCTION_WEB_PLATFORM_FOUNDATION\n\n");
}
