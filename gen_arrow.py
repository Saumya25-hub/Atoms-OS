from PIL import Image, ImageDraw

img = Image.new('L', (16, 24), 0)
draw = ImageDraw.Draw(img)
polygon = [(0,0), (15,11), (7,13), (5,19)]
draw.polygon(polygon, fill=1, outline=2)

for y in range(24):
    line = ''
    for x in range(16):
        p = img.getpixel((x, y))
        if p == 2: line += 'B'
        elif p == 1: line += 'W'
        else: line += ' '
    print('"' + line + '",')
