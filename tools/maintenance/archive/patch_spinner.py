
with open('kernel/drivers/input/cursor/cursor_theme.c', 'r') as f:
    lines = f.readlines()

start_idx = -1
end_idx = -1
for i, line in enumerate(lines):
    if '/* Busy Spinner (4 animated frames) */' in line:
        start_idx = i
    if '/* Wait Hourglass (4 animated frames) */' in line:
        end_idx = i
        break

if start_idx != -1 and end_idx != -1:
    with open('spinner_c.txt', 'r') as f:
        spinner_c = f.readlines()
        
    new_lines = lines[:start_idx] + spinner_c + lines[end_idx:]
    with open('kernel/drivers/input/cursor/cursor_theme.c', 'w') as f:
        f.writelines(new_lines)
    print('Patched successfully!')
else:
    print('Could not find boundaries!')

