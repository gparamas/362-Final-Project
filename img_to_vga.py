from PIL import Image
import numpy as np

def main():
    img = Image.open("square.png")
    img = img.resize((640, 480))
    data = list(img.getdata())
    data = np.array(data)
    print(data.shape)
    arr = []
    for i in range(0, len(data), 2):
        r1, g1, b1, c = data[i]
        r2, g2, b2, c1 = data[i+1]
        a = (((r1 > 127) & 0x01) | (((g1 > 127) & 0x01) << 1) | (((b1 > 127) & 0x01) << 2) | (((r2 > 127) & 0x01) << 3) | (((g2 > 127) & 0x01) << 4) | (((b2 > 127) & 0x01) << 5)) & 63
        arr.append(a)
    arr = np.array(arr, dtype=np.uint8)
    arr.tofile("img.vga")

def white():
    arr = np.array([np.uint8(0xFF) for x in range(0, 153600)], dtype=np.uint8)
    arr.tofile("img.vga")
 

def r():
    a = np.fromfile("img.vga", dtype=np.uint8)
    b = []
    for i in range(153600):
        b.append(np.uint8(i//512))
    b = np.array(b, np.uint8)
    a = np.bitwise_xor(a, b)
    a.tofile("peen")
    print(a)
    print(len(a))

def flag():
    flag = Image.open("flag.png")
    flag = flag.resize((30, 30))
    data = list(flag.getdata())

    arr = []

    for i in range(0, len(data), 2):
        r1, g1, b1, c = data[i]
        r2, g2, b2, c1 = data[i+1]
        a = (((r1 > 127) & 0x01) | (((g1 > 127) & 0x01) << 1) | (((b1 > 127) & 0x01) << 2) | (((r2 > 127) & 0x01) << 3) | (((g2 > 127) & 0x01) << 4) | (((b2 > 127) & 0x01) << 5)) & 63
        arr.append(a)
    arr = np.array(arr, dtype=np.uint8)
    arr.tofile("flag.vga")
    

flag()