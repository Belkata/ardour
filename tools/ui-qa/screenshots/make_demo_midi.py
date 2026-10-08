#!/usr/bin/env python3
"""Write small demo .mid files (8 bars, 120 bpm, 4/4) for MIDI editor screenshots."""
import struct, sys, os
PPQ = 480
def vlq (n):
	b = [n & 0x7f]; n >>= 7
	while n: b.insert (0, (n & 0x7f) | 0x80); n >>= 7
	return bytes (b)
def smf (path, notes, ch=0):
	ev = []
	for (start, dur, key, vel) in notes:
		ev.append ((int (start * PPQ), 1, bytes ([0x90 | ch, key, vel])))
		ev.append ((int ((start + dur) * PPQ), 0, bytes ([0x80 | ch, key, 0])))
	ev.sort ()
	trk = b'\x00\xff\x51\x03\x07\xa1\x20'  # tempo 120
	t = 0
	for (tick, _, data) in ev:
		trk += vlq (tick - t) + data; t = tick
	trk += b'\x00\xff\x2f\x00'
	open (path, 'wb').write (b'MThd' + struct.pack ('>IHHH', 6, 0, 1, PPQ) + b'MTrk' + struct.pack ('>I', len (trk)) + trk)
out = sys.argv[1]; os.makedirs (out, exist_ok=True)
prog = [[60, 64, 67], [57, 60, 64], [53, 57, 60], [55, 59, 62]]   # C Am F G
chords, bass, melody, drums = [], [], [], []
for bar in range (8):
	c = prog[bar % 4]; b0 = bar * 4
	for i, k in enumerate (c): chords.append ((b0, 3.75, k, (96, 64, 78)[i] - (bar % 2) * 14))   # root loudest, every other bar softer
	for i, off in enumerate ((0, 1.5, 2, 3, 3.5)):
		bass.append ((b0 + off, .45, c[0] - 24 + (12 if i == 3 else 0), 90 - i * 6))
	for beat in range (4):
		drums.append ((b0 + beat, .25, 36 if beat % 2 == 0 else 38, 110 if beat % 2 == 0 else 95))
	for e in range (8):
		drums.append ((b0 + e * .5, .2, 42, 60 + (e % 2) * 30))
	mel = [c[2] + 12, c[1] + 12, c[2] + 12, c[0] + 24, c[2] + 12, c[1] + 12]
	for i, (off, d) in enumerate (((0, .5), (.5, .5), (1, 1), (2, .75), (2.75, .25), (3, 1))):
		melody.append ((b0 + off, d * .95, mel[i], 64 + (i * 13) % 50))
smf (out + '/drums.mid', drums, 9); smf (out + '/bass.mid', bass); smf (out + '/keys.mid', chords); smf (out + '/lead.mid', melody)
