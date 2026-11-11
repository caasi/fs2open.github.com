"""Mean brightness of the sky as a function of distance (px) from the ship silhouette.
Usage: ring.py <coc-debug.png> <image.png> [...]. The mask is 'not sky' = pixels that are not
red-dominant in the FSO_DOF_DEBUG=1 view (sky and the far ISD are red there)."""
import sys
import numpy as np
from PIL import Image
from scipy import ndimage
dbg = np.asarray(Image.open(sys.argv[1]).convert("RGB")).astype(float)
red = (dbg[..., 0] > 2 * dbg[..., 1] + 20)
ship = ndimage.binary_opening(~red, iterations=1)
lab, n = ndimage.label(ship)
big = lab == (np.argmax(np.bincount(lab.ravel())[1:]) + 1)  # largest non-red blob: the X-wing
dist = ndimage.distance_transform_edt(~big)
# Only the left half of the frame below the ISD: no ISD, no engine glows.
roi = np.zeros_like(red); roi[440:620, 430:545] = True  # left wing tips, away from the engines
red &= roi
bands = [(1, 3), (3, 6), (6, 10), (10, 16), (16, 24), (24, 40), (40, 80)]
print("file".ljust(28), " ".join(f"{a:>2}-{b:<3}" for a, b in bands))
for f in sys.argv[2:]:
    img = np.asarray(Image.open(f).convert("L")).astype(float)
    m = [(dist >= a) & (dist < b) & red for a, b in bands]
    print(f.split("/")[-1].ljust(28), " ".join(f"{img[k].mean():6.2f}" for k in m), " mean")
    print("".ljust(28), " ".join(f"{(img[k] < 8).mean():6.2f}" for k in m), " fraction < 8")
