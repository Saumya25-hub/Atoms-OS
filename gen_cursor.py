
grid = [[' ' for _ in range(16)] for _ in range(20)]

def draw_line(x0, y0, x1, y1, c):
    dx = abs(x1 - x0)
    dy = abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx - dy
    while True:
        if 0 <= y0 < 20 and 0 <= x0 < 16:
            grid[y0][x0] = c
        if x0 == x1 and y0 == y1:
            break
        e2 = 2 * err
        if e2 > -dy:
            err -= dy
            x0 += sx
        if e2 < dx:
            err += dx
            y0 += sy

draw_line(0, 0, 15, 11, 'B')
draw_line(15, 11, 7, 13, 'B')
draw_line(7, 13, 5, 19, 'B')
draw_line(5, 19, 0, 0, 'B')

for y in range(20):
    first_b = -1
    last_b = -1
    for x in range(16):
        if grid[y][x] == 'B':
            if first_b == -1: first_b = x
            last_b = x
    if first_b != -1 and first_b != last_b:
        for x in range(first_b+1, last_b):
            if grid[y][x] != 'B':
                grid[y][x] = 'W'

for row in grid:
    print('\x22' + ''.join(row) + '\x22,')

