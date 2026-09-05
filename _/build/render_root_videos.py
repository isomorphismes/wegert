#!/usr/bin/env python3
import math, os, subprocess, sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont

TAU = 2.0 * math.pi
LOG10 = math.log(10.0)
FPS = 18
PLOT = 400
HEAD = 78
W = PLOT
H = HEAD + PLOT
NFRAMES = 72
OUT = Path(sys.argv[1] if len(sys.argv) > 1 else '.')
OUT.mkdir(parents=True, exist_ok=True)

# Exact constants/formulas from wegert_color.glsl.
WHITE_U = 0.19783982482140777
WHITE_V = 0.46833630293240974

x = np.linspace(-2.35, 2.35, PLOT, dtype=np.float32)
y = np.linspace(2.35, -2.35, PLOT, dtype=np.float32)
X, Y = np.meshgrid(x, y)
Z = X + 1j * Y

try:
    FONT = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 16)
    SMALL = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 11)
except Exception:
    FONT = ImageFont.load_default(); SMALL = FONT

def fract(a):
    return a - np.floor(a)

def srgb_component(v):
    v = np.maximum(v, 0.0)
    return np.where(v <= 0.0031308, 12.92*v, 1.055*np.power(v, 1.0/2.4)-0.055)

def wegert_rgb(w):
    phase = np.angle(w)
    logm = np.log(np.maximum(np.abs(w), 1e-12))
    hue = 360.0 * fract(phase / TAU)
    band = fract(logm / LOG10)
    L = 66.0 + 4.0*band + 3.0*fract(hue / 100.0)
    C = 45.0
    hr = np.deg2rad(hue)
    us = C*np.cos(hr); vs = C*np.sin(hr)
    yy = np.where(L > 8.0, ((L+16.0)/116.0)**3, L/903.2962962962963)
    up = us/(13.0*L) + WHITE_U
    vp = vs/(13.0*L) + WHITE_V
    xx = (9.0*yy*up)/(4.0*vp)
    zz = yy*(12.0-3.0*up-20.0*vp)/(4.0*vp)
    lr = 3.2404542*xx - 1.5371385*yy - 0.4985314*zz
    lg = -0.9692660*xx + 1.8760108*yy + 0.0415560*zz
    lb = 0.0556434*xx - 0.2040259*yy + 1.0572252*zz
    rgb = np.stack([srgb_component(lr), srgb_component(lg), srgb_component(lb)], axis=-1)
    return np.clip(rgb*255.0, 0, 255).astype(np.uint8)

def dark_rgb(w):
    phase = fract((np.angle(w) + math.pi) / TAU)
    logm = np.log(np.maximum(np.abs(w), 1e-10))
    h = phase
    s = np.full_like(h, 0.92)
    rings = 0.5 + 0.5*np.cos(TAU*fract(logm/LOG10))
    v = 0.24 + 0.66*(0.25 + 0.75*rings)
    i = np.floor(h*6).astype(np.int32)
    f = h*6 - i
    p = v*(1-s); q = v*(1-f*s); t = v*(1-(1-f)*s)
    im = i % 6
    r = np.choose(im, [v,q,p,p,t,v]); g=np.choose(im,[t,v,v,q,p,p]); b=np.choose(im,[p,p,t,v,v,q])
    rgb=np.stack([r,g,b],axis=-1)
    return np.clip(rgb*255,0,255).astype(np.uint8)

def eval_rational(zeros, poles):
    w = np.ones_like(Z, dtype=np.complex64)
    for a,m in zeros:
        w *= (Z-a)**m
    for b,m in poles:
        w /= np.where(np.abs(Z-b) < 1e-7, 1e-7+0j, (Z-b)**m)
    return w

def pix(z):
    px = int(round((z.real + 2.35) / 4.70 * (PLOT-1)))
    py = int(round((2.35 - z.imag) / 4.70 * (PLOT-1))) + HEAD
    return px, py

def annotate(rgb, title, subtitle, zeros, poles):
    canvas = Image.new('RGB', (W,H), 'white')
    canvas.paste(Image.fromarray(rgb), (0,HEAD))
    d=ImageDraw.Draw(canvas)
    d.text((10,8), title, fill=(20,20,20), font=FONT)
    d.text((10,34), subtitle, fill=(75,75,75), font=SMALL)
    d.text((10,56), 'phase = time    log-modulus base = 10', fill=(100,100,100), font=SMALL)
    for a,m in zeros:
        cx,cy=pix(a); r=5+2*(m-1)
        d.ellipse((cx-r,cy-r,cx+r,cy+r), fill=(255,255,255), outline=(20,20,20), width=1)
        if m>1: d.ellipse((cx-r+3,cy-r+3,cx+r-3,cy+r-3), outline=(20,20,20), width=1)
    for b,m in poles:
        cx,cy=pix(b); r=5+2*(m-1)
        d.ellipse((cx-r,cy-r,cx+r,cy+r), fill=(20,20,20), outline=(255,255,255), width=1)
        if m>1: d.ellipse((cx-r+3,cy-r+3,cx+r-3,cy+r-3), outline=(255,255,255), width=1)
    return np.asarray(canvas)

def encode(name, frame_iter, width=W, height=H, fps=FPS):
    path=OUT/name
    cmd=['ffmpeg','-y','-loglevel','error','-f','rawvideo','-pix_fmt','rgb24','-s',f'{width}x{height}','-r',str(fps),'-i','-','-an','-c:v','libx264','-preset','slow','-crf','30','-pix_fmt','yuv420p','-movflags','+faststart',str(path)]
    p=subprocess.Popen(cmd, stdin=subprocess.PIPE)
    try:
        for fr in frame_iter:
            p.stdin.write(np.ascontiguousarray(fr,dtype=np.uint8).tobytes())
    finally:
        p.stdin.close()
    if p.wait()!=0: raise SystemExit(f'ffmpeg failed: {name}')
    print(path)

def orbit(c,r,t,phase=0.0):
    return c + r*np.exp(1j*(TAU*t+phase))

def frames_case(kind):
    for k in range(NFRAMES):
        t=k/NFRAMES
        if kind=='two_poles':
            zeros=[(-0.25+0.10j,1)]
            poles=[(orbit(-0.85-0.15j,0.42,t),1),(orbit(0.85+0.15j,0.42,t,math.pi),1)]
            title='Two moving simple poles'; sub='f(z) = (z-a) / ((z-p₁(t))(z-p₂(t)))'
        elif kind=='three_roots':
            zeros=[(orbit(-0.95+0.2j,0.35,t),1),(orbit(0.75+0.55j,0.40,t,2.1),1),(orbit(0.1-0.9j,0.32,t,4.2),1)]
            poles=[]; title='Polynomial with three moving simple roots'; sub='f(z) = ∏ᵢ (z-rᵢ(t))'
        elif kind=='simple_repeated':
            zeros=[(orbit(-0.65+0.15j,0.36,t),1),(orbit(0.55-0.35j,0.30,t,2.4),2)]
            poles=[]; title='Polynomial with simple and repeated roots'; sub='f(z) = (z-r₁(t))(z-r₂(t))²'
        elif kind=='simple_double_poles':
            zeros=[(-0.15+0.25j,1)]
            poles=[(orbit(-0.75,0.32,t),1),(orbit(0.65+0.05j,0.30,t,math.pi),2)]
            title='Moving simple and double poles'; sub='f(z) = (z-a) / ((z-p₁(t))(z-p₂(t))²)'
        elif kind=='two_two':
            zeros=[(orbit(-0.75+0.45j,0.28,t),1),(orbit(0.45-0.65j,0.30,t,1.6),1)]
            poles=[(orbit(0.75+0.55j,0.28,t,3.2),1),(orbit(-0.45-0.55j,0.30,t,4.8),1)]
            title='Two simple zeros and two simple poles'; sub='f(z) = (z-z₁(t))(z-z₂(t)) / ((z-p₁(t))(z-p₂(t)))'
        elif kind=='repeated_both':
            zeros=[(orbit(-0.75+0.35j,0.30,t),1),(orbit(0.05-0.55j,0.30,t,2.0),2)]
            poles=[(orbit(0.75+0.35j,0.30,t,4.0),1),(orbit(-0.05+0.75j,0.25,t,5.0),2)]
            title='Simple and double zeros and poles'; sub='f(z) = (z-z₁)(z-z₂)² / ((z-p₁)(z-p₂)²)'
        w=eval_rational(zeros,poles)
        yield annotate(wegert_rgb(w),title,sub,zeros,poles)

def meromorphic_frames():
    n=15*FPS
    S=560
    xx=np.linspace(-2.6,2.6,S,dtype=np.float32); yy=np.linspace(2.6,-2.6,S,dtype=np.float32)
    XX,YY=np.meshgrid(xx,yy); ZZ=XX+1j*YY
    for k in range(n):
        t=k/n
        roots=[-1.4*np.exp(1j*(TAU*t)), 0.75*np.exp(1j*(TAU*t+2.0)), 1.2*np.exp(1j*(-TAU*t+4.0))]
        poles=[-0.7+0.8j*np.exp(1j*(TAU*t*0.7)), 0.9-0.65j*np.exp(1j*(TAU*t*0.8))]
        w=np.exp(0.18*ZZ)
        for a in roots: w*=ZZ-a
        for b in poles: w/=np.where(np.abs(ZZ-b)<1e-7,1e-7+0j,ZZ-b)
        rgb=dark_rgb(w)
        im=Image.fromarray(rgb); d=ImageDraw.Draw(im)
        for a in roots:
            px=int((a.real+2.6)/5.2*(S-1)); py=int((2.6-a.imag)/5.2*(S-1)); d.ellipse((px-4,py-4,px+4,py+4),fill='white',outline='black')
        for b in poles:
            px=int((b.real+2.6)/5.2*(S-1)); py=int((2.6-b.imag)/5.2*(S-1)); d.ellipse((px-4,py-4,px+4,py+4),fill='black',outline='white')
        yield np.asarray(im)

cases=[
 ('two_moving_simple_poles.mp4','two_poles'),
 ('three_moving_simple_roots.mp4','three_roots'),
 ('moving_simple_and_repeated_roots.mp4','simple_repeated'),
 ('moving_simple_and_double_poles.mp4','simple_double_poles'),
 ('moving_two_simple_zeros_and_two_simple_poles.mp4','two_two'),
 ('moving_repeated_zeros_and_poles.mp4','repeated_both'),
]
for name,kind in cases: encode(name,frames_case(kind))
encode('wegert_meromorphic_15s.mp4',meromorphic_frames(),560,560,FPS)
