import os

repo_root = r"d:\Signatures_OS"
kernel_dir = os.path.join(repo_root, "kernel")

replacements = {
    '"../lib/': '"kernel/core/lib/',
    '"../memory/': '"kernel/core/memory/',
    '"../display/': '"kernel/drivers/display/',
    '"../bocompositor/': '"kernel/wm/compositor/',
    '"../bwe/': '"kernel/wm/bwe/',
    '"../BOSurface/': '"kernel/wm/surface/', # Actually BOSurface/Core went to wm/surface, Events went to ui/events
    '"../../lib/': '"kernel/core/lib/',
    '"../../memory/': '"kernel/core/memory/',
    '"../../display/': '"kernel/drivers/display/',
    '"../../bocompositor/': '"kernel/wm/compositor/',
    '"../../bwe/': '"kernel/wm/bwe/',
    '"../../BOSurface/': '"kernel/wm/surface/',
    '"../conhost/': '"kernel/shell/conhost/',
    '"../Events/': '"kernel/ui/events/',
    '"../../Events/': '"kernel/ui/events/',
    '"../vfs/': '"kernel/vfs/vfs_legacy/',
    '"../../vfs/': '"kernel/vfs/vfs_legacy/',
}

for root, dirs, files in os.walk(kernel_dir):
    for file in files:
        if file.endswith(('.c', '.h')):
            filepath = os.path.join(root, file)
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
            original_content = content
            for old_str, new_str in replacements.items():
                content = content.replace(old_str, new_str)
            if content != original_content:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(content)
                print(f"Fixed includes in {filepath}")
