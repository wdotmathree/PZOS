from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont
import numpy as np

WIDTH = 9
HEIGHT = 17
SIZE = 16
ADJUST_HEIGHT = 3

font = TTFont("in.ttf")
img_font = ImageFont.truetype("in.ttf", SIZE)

bmp = np.zeros((HEIGHT * WIDTH * (0x7f - 0x20)), dtype=np.uint8)
preview = np.zeros((HEIGHT, WIDTH * (0x7f - 0x20)), dtype=np.uint8)

i = 0
for j in range(0x20, 0x7f):
	c = chr(j)

	img = Image.new('RGB', (WIDTH, HEIGHT), color='black')
	draw = ImageDraw.Draw(img)
	draw.text((0, -ADJUST_HEIGHT), c, font=img_font, fill='white')
	for y in range(HEIGHT):
		for x in range(WIDTH):
			bmp[y * WIDTH + x + i * HEIGHT] = img.getpixel((x, y))[0]
			preview[y, i + x] = img.getpixel((x, y))[0]
	i += WIDTH

with open("glyphs.bmp", "wb") as f:
	Image.fromarray(preview).save(f, format='BMP')

with open("glyphs.bin", "wb") as f:
	bmp.tofile(f)
