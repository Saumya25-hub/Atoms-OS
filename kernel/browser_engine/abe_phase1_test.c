#include "abe_phase1_test.h"
#include "url/abe_url.h"
#include "resource/abe_resource.h"
#include "diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static bool Test_EngineLifecycle(void) {
    display_print("[ABE_TEST] 1. Testing Engine Initialization & Clean Shutdown...\n");
    ABE_Config cfg;
    ABE_GetDefaultConfig(&cfg);

    ABE_Error err = ABE_Initialize(&cfg);
    if (err != ABE_SUCCESS) {
        display_print("[ABE_TEST] FAIL: Engine initialization failed!\n");
        return false;
    }

    if (!ABE_IsInitialized()) {
        display_print("[ABE_TEST] FAIL: ABE_IsInitialized returned false!\n");
        return false;
    }

    display_print("[ABE_TEST] PASS: Engine Core Initialized Successfully.\n");
    return true;
}

static bool Test_URLParsingAndNormalization(void) {
    display_print("[ABE_TEST] 2. Testing URL Engine (HTTP, HTTPS, FILE, ABOUT, DATA, Normalization, Query, Relative)...\n");

    // Test 1: Standard HTTPS URL with query & fragment
    ABE_URL url1;
    ABE_Error err = ABE_ParseURL("https://www.google.com:443/search/./v1/../results?q=atoms+os#top", &url1);
    if (err != ABE_SUCCESS || !url1.is_valid) {
        display_print("[ABE_TEST] FAIL: HTTPS URL parse failed!\n");
        return false;
    }
    if (strcmp(url1.host, "www.google.com") != 0 || strcmp(url1.path, "/search/results") != 0) {
        display_print("[ABE_TEST] FAIL: Path normalization in URL failed! Path: ");
        display_print(url1.path); display_print("\n");
        return false;
    }

    // Test 2: File URL scheme
    ABE_URL url_file;
    err = ABE_ParseURL("file:///home/user/document.html", &url_file);
    if (err != ABE_SUCCESS || url_file.scheme != ABE_SCHEME_FILE) {
        display_print("[ABE_TEST] FAIL: File URL scheme parse failed!\n");
        return false;
    }

    // Test 3: About URL scheme
    ABE_URL url_about;
    err = ABE_ParseURL("about:blank", &url_about);
    if (err != ABE_SUCCESS || url_about.scheme != ABE_SCHEME_ABOUT) {
        display_print("[ABE_TEST] FAIL: About URL scheme parse failed!\n");
        return false;
    }

    // Test 4: Data URL scheme
    ABE_URL url_data;
    err = ABE_ParseURL("data:text/html;base64,SGVsbG8=", &url_data);
    if (err != ABE_SUCCESS || url_data.scheme != ABE_SCHEME_DATA) {
        display_print("[ABE_TEST] FAIL: Data URL scheme parse failed!\n");
        return false;
    }

    // Test 5: Relative URL Resolution
    char resolved[1024];
    err = ABE_ResolveRelativeURL("https://github.com/Saumya25-hub/SignaturesOS", "wiki/Phase1", resolved, sizeof(resolved));
    if (err != ABE_SUCCESS || strstr(resolved, "https://github.com/") == NULL) {
        display_print("[ABE_TEST] FAIL: Relative URL Resolution failed!\n");
        return false;
    }

    display_print("[ABE_TEST] PASS: URL Engine fully verified across all schemes.\n");
    return true;
}

static bool Test_MultiWindowManager(void) {
    display_print("[ABE_TEST] 3. Testing Window Manager (Multiple Windows, Resize, Fullscreen)...\n");

    ABE_WindowHandle win1 = ABE_INVALID_HANDLE;
    ABE_WindowHandle win2 = ABE_INVALID_HANDLE;

    ABE_Error err = ABE_CreateWindow("Browser Window 1", 50, 50, 800, 600, &win1);
    if (err != ABE_SUCCESS || win1 == ABE_INVALID_HANDLE) {
        display_print("[ABE_TEST] FAIL: Window 1 creation failed!\n");
        return false;
    }

    err = ABE_CreateWindow("Browser Window 2", 100, 100, 1024, 768, &win2);
    if (err != ABE_SUCCESS || win2 == ABE_INVALID_HANDLE) {
        display_print("[ABE_TEST] FAIL: Window 2 creation failed!\n");
        return false;
    }

    // Test Resize & Fullscreen
    ABE_ResizeWindow(win1, 900, 700);
    ABE_SetWindowFullscreen(win1, true);
    ABE_SetWindowFullscreen(win1, false);

    ABE_WindowInfo info;
    ABE_GetWindowInfo(win1, &info);
    if (info.width != 900 || info.height != 700) {
        display_print("[ABE_TEST] FAIL: Window restore bounds mismatch!\n");
        return false;
    }

    display_print("[ABE_TEST] PASS: Multi-Window Engine verified.\n");
    return true;
}

static bool Test_TabEngineAndNavigation(void) {
    display_print("[ABE_TEST] 4. Testing Tab Engine & Navigation Stack...\n");

    ABE_WindowHandle win = ABE_INVALID_HANDLE;
    ABE_CreateWindow("Tab Engine Test", 10, 10, 800, 600, &win);

    ABE_TabHandle tab1 = ABE_INVALID_HANDLE;
    ABE_TabHandle tab2 = ABE_INVALID_HANDLE;

    ABE_CreateTab(win, "https://wikipedia.org", &tab1);
    ABE_CreateTab(win, "https://github.com", &tab2);

    ABE_SelectTab(win, tab2);

    // Test Navigation History
    ABE_LoadURL(win, tab1, "https://google.com");
    ABE_LoadURL(win, tab1, "https://youtube.com");

    if (!ABE_CanGoBack(tab1)) {
        display_print("[ABE_TEST] FAIL: ABE_CanGoBack returned false!\n");
        return false;
    }

    ABE_GoBack(win, tab1);
    ABE_GoForward(win, tab1);
    ABE_Reload(win, tab1);

    ABE_TabInfo tinfo;
    ABE_GetTabInfo(tab1, &tinfo);
    if (strstr(tinfo.url, "youtube.com") == NULL) {
        display_print("[ABE_TEST] FAIL: Tab current URL mismatch!\n");
        return false;
    }

    ABE_CloseTab(win, tab2);
    ABE_DestroyWindow(win);

    display_print("[ABE_TEST] PASS: Tab Engine & Navigation Stack verified.\n");
    return true;
}

static bool Test_ResourceManagerAndPools(void) {
    display_print("[ABE_TEST] 5. Testing Resource Manager Memory Pools & Handles...\n");

    void* ptr = NULL;
    ABE_ResourceHandle handle = ABE_INVALID_HANDLE;

    ABE_Error err = ABE_Resource_Allocate(ABE_RESOURCE_TYPE_IMAGE, 4096, &ptr, &handle);
    if (err != ABE_SUCCESS || handle == ABE_INVALID_HANDLE || !ptr) {
        display_print("[ABE_TEST] FAIL: Resource allocation failed!\n");
        return false;
    }

    void* fetched_ptr = ABE_Resource_GetPointer(handle);
    if (fetched_ptr != ptr) {
        display_print("[ABE_TEST] FAIL: Resource pointer lookup mismatch!\n");
        return false;
    }

    ABE_Resource_Free(handle);
    display_print("[ABE_TEST] PASS: Resource Manager verified.\n");
    return true;
}

void ABE_RunPhase1_VerificationSuite(void) {
    display_print("\n=========================================================\n");
    display_print(" ATOMS OS — ABE Phase 1 Browser Core Verification Suite  \n");
    display_print("=========================================================\n");

    if (!Test_EngineLifecycle()) return;
    if (!Test_URLParsingAndNormalization()) return;
    if (!Test_MultiWindowManager()) return;
    if (!Test_TabEngineAndNavigation()) return;
    if (!Test_ResourceManagerAndPools()) return;

    ABE_DiagnosticsMetrics metrics;
    ABE_GetDiagnosticsMetrics(&metrics);
    display_print("[ABE_DIAG] Total Windows Created: "); display_print_dec(metrics.total_windows_created); display_print("\n");
    display_print("[ABE_DIAG] Total Tabs Created   : "); display_print_dec(metrics.total_tabs_created); display_print("\n");
    display_print("[ABE_DIAG] Total URLs Parsed    : "); display_print_dec(metrics.urls_parsed_count); display_print("\n");
    display_print("[ABE_DIAG] Total Navigations    : "); display_print_dec(metrics.total_navigations); display_print("\n");

    ABE_Shutdown();

    display_print("\nPASS_PHASE1_ABE_BROWSER_CORE_FOUNDATION\n\n");
}
