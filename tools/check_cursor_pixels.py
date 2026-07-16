from PIL import Image

def find_cursor():
    img = Image.open("cursor_desktop.ppm")
    w, h = img.width, img.height
    print(f"Checking {w}x{h} image for cursor sprite pixels...")
    
    # Let's inspect the bounding box where mouse_move 100 100 -> -50 -50 -> 20 20 landed
    # Initial mouse position or moved position
    # Let's sample pixel differences or find typical cursor colors (white/black border/red hotspot or standard pointer colors)
    # Let's check what the cursor bitmap looks like in memory or search for non-wallpaper pixels near expected coordinates
    # Let's check color histogram or specific bright/distinct pixels
    found = 0
    min_x, max_x = w, 0
    min_y, max_y = h, 0
    for y in range(h):
        for x in range(w):
            r, g, b = img.getpixel((x, y))
            # Let's check if there's pure white (255,255,255) or cursor border (0,0,0) in the central workspace
            # Note: wallpaper or desktop icons might have colors, let's look around the center (where mouse initialized and moved)
            if y < 700 and (r == 255 and g == 255 and b == 255):
                if x > min_x - 50 and x < max_x + 50 or found == 0:
                    found += 1
                    if x < min_x: min_x = x
                    if x > max_x: max_x = x
                    if y < min_y: min_y = y
                    if y > max_y: max_y = y
    print(f"White pixels detected in desktop area: {found}, box: ({min_x}, {min_y}) -> ({max_x}, {max_y})")

if __name__ == "__main__":
    find_cursor()
