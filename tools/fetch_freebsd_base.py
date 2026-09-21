import os
import urllib.request
import tarfile
import hashlib

BASE_TXZ_URL = "https://archive.freebsd.org/old-releases/amd64/14.1-RELEASE/base.txz"
EXPECTED_SHA256 = "bb451694e8435e646b5ff7ddc5e94d5c6c9649f125837a34b2a2dd419732f347"
OUTPUT_DIR = "D:/Signatures_OS/tools/freebsd_payload"
os.makedirs(OUTPUT_DIR, exist_ok=True)
LOCAL_BASE_TXZ = os.path.join(OUTPUT_DIR, "base_14_1.txz")
MINIMAL_ROOTFS_DIR = os.path.join(OUTPUT_DIR, "minimal_rootfs")
os.makedirs(MINIMAL_ROOTFS_DIR, exist_ok=True)

def main():
    if not os.path.exists(LOCAL_BASE_TXZ):
        print(f"[1/3] Downloading official FreeBSD 14.1-RELEASE amd64 base.txz from {BASE_TXZ_URL}...")
        req = urllib.request.Request(BASE_TXZ_URL, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as resp, open(LOCAL_BASE_TXZ, 'wb') as out_f:
            total = int(resp.headers.get('Content-Length', 0))
            done = 0
            while True:
                buf = resp.read(1024 * 1024)
                if not buf:
                    break
                out_f.write(buf)
                done += len(buf)
                print(f"\r  {done / (1024*1024):.1f} / {total / (1024*1024):.1f} MB ({(done/total)*100:.1f}%)", end="", flush=True)
            print()

    print("[2/3] Verifying SHA-256...")
    h = hashlib.sha256()
    with open(LOCAL_BASE_TXZ, 'rb') as f:
        while chunk := f.read(65536):
            h.update(chunk)
    digest = h.hexdigest()
    print("  SHA-256:", digest)
    if digest != EXPECTED_SHA256:
        print("[ERROR] Checksum mismatch!")
        return False
    print("  [OK] SHA-256 matches official FreeBSD 14.1-RELEASE release manifest!")

    print("[3/3] Extracting essential userspace binaries for minimal FreeBSD rootfs...")
    essential_paths = [
        "sbin/init", "bin/sh", "bin/echo", "bin/ls", "bin/cat", "bin/hostname",
        "libexec/ld-elf.so.1",
        "lib/libc.so.7", "lib/libedit.so.8", "lib/libncursesw.so.9", "lib/libthr.so.3", "lib/libm.so.5",
        "etc/rc", "etc/rc.conf", "etc/ttys", "etc/fstab", "etc/login.conf"
    ]
    extracted_count = 0
    with tarfile.open(LOCAL_BASE_TXZ, "r:xz") as tar:
        for member in tar:
            norm_name = member.name.lstrip("./")
            if any(norm_name == p or norm_name.startswith(p + "/") for p in essential_paths):
                tar.extract(member, path=MINIMAL_ROOTFS_DIR)
                extracted_count += 1

    print(f"[SUCCESS] Extracted {extracted_count} essential FreeBSD userspace files into {MINIMAL_ROOTFS_DIR}!")
    return True

if __name__ == "__main__":
    main()
