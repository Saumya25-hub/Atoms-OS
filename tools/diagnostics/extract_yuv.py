import os
import sys

# Create Host Windows folder D:\BOSPECTRA_TEST
host_dir = r"D:\BOSPECTRA_TEST"
os.makedirs(host_dir, exist_ok=True)
print(f"[HOST] Folder verified/created: {host_dir}")

# Look for raw frame bytes in build\OS.img or build\SignaturesOS.vmdk
img_path = r"build\OS.img"
if os.path.exists(img_path):
    print(f"[HOST] Inspecting virtual disk: {img_path}")
    with open(img_path, "rb") as f:
        data = f.read()

    # Search for YUV dump marker or RAW frame data
    out_file = os.path.join(host_dir, "frame_0001.yuv")
    # For a 640x360 frame, YUV420P total size is 640*360 * 1.5 = 345600 bytes
    frame_size = 640 * 360 + 320 * 180 + 320 * 180 # 345600 bytes
    
    # If frame_0001.yuv is found in the FAT32 disk image
    marker = b"frame_0001.yuv"
    idx = data.find(marker)
    if idx != -1:
        print(f"[HOST] Found frame_0001.yuv entry in OS image at offset {hex(idx)}")
    
    # Check if raw files exist in project root or build
    raw_files = [f for f in os.listdir(".") if f.endswith(".yuv") or f.endswith(".raw")]
    print(f"[HOST] Raw files in workspace: {raw_files}")

