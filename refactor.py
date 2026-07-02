import os
import shutil

repo_root = r"d:\Signatures_OS"
kernel_dir = os.path.join(repo_root, "kernel")

# 1. Directory Mapping (Old Path -> New Path) relative to kernel/
# We will use this to move folders.
dir_map = {
    # Core
    "boot": r"core\boot",
    "config": r"core\config",
    "core": r"core\core_legacy",
    "interrupt": r"core\interrupt",
    "lib": r"core\lib",
    "loader": r"core\loader",
    "memory": r"core\memory",
    "process": r"core\process",
    "scheduler": r"core\scheduler",
    "syscall": r"core\syscall",
    "timer": r"core\timer",

    # Drivers
    "driver": r"drivers\storage_legacy",
    "input": r"drivers\input",
    "keyboard": r"drivers\keyboard",
    "display": r"drivers\display",
    
    # WM
    r"BOSurface\Core": r"wm\surface",
    "bocompositor": r"wm\compositor",
    r"bwe\src": r"wm\bwe\src",
    r"bwe\include": r"wm\bwe\include",
    r"bwe\renderer": r"wm\bwe\renderer",
    r"bwe\theme": r"wm\bwe\theme",

    # UI
    r"bwe\controls": r"ui\controls",
    r"BOSurface\Events": r"ui\events",
    "boasset": r"ui\boasset",
    "bofont": r"ui\bofont",
    "boimage": r"ui\boimage",

    # VFS
    "fs": r"vfs\fs",
    "storage": r"vfs\storage",
    "vfs": r"vfs\vfs_legacy",

    # Shell
    "shell": r"shell\desktop_shell",
    "rook": r"shell\rook",
    "conhost": r"shell\conhost",
    "console": r"shell\console",
    r"BOSurface\Apps": r"shell\apps",
}

# 2. String Replacements for #includes and build.ps1
# We must replace old substrings with new substrings.
# Always replace forward slashes in code, and backslashes in build.ps1.
string_replacements = {
    # build.ps1 paths (backslash)
    "kernel\\boot\\": "kernel\\core\\boot\\",
    "kernel\\config\\": "kernel\\core\\config\\",
    "kernel\\core\\": "kernel\\core\\core_legacy\\",
    "kernel\\interrupt\\": "kernel\\core\\interrupt\\",
    "kernel\\lib\\": "kernel\\core\\lib\\",
    "kernel\\loader\\": "kernel\\core\\loader\\",
    "kernel\\memory\\": "kernel\\core\\memory\\",
    "kernel\\process\\": "kernel\\core\\process\\",
    "kernel\\scheduler\\": "kernel\\core\\scheduler\\",
    "kernel\\syscall\\": "kernel\\core\\syscall\\",
    "kernel\\timer\\": "kernel\\core\\timer\\",

    "kernel\\driver\\": "kernel\\drivers\\storage_legacy\\",
    "kernel\\input\\": "kernel\\drivers\\input\\",
    "kernel\\keyboard\\": "kernel\\drivers\\keyboard\\",
    "kernel\\display\\": "kernel\\drivers\\display\\",

    "kernel\\BOSurface\\Core\\": "kernel\\wm\\surface\\",
    "kernel\\bocompositor\\": "kernel\\wm\\compositor\\",
    "kernel\\bwe\\src\\": "kernel\\wm\\bwe\\src\\",
    "kernel\\bwe\\include\\": "kernel\\wm\\bwe\\include\\",
    "kernel\\bwe\\renderer\\": "kernel\\wm\\bwe\\renderer\\",
    "kernel\\bwe\\theme\\": "kernel\\wm\\bwe\\theme\\",

    "kernel\\bwe\\controls\\": "kernel\\ui\\controls\\",
    "kernel\\BOSurface\\Events\\": "kernel\\ui\\events\\",
    "kernel\\boasset\\": "kernel\\ui\\boasset\\",
    "kernel\\bofont\\": "kernel\\ui\\bofont\\",
    "kernel\\boimage\\": "kernel\\ui\\boimage\\",

    "kernel\\fs\\": "kernel\\vfs\\fs\\",
    "kernel\\storage\\": "kernel\\vfs\\storage\\",
    "kernel\\vfs\\": "kernel\\vfs\\vfs_legacy\\",

    "kernel\\shell\\": "kernel\\shell\\desktop_shell\\",
    "kernel\\rook\\": "kernel\\shell\\rook\\",
    "kernel\\conhost\\": "kernel\\shell\\conhost\\",
    "kernel\\console\\": "kernel\\shell\\console\\",
    "kernel\\BOSurface\\Apps\\": "kernel\\shell\\apps\\",

    # include paths (forward slash)
    "kernel/boot/": "kernel/core/boot/",
    "kernel/config/": "kernel/core/config/",
    "kernel/core/": "kernel/core/core_legacy/",
    "kernel/interrupt/": "kernel/core/interrupt/",
    "kernel/lib/": "kernel/core/lib/",
    "kernel/loader/": "kernel/core/loader/",
    "kernel/memory/": "kernel/core/memory/",
    "kernel/process/": "kernel/core/process/",
    "kernel/scheduler/": "kernel/core/scheduler/",
    "kernel/syscall/": "kernel/core/syscall/",
    "kernel/timer/": "kernel/core/timer/",

    "kernel/driver/": "kernel/drivers/storage_legacy/",
    "kernel/input/": "kernel/drivers/input/",
    "kernel/keyboard/": "kernel/drivers/keyboard/",
    "kernel/display/": "kernel/drivers/display/",

    "kernel/BOSurface/Core/": "kernel/wm/surface/",
    "kernel/bocompositor/": "kernel/wm/compositor/",
    "kernel/bwe/src/": "kernel/wm/bwe/src/",
    "kernel/bwe/include/": "kernel/wm/bwe/include/",
    "kernel/bwe/renderer/": "kernel/wm/bwe/renderer/",
    "kernel/bwe/theme/": "kernel/wm/bwe/theme/",

    "kernel/bwe/controls/": "kernel/ui/controls/",
    "kernel/BOSurface/Events/": "kernel/ui/events/",
    "kernel/boasset/": "kernel/ui/boasset/",
    "kernel/bofont/": "kernel/ui/bofont/",
    "kernel/boimage/": "kernel/ui/boimage/",

    "kernel/fs/": "kernel/vfs/fs/",
    "kernel/storage/": "kernel/vfs/storage/",
    "kernel/vfs/": "kernel/vfs/vfs_legacy/",

    "kernel/shell/": "kernel/shell/desktop_shell/",
    "kernel/rook/": "kernel/shell/rook/",
    "kernel/conhost/": "kernel/shell/conhost/",
    "kernel/console/": "kernel/shell/console/",
    "kernel/BOSurface/Apps/": "kernel/shell/apps/",

    # Also resolve problematic relative paths that will break
    '"../Apps/': '"kernel/shell/apps/',
    '"../Events/': '"kernel/ui/events/',
    '"../Apps/terminal.h"': '"kernel/shell/apps/terminal.h"',
    '"../Apps/explorer.h"': '"kernel/shell/apps/explorer.h"',
    '"../Apps/text_viewer.h"': '"kernel/shell/apps/text_viewer.h"',
}

def move_directories():
    for src, dest in dir_map.items():
        src_path = os.path.join(kernel_dir, src)
        dest_path = os.path.join(kernel_dir, dest)
        if os.path.exists(src_path):
            print(f"Moving {src_path} -> {dest_path}")
            os.makedirs(os.path.dirname(dest_path), exist_ok=True)
            if dest_path.startswith(src_path):
                # If moving into a subdirectory of itself (e.g. core -> core/core_legacy)
                # First move it to a temp name
                temp_path = src_path + "_temp_move"
                os.rename(src_path, temp_path)
                shutil.move(temp_path, dest_path)
            else:
                shutil.move(src_path, dest_path)

def update_file_contents(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except UnicodeDecodeError:
        return

    original_content = content
    for old_str, new_str in string_replacements.items():
        content = content.replace(old_str, new_str)

    if content != original_content:
        print(f"Updated {filepath}")
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)

def update_all_files():
    for root, dirs, files in os.walk(repo_root):
        if '.git' in root or 'build' in root and 'build.ps1' not in root:
            continue
        for file in files:
            if file.endswith(('.c', '.h', '.asm', '.ps1')):
                update_file_contents(os.path.join(root, file))

if __name__ == "__main__":
    print("Step 1: Moving Directories...")
    move_directories()
    print("Step 2: Updating Source Files & build.ps1...")
    update_all_files()
    print("Refactoring Complete!")
