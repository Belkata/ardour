"""Put each track's region into a few cue slots of the demo session.

usage: python3 fill_cues.py <session.ardour>

Ardour has no Lua API for clip slots, but triggers are stored in the
session file with a region id, and reloaded from it. We point the
first few Trigger nodes of every track's triggerbox at a region from
that track's playlist.
"""
import sys
import xml.etree.ElementTree as ET

path = sys.argv[1]
tree = ET.parse(path)
root = tree.getroot()

# playlist id -> first region id
pl_region = {}
for pl in root.iter("Playlist"):
    reg = pl.find("Region")
    if reg is not None:
        pl_region[pl.get("id")] = reg.get("id")

# which slots get a clip, per track (by import order); gives a varied grid
pattern = {
    "kick":   [0, 1, 2, 3, 5],
    "snare":  [1, 2, 3, 4],
    "bass":   [1, 2, 3, 5],
    "guitar": [0, 3, 5],
    "vocal":  [1, 3, 4],
}

filled = 0
for route in root.iter("Route"):
    name = (route.get("name") or "").lower()
    slots = pattern.get(name)
    if slots is None:
        continue
    # show the track on the cue page (imported tracks are hidden there by default)
    pi = route.find("PresentationInfo")
    if pi is not None and "TriggerTrack" not in pi.get("flags", ""):
        pi.set("flags", pi.get("flags") + ",TriggerTrack")
    plid = route.get("audio-playlist")
    rid = pl_region.get(plid)
    if not rid:
        print("no region for", name, file=sys.stderr)
        continue
    for proc in route.iter("Processor"):
        if proc.get("type") != "triggerbox":
            continue
        for trig in proc.iter("Trigger"):
            if int(trig.get("index", -1)) in slots:
                trig.set("region", rid)
                trig.set("name", "%s %s" % (name, "ABCDEFGH"[int(trig.get("index"))]))
                filled += 1

tree.write(path, encoding="UTF-8", xml_declaration=True)
print("filled", filled, "slots")
