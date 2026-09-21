import os
import urllib.request
import tarfile
import hashlib

KERNEL_TXZ_URL = "https://archive.freebsd.org/old-releases/amd64/14.1-RELEASE/kernel.txz"
EXPECTED_SHA256 = "f05c0a74f101ee5638185c36003ed4d3dd6972826de6a64d91ec5cd0fdd3a0a6"
OUTPUT_DIR = "D:/Signatures_OS/tools/freebsd_payload"
os.makedirs(OUTPUT_DIR, exist_ok=True)
LOCAL_TXZ = os.path.join(OUTPUT_DIR, "kernel_14_1.txz")
EXTRACTED_KERNEL = os.path.join(OUTPUT_DIR, "freebsd14_kernel.elf")

def main():
    if os.path.exists(LOCAL_TXZ):
        h = hashlib.sha256()
        with open(LOCAL_TXZ, 'rb') as f:
            while chunk := f.read(65536):
                h.update(chunk)
        if h.hexdigest() == EXPECTED_SHA256:
            print("[INFO] Archive already fully downloaded and verified!")
        else:
            try:
                os.remove(LOCAL_TXZ)
            except Exception:
                pass

    if not os.path.exists(LOCAL_TXZ):
        print(f"[1/4] Downloading {KERNEL_TXZ_URL}...")
        req = urllib.request.Request(KERNEL_TXZ_URL, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as resp, open(LOCAL_TXZ, 'wb') as out_f:
            total = int(resp.headers.get('Content-Length', 0))
            done = 0
            while True:
                buf = resp.read(512 * 1024)
                if not buf:
                    break
                out_f.write(buf)
                done += len(buf)
                print(f"\r  {done / (1024*1024):.1f} / {total / (1024*1024):.1f} MB ({(done/total)*100:.1f}%)", end="", flush=True)
            print()

    print("[2/4] Verifying SHA-256...")
    h = hashlib.sha256()
    with open(LOCAL_TXZ, 'rb') as f:
        while chunk := f.read(65536):
            h.update(chunk)
    digest = h.hexdigest()
    print("  SHA-256:", digest)
    if digest != EXPECTED_SHA256:
        print("[ERROR] Checksum mismatch!")
        return False

    print("[3/4] Extracting /boot/kernel/kernel...")
    with tarfile.open(LOCAL_TXZ, "r:xz") as tar:
        for member in tar:
            if member.name in ("boot/kernel/kernel", "./boot/kernel/kernel", "/boot/kernel/kernel"):
                print(f"  Extracting {member.name} ({member.size} bytes)...")
                f = tar.extractfile(member)
                if f:
                    with open(EXTRACTED_KERNEL, "wb") as out_k:
                        out_k.write(f.read())
                break

    if os.path.exists(EXTRACTED_KERNEL):
        print(f"[4/4] Successfully extracted genuine FreeBSD kernel: {EXTRACTED_KERNEL} ({os.path.getsize(EXTRACTED_KERNEL)} bytes)")
        return True
    else:
        print("[ERROR] Could not extract kernel!")
        return False

if __name__ == "__main__":
    main()
