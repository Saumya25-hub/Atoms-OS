
with open('boot/stage2.asm', 'r') as f:
    code = f.read()

bad_copy = '''    ; Copy chunk to high memory by temporarily entering Protected Mode
    cli
    push ds
    push es'''

good_copy = '''    ; Copy chunk to high memory by temporarily entering Protected Mode
    pushad
    cli
    push ds
    push es'''

code = code.replace(bad_copy, good_copy)

bad_exit = '''    pop es
    pop ds
    sti

    sub cx, dx'''

good_exit = '''    pop es
    pop ds
    sti
    popad

    sub cx, dx'''

code = code.replace(bad_exit, good_exit)

with open('boot/stage2.asm', 'w') as f:
    f.write(code)

print('Fixed!')

