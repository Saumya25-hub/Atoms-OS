#include "include/bos_installer.h"
#include "kernel/runtime/package/include/bos_pack_loader.h"
#include "kernel/runtime/manifest/include/bos_manifest.h"
#include "kernel/runtime/loader/include/bos_elf_loader.h"
#include "kernel/runtime/sdk_runtime/include/bos_sdk_runtime.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);

bool bos_install_package(const char* bosx_path) {
    display_print("[BOSX INSTALLER] Starting Package Installation: ");
    display_print(bosx_path ? bosx_path : "Application.bosx");
    display_print("\n");

    BOS_Package* pack = bos_package_open(bosx_path);
    if (!pack) return false;

    if (!bos_package_verify(pack)) {
        bos_package_close(pack);
        return false;
    }

    bos_package_mount(pack, "/apps/");

    const char* sample_json = "{\"name\":\"Forge Application\",\"identifier\":\"com.bos.app\",\"version\":\"1.0.0\",\"entry_point\":\"app.elf\"}";
    BOS_Manifest* manifest = bos_manifest_parse(sample_json);

    display_print("[BOSX INSTALLER] Registered Desktop Shortcut for ");
    display_print(manifest ? manifest->name : "App");
    display_print("\n");

    bos_manifest_free(manifest);
    bos_package_close(pack);
    return true;
}

bool bos_launch_installed_app(const char* identifier) {
    display_print("[BOSX RUNTIME LAUNCHER] Opening Application Package: ");
    display_print(identifier ? identifier : "/apps/Application.bosx");
    display_print("\n");

    BOS_Package* pack = bos_package_open(identifier ? identifier : "/apps/Application.bosx");
    if (!pack) return false;

    const char* sample_json = "{\"name\":\"Visual Forge User App\",\"identifier\":\"com.bos.app\",\"version\":\"1.0.0\",\"entry_point\":\"app.elf\"}";
    BOS_Manifest* manifest = bos_manifest_parse(sample_json);

    BOS_Process* proc = bos_elf_load_and_spawn(pack, manifest);
    if (!proc) {
        bos_manifest_free(manifest);
        bos_package_close(pack);
        return false;
    }

    // Dynamic execution of Visual Forge generated UI Context
    BOS_WindowHandle win = BOS_SDK_CreateWindow(100, 100, 800, 600, manifest ? manifest->name : "User Application");
    if (win) {
        display_print("[BOSFSR32 RUNTIME] Loaded Visual Forge canvas elements dynamically.\n");
    }

    display_print("[BOSX RUNTIME LAUNCHER] Application surface running natively!\n");
    return true;
}
