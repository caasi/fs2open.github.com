"""Dot-pattern measure for blurred stars: RMS of the image minus a 2 px Gaussian blur of itself
(fine, pixel-scale texture), in the empty sky at the top left (x 0-400, y 0-300), grey 0-255.
Also the mean, to show the overall sky brightness. Usage: dots.py <png> ..."""
import sys
import numpy as np
from PIL import Image
from scipy import ndimage
for f in sys.argv[1:]:
    g = np.asarray(Image.open(f).convert("L")).astype(float)[:300, :400]
    hf = g - ndimage.gaussian_filter(g, 2)
    print(f"{f.split('/')[-1]:28s} fine RMS {hf.std():5.2f}  mean {g.mean():5.2f}")
