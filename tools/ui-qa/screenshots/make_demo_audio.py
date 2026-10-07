#!/usr/bin/env python3
"""Write five short synthetic stems (kick, snare, bass, guitar, vocal) for the
screenshot demo session. usage: make_demo_audio.py <out-dir>"""
import math, os, random, struct, sys, wave

out = sys.argv[1]
os.makedirs(out, exist_ok=True)
sr = 48000; beat = 60 / 120; secs = int(beat * 4 * 10)
random.seed(1)

def write(name, f):
    w = wave.open(os.path.join(out, name + ".wav"), "wb")
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr)
    w.writeframes(b"".join(struct.pack("<h", int(max(-1, min(1, f(i / sr))) * 30000)) for i in range(secs * sr)))
    w.close()

def kick(t):
    p = t % beat; return math.exp(-p * 18) * math.sin(2 * math.pi * (50 + 120 * math.exp(-p * 40)) * p)
def snare(t):
    p = (t + beat) % (2 * beat)
    return math.exp(-p * 22) * (random.uniform(-1, 1) * 0.7 + 0.3 * math.sin(2 * math.pi * 190 * p)) if p < 0.4 else 0
def bass(t):
    n = [55, 55, 65.4, 49][int(t / (beat * 4)) % 4]; p = t % (beat / 2)
    return 0.6 * math.tanh(2 * math.sin(2 * math.pi * n * t)) * (0.5 + 0.5 * math.exp(-p * 6))
def vocal(t):
    env = max(0, math.sin(2 * math.pi * t / (beat * 8))) ** 0.7
    f = 220 * (1 + 0.02 * math.sin(2 * math.pi * 5 * t))
    return env * 0.5 * (math.sin(2 * math.pi * f * t) + 0.4 * math.sin(4 * math.pi * f * t) + 0.2 * random.uniform(-1, 1))
def guitar(t):
    p = t % (beat / 2); return 0.5 * math.exp(-p * 5) * sum(math.sin(2 * math.pi * fr * t) for fr in (196, 247, 294)) / 3

for name, fn in [("kick", kick), ("snare", snare), ("bass", bass), ("guitar", guitar), ("vocal", vocal)]:
    write(name, fn)
