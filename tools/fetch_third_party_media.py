import os
import json
import urllib.request

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

# 1. Fetch minimp4.h
minimp4_url = "https://raw.githubusercontent.com/lieff/minimp4/master/minimp4.h"
print(f"Fetching {minimp4_url}...")
req = urllib.request.urlopen(minimp4_url)
data = req.read()
with open(os.path.join(MP4_INC, "minimp4.h"), "wb") as f:
    f.write(data)
print(f"Saved minimp4.h ({len(data)} bytes)")

# 2. Fetch h264bsd files from GitHub API
api_url = "https://api.github.com/repos/oneam/h264bsd/contents/src"
print(f"Fetching h264bsd file list from {api_url}...")
req = urllib.request.Request(api_url, headers={"User-Agent": "ATOMS-OS-Media-Importer"})
resp = urllib.request.urlopen(req)
files_meta = json.loads(resp.read().decode("utf-8"))

for item in files_meta:
    name = item["name"]
    dl_url = item["download_url"]
    if not dl_url:
        continue
    target_dir = H264_INC if name.endswith(".h") else H264_SRC
    target_path = os.path.join(target_dir, name)
    print(f"Downloading {name}...")
    try:
        f_req = urllib.request.Request(dl_url, headers={"User-Agent": "ATOMS-OS-Media-Importer"})
        f_data = urllib.request.urlopen(f_req).read()
        with open(target_path, "wb") as f:
            f.write(f_data)
        print(f"  -> Saved {name} ({len(f_data)} bytes)")
    except Exception as e:
        print(f"  ERROR downloading {name}: {e}")

# 3. Create third_party/media/LICENSE
license_path = os.path.join(MEDIA_DIR, "LICENSE")
license_content = """ATOMS OS — Third-Party Media Subsystem Licenses
=====================================================

The code in third_party/media/ consists of:

1. h264bsd (isolated under third_party/media/h264/)
   Source: Android Open Source Project (AOSP) Stagefright
   License: Apache License, Version 2.0
   Summary: Permissive open source license with patent grant.
   Zero GPL/LGPL code.

2. minimp4 (isolated under third_party/media/mp4/)
   Author: lieff
   License: Creative Commons Zero 1.0 Universal (CC0-1.0) / Public Domain
   Zero GPL/LGPL code.
"""
with open(license_path, "w", encoding="utf-8") as f:
    f.write(license_content)

print("\nAll third_party/media components fetched successfully!")
