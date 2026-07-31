#!/usr/bin/env python3
"""
BOS OS Package Builder Tool (bospack)
Packages application binary, manifest.json, and resources into a .bosapp bundle archive.
"""

import sys
import os
import json
import zipfile

def main():
    print("=========================================")
    print("      BOS SDK Package Builder Tool       ")
    print("=========================================")

    if len(sys.argv) < 3:
        print("Usage: python bospack.py <app_dir> <output_package.bosapp>")
        sys.exit(1)

    app_dir = sys.argv[1]
    out_pkg = sys.argv[2]

    manifest_path = os.path.join(app_dir, "manifest.json")
    if not os.path.exists(manifest_path):
        print(f"[ERROR] manifest.json not found in {app_dir}")
        sys.exit(1)

    print(f"[SDK PACK] Creating package archive: {out_pkg}")
    with zipfile.ZipFile(out_pkg, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for root, _, files in os.walk(app_dir):
            for file in files:
                abs_path = os.path.join(root, file)
                rel_path = os.path.relpath(abs_path, app_dir)
                zipf.write(abs_path, rel_path)

    print(f"[SDK PACK SUCCESS] .bosapp package generated successfully: {out_pkg}")

if __name__ == "__main__":
    main()
