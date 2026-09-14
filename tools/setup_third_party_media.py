import os
import shutil

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MEDIA_DIR = os.path.join(BASE_DIR, "third_party", "media")
H264_INC = os.path.join(MEDIA_DIR, "h264", "include")
H264_SRC = os.path.join(MEDIA_DIR, "h264", "src")
MP4_INC = os.path.join(MEDIA_DIR, "mp4", "include")
MP4_SRC = os.path.join(MEDIA_DIR, "mp4", "src")

os.makedirs(H264_INC, exist_ok=True)
os.makedirs(H264_SRC, exist_ok=True)
os.makedirs(MP4_INC, exist_ok=True)
os.makedirs(MP4_SRC, exist_ok=True)

# 1. Copy h264bsd files from cloned repo
src_repo = os.path.join(BASE_DIR, "build", "h264bsd_repo", "src")
for f in os.listdir(src_repo):
    src_file = os.path.join(src_repo, f)
    if os.path.isfile(src_file):
        if f.endswith(".h"):
            dest_file = os.path.join(H264_INC, f)
        else:
            dest_file = os.path.join(H264_SRC, f)
        shutil.copy2(src_file, dest_file)

# 2. Patch h264bsd_cfg.h for freestanding
cfg_path = os.path.join(H264_INC, "h264bsd_cfg.h")
with open(cfg_path, "r", encoding="utf-8", errors="ignore") as f:
    content = f.read()
# Replace <stdlib.h> and <memory.h>
content = content.replace("#include <stdlib.h>", "/* #include <stdlib.h> */")
content = content.replace("#include <memory.h>", '#include "kernel/core/lib/include/string.h"')
with open(cfg_path, "w", encoding="utf-8") as f:
    f.write(content)

# 3. Patch h264bsd_util.h for freestanding and bospectra memory
util_h_path = os.path.join(H264_INC, "h264bsd_util.h")
with open(util_h_path, "r", encoding="utf-8", errors="ignore") as f:
    content = f.read()

# Replace <assert.h> and <stdio.h>
content = content.replace("#include <assert.h>", "/* #include <assert.h> */")
content = content.replace("#include <stdio.h>", "/* #include <stdio.h> */")

# Replace ALLOCATE and FREE macros and add malloc/free wrappers
old_allocate = """#define ALLOCATE(ptr, count, type) \\
{ \\
    (ptr) = malloc((count) * sizeof(type)); \\
}"""
new_allocate = """#include "kernel/media/bospectra/memory/bospectra_memory.h"
#define malloc(sz) bospectra_mem_alloc((sz), "H264Decoder")
#define free(p) bospectra_mem_free((void*)(p))
#define ALLOCATE(ptr, count, type) \\
{ \\
    (ptr) = (type*)bospectra_mem_alloc((count) * sizeof(type), "H264Decoder"); \\
}"""

old_free = """#define FREE(ptr) \\
{ \\
    free((ptr)); (ptr) = NULL; \\
}"""
new_free = """#define FREE(ptr) \\
{ \\
    if (ptr) { bospectra_mem_free((void*)(ptr)); (ptr) = NULL; } \\
}"""

content = content.replace(old_allocate, new_allocate)
content = content.replace(old_free, new_free)

with open(util_h_path, "w", encoding="utf-8") as f:
    f.write(content)

# 4. Patch h264bsd_decoder.c for malloc/free in RGB conversion buffer
dec_c_path = os.path.join(H264_SRC, "h264bsd_decoder.c")
with open(dec_c_path, "r", encoding="utf-8", errors="ignore") as f:
    content = f.read()
content = content.replace("free(pStorage->conversionBuffer)", "bospectra_mem_free(pStorage->conversionBuffer)")
content = content.replace("(u32*)malloc(rgbSize)", "(u32*)bospectra_mem_alloc(rgbSize, \"H264RGB\")")
with open(dec_c_path, "w", encoding="utf-8") as f:
    f.write(content)

# 5. Create third_party/media/LICENSE
license_path = os.path.join(MEDIA_DIR, "LICENSE")
with open(license_path, "w", encoding="utf-8") as f:
    f.write("""ATOMS OS — Third-Party Media Subsystem Licenses
=====================================================

1. h264bsd (isolated under third_party/media/h264/)
   Source: Android Open Source Project (AOSP) Stagefright
   License: Apache License, Version 2.0
   Zero GPL/LGPL code.

2. MP4 Demuxer (isolated under third_party/media/mp4/)
   License: Creative Commons Zero 1.0 Universal (CC0-1.0) / Public Domain
   Zero GPL/LGPL code.
""")

print("third_party/media successfully configured!")
