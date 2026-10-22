#!/usr/bin/env python3
"""Genere assets/icon.png (48x48), assets/banner.png (256x128), assets/audio.wav.
Ne remplace PAS un fichier deja present. Aucune dependance externe."""
import os, struct, zlib, wave, math, random

os.makedirs("assets", exist_ok=True)

def write_png(path, w, h, px):
    raw = b"".join(b"\x00" + bytes(c for p in px[y*w:(y+1)*w] for c in p) for y in range(h))
    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
    data = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    data += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    open(path, "wb").write(data)

def alien_pixel(u, v):
    """u,v entre 0 et 1 : tete d'alien verte avec grands yeux noirs."""
    dx, dy = (u - 0.5) / 0.38, (v - 0.48) / 0.42
    if dx*dx + dy*dy < 1.0 and v < 0.85:
        for ex in (0.36, 0.64):
            a, b = (u - ex) / 0.10, (v - 0.42) / 0.07
            if a*a + b*b < 1.0:
                return (0, 0, 0)
        return (70, 200, 90)
    return None

def make_icon(path):
    n, px = 48, []
    for y in range(n):
        for x in range(n):
            a = alien_pixel(x / n, y / n)
            px.append(a if a else (10 + y // 6, 12, 18))
    write_png(path, n, n, px)

def make_banner(path):
    w, h, px = 256, 128, []
    for y in range(h):
        for x in range(w):
            a = alien_pixel((x - 78) / 100.0, y / h) if 78 <= x < 178 else None
            if a:
                px.append(a)
            else:
                px.append((8 + (x % 32 == 0) * 6, 10, 16 + y // 8))
    write_png(path, w, h, px)

def make_audio(path):
    rate, secs = 22050, 2.0
    n = int(rate * secs)
    w = wave.open(path, "wb")
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
    frames = bytearray()
    for i in range(n):
        t = i / rate
        s = 0.5*math.sin(2*math.pi*55*t) + 0.4*math.sin(2*math.pi*58*t) + 0.2*math.sin(2*math.pi*110.5*t)
        s *= 0.6 + 0.4*math.sin(2*math.pi*0.7*t)
        s += random.uniform(-0.03, 0.03)
        fade = min(1.0, t / 0.2, (secs - t) / 0.3)
        v = int(max(-1, min(1, s * 0.6 * fade)) * 30000)
        frames += struct.pack("<h", v)
    w.writeframes(bytes(frames)); w.close()

for name, fn in (("icon.png", make_icon), ("banner.png", make_banner), ("audio.wav", make_audio)):
    p = os.path.join("assets", name)
    if os.path.exists(p):
        print("deja present :", p)
    else:
        fn(p); print("cree :", p)
