import os
import re

for root, _, files in os.walk('kernel/audio'):
    for file in files:
        if file.endswith('.c') or file.endswith('.h'):
            path = os.path.join(root, file)
            with open(path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_content = content
            
            # Replace relative includes that step out of kernel/audio
            # E.g., #include "../core/memory/heap/include/heap.h"
            content = re.sub(r'#include\s+"(?:\.\./)+core/(.*)"', r'#include "kernel/core/\1"', content)
            content = re.sub(r'#include\s+"(?:\.\./)+drivers/display/(.*)"', r'#include "kernel/drivers/display/\1"', content)
            content = re.sub(r'#include\s+"(?:\.\./)+debug/(.*)"', r'#include "kernel/debug/\1"', content)
            content = re.sub(r'#include\s+"(?:\.\./)+vfs/(.*)"', r'#include "kernel/vfs/\1"', content)
            content = re.sub(r'#include\s+"(?:\.\./)+audio/(.*)"', r'#include "kernel/audio/\1"', content)
            
            if content != original_content:
                with open(path, 'w', encoding='utf-8') as f:
                    f.write(content)
                print(f'Updated {path}')
