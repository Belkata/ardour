#!/usr/bin/env python3
"""WCAG contrast checks for an Ardour color theme.

Part 1 (relative): text pairs must reach 4.5:1 (WCAG AA, normal text). For
buttons, Ardour picks the better of "gtk_foreground" / "gtk_background" as
text color (ArdourButton::get_contrasting_color), so we check the best
achievable ratio against each "<name>: fill" / "fill active" and require 3:1.
A pair only fails if the theme is below the threshold *and* worse than the
reference theme (dark), so pre-existing weaknesses are reported as warnings.

Part 2 (absolute, WCAG 2.x): a curated list of pairs (build_rules() below) that
must pass for the "modern" theme; for any other theme the same pairs are
only reported. Each rule says where the pair is used.

    text    >= 4.5:1  normal text (WCAG 1.4.3)
    large   >= 3.0:1  clearly large text, e.g. big clock digits (1.4.3)
    ui      >= 3.0:1  non-text: icons, LEDs, fills that carry meaning (1.4.11)
    info    advisory  printed, never fails; the reason is in the rule comment

Exemptions (deliberately not checked, see QA.md):
  * insensitive / disabled / bypassed text and widgets (WCAG 1.4.3, 1.4.11);
  * text drawn over user-chosen colors (track/region colors, marker
    colors set by the user): Ardour picks black or white itself
    (contrasting_text_color), the theme cannot influence it;
  * button outlines and button fill vs window background: every button is
    identified by its label or icon, whose contrast is checked (1.4.11 only
    asks for the parts needed to identify a control);
  * decorative lines (grid, separators, borders).

usage: check_contrast.py [--root <ardour checkout>] [--theme modern] [--ref dark]
"""

import argparse
import colorsys
import os
import re
import sys
import xml.etree.ElementTree as ET

TEXT_PAIRS = [
    ("gtk_foreground", "gtk_background", "window text"),
    ("gtk_foreground", "gtk_bases", "list/entry text"),
    ("gtk_fg_selected", "gtk_bg_selected", "selected list row"),
    ("theme:contrasting clock", "clock: background", "main clock digits"),
    ("neutral:foreground", "neutral:background", "text on track lanes"),
    ("neutral:foreground", "theme:bg1", "text on panels"),
]
BUTTONS = [
    "generic button", "transport button", "mouse mode button", "page switch button",
    "route button", "mixer strip button", "tab button", "zoom button",
    "mute button", "solo button", "record enable button", "primary button",
    "add track row button",
]
TEXT_MIN = 4.5
UI_MIN = 3.0
LARGE_MIN = 3.0


def load(path):
    root = ET.parse(path).getroot()
    colors = {c.get("name"): c.get("value") for c in root.find("Colors")}
    aliases = {a.get("name"): a.get("alias") for a in root.find("ColorAliases")}
    mods = {m.get("name"): m.get("modifier") for m in root.find("Modifiers")}
    return colors, aliases, mods


def resolve(theme, name):
    colors, aliases = theme[0], theme[1]
    v = colors.get(aliases.get(name, name))
    if v is None:
        return None
    v = v.lower().replace("0x", "")
    return tuple(int(v[i:i + 2], 16) / 255.0 for i in (0, 2, 4))


def lum(rgb):
    def ch(c):
        return c / 12.92 if c <= 0.03928 else ((c + 0.055) / 1.055) ** 2.4
    r, g, b = (ch(c) for c in rgb)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def ratio(a, b):
    la, lb = lum(a), lum(b)
    hi, lo = max(la, lb), min(la, lb)
    return (hi + 0.05) / (lo + 0.05)


def ardour_luminance(rgb):
    """Gtkmm2ext luminance(): gamma-encoded relative luminance, 0..1."""
    def lin(c):
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    y = 0.212655 * lin(rgb[0]) + 0.715158 * lin(rgb[1]) + 0.072187 * lin(rgb[2])
    return y * 12.92 if y <= 0.0031308 else 1.055 * y ** (1 / 2.4) - 0.055


# ---- color expressions used by the rules below ----------------------------
#   "name"                      alias or palette color
#   ("rgb", r, g, b)            literal, 0..1
#   ("over", fg, a, bg)       fg at alpha a over bg; a is a number or the
#                               name of a theme <Modifier> "= alpha:x"
#   ("mix", a, b, t)            a + (b - a) * t  (UINT_INTERPOLATE)
#   ("autotext", bg)            contrasting_text_color(): near-white if
#                               luminance < 0.5, else black (marker/region names)
#   ("btntext", fill[, icon])   ArdourButton::get_contrasting_color(): the
#                               better of gtk_foreground / gtk_background;
#                               icon=True adds the vector-icon bias of 0.35
#   ("shade", c, f)             GTK shade(f, c) (HLS lightness/saturation * f)

def modifier_alpha(theme, name):
    m = re.search(r"alpha:([0-9.]+)", theme[2].get(name, ""))
    return float(m.group(1)) if m else 1.0


def ev(theme, e):
    if isinstance(e, str):
        return resolve(theme, e)
    op = e[0]
    if op == "rgb":
        return tuple(e[1:4])
    if op == "over":
        fg, a, bg = ev(theme, e[1]), e[2], ev(theme, e[3])
        if isinstance(a, str):
            a = modifier_alpha(theme, a)
        return tuple(f * a + b * (1 - a) for f, b in zip(fg, bg))
    if op == "mix":
        a, b = ev(theme, e[1]), ev(theme, e[2])
        return tuple(x + (y - x) * e[3] for x, y in zip(a, b))
    if op == "autotext":
        return (0.98, 0.98, 0.98) if ardour_luminance(ev(theme, e[1])) < 0.5 else (0.0, 0.0, 0.0)
    if op == "btntext":
        fill = ev(theme, e[1])
        fg, bg = resolve(theme, "gtk_foreground"), resolve(theme, "gtk_background")
        bias = 0.35 if len(e) > 2 and e[2] else 0.0
        normal = abs(ardour_luminance(fg) - ardour_luminance(fill))
        invert = abs(ardour_luminance(bg) - ardour_luminance(fill))
        return fg if normal + bias > invert else bg
    if op == "shade":
        r, g, b = ev(theme, e[1])
        h, l, s = colorsys.rgb_to_hls(r, g, b)
        return colorsys.hls_to_rgb(h, min(1.0, l * e[2]), min(1.0, s * e[2]))
    raise ValueError(e)


# Buttons that show a text label (ArdourButton::Text) and buttons that show
# only a vector icon. Active fills of buttons with an LED are not text fills
# (text keeps the inactive color), but checking them is merely conservative.
TEXT_BUTTONS = [
    ("generic button", "default for every button without its own colors"),
    ("page switch button", "Editor/Mixer/Cue/... switcher in the top bar"),
    ("route button", "mixer strip In/Out/Comments buttons"),
    ("mixer strip button", "mixer strip buttons (Monitor In/Disk, phase...)"),
    ("tab button", "tab strip buttons"),
    ("mute button", "M button"),
    ("solo button", "S button"),
    ("primary button", "Add Track and other main actions in dialogs"),
    ("add track row button", "Quick Add row buttons"),
    ("tracknumber label", "track number badge in track header / strip"),
    ("rude solo", "'Solo active' pill in the top bar"),
    ("rude audition", "'Audition' pill in the top bar"),
    ("rude isolate", "'Isolate' pill in the top bar"),
    ("feedback alert", "'Feedback' pill in the top bar"),
    ("punch button", "Punch in / out in the top bar"),
    ("latency button", "latency button in the top bar"),
    ("monitor button", "Input / Disk monitoring in the mixer strip"),
    ("transport option button", "Auto Return etc. in the top bar"),
    ("transport active option button", "Sync etc. in the top bar"),
    ("processor prefader", "processor box entry (pre-fader)"),
    ("processor fader", "processor box entry at the fader (e.g. 'Fader')"),
    ("processor postfader", "processor box entry (post-fader)"),
]
ICON_BUTTONS = [
    ("transport button", "play/stop/record/loop icons"),
    ("mouse mode button", "Grab/Range/Cut/... tool icons"),
    ("zoom button", "zoom in/out/fit icons"),
    ("record enable button", "record-arm icon in track header / strip"),
]


# Buttons that really show an LED (led_default_elements / just_led_default_elements).
LED_BUTTONS = [
    ("generic button", "Rec Cues / Play Cues / Auto-Input (application_bar.cc, recorder_ui.cc)"),
    ("latency button", "top bar latency button"),
    ("solo isolate", "solo-isolate LED in the mixer strip (mixer_strip.cc)"),
    ("solo safe", "solo-safe LED in the mixer strip (mixer_strip.cc)"),
    ("processor prefader", "processor active LED (processor_box.cc)"),
    ("processor fader", "processor active LED"),
    ("processor postfader", "processor active LED"),
    ("processor control button", "processor control toggles"),
    ("foldback prefader", "foldback strip send (foldback_strip.cc)"),
    ("foldback postfader", "foldback strip send"),
    ("plugin bypass button", "plugin window bypass (plugin_ui.cc)"),
]


def build_rules(theme):
    """-> list of (level, fg_expr, bg_expr, min, label)."""
    R = []

    def add(level, fg, bg, label, minimum=None):
        minimum = {"text": TEXT_MIN, "large": LARGE_MIN, "ui": UI_MIN, "info": UI_MIN}[level] if minimum is None else minimum
        R.append((level, fg, bg, minimum, label))

    # -- rulers
    add("text", "ruler text", "ruler base", "ruler tick labels (editor_canvas.cc Editor::color_handler)")
    add("text", "gtk_light_text_on_dark", "gtk_background", "ruler row labels 'Mins:Secs', 'Bars:Beats' (EditorRulerLabel, clearlooks.rc.in)")
    # tempo/meter marker names have no flag and are drawn white (marker.cc ArdourMarker::set_color)
    for bar in ("tempo bar", "meter bar", "marker bar"):
        add("text", ("rgb", 1.0, 1.0, 1.0), ("over", bar, "marker bar", "ruler base"), "white marker/tempo names on the %s" % bar)
    # -- track headers, strips, panels (route name labels use gtk_foreground)
    for bg, what in (("gtk_audio_track", "audio track header"), ("gtk_audio_bus", "bus header/strip"),
                     ("gtk_midi_track", "MIDI track header/strip"), ("gtk_track_header_selected", "selected track header"),
                     ("gtk_automation_track_header", "automation lane header")):
        add("text", "gtk_foreground", bg, "name text on %s" % what)
    add("text", "gtk_fg_tooltip", "gtk_bg_tooltip", "tooltips")
    add("text", "neutral:foreground", "theme:selected header", "text on selected track header (gtk_track_header_selected)")
    # -- clocks (digits are large text; cursor/edited text also)
    for clk in ("clock", "big clock", "nudge clock", "secondary clock", "transport clock", "stretch clock"):
        add("text", clk + ": text", clk + ": background", "%s digits" % clk)
        add("large", clk + ": edited text", clk + ": background", "%s while typing" % clk)
    for clk in ("punch clock", "selection clock"):
        add("text", clk + ": text", clk + ": background", "%s digits" % clk)
        add("large", clk + ": edited text", clk + ": background", "%s while typing (large clock font)" % clk)
    for clk in ("secondary delta clock", "transport delta clock"):
        add("text", clk + ": text", clk + ": background", "%s digits" % clk)
    add("large", "big clock active: text", "big clock active: background", "big clock while recording")
    add("text", "gtk_control_text2", "shuttle", "shuttle speed text on the shuttle marker (style shuttle_control, shuttle_control.cc)")
    # -- panners ('L'/'R' boxes, position text)
    add("text", "stereo panner text", "stereo panner fill", "stereo panner L/R letters")
    add("text", "stereo panner inverted text", "stereo panner inverted fill", "stereo panner L/R letters, inverted width")
    add("text", "mono panner text", ("over", "mono panner fill", "panner fill", "mono panner bg"), "mono panner L/R letters (fill drawn with the 'panner fill' alpha)")
    # -- markers: name flag text is contrasting_text_color() of the marker color
    for marker in ("location marker", "location cd marker", "location range", "location session",
                   "location loop", "location punch", "location arrangement marker", "entered marker"):
        add("text", ("autotext", marker), marker, "marker name flag text over '%s'" % marker)
    # -- regions: name text is contrasting_text_color() of the (alpha-composited)
    #    region fill; default regions use 'time axis view item base'
    region = ("over", "time axis view item base", "time axis view item base", "audio track base")
    add("text", ("autotext", "time axis view item base"), region, "region name text, default region over a lane")
    sel = ("mix", region, "selected region base", 0.6)
    add("text", ("autotext", "time axis view item base"), sel, "region name text, selected region")
    add("ui", "waveform fill", region, "waveform fill vs default region fill")
    add("ui", "play head", "ruler base", "playhead vs ruler")
    add("ui", "play head", "audio track base", "playhead vs track lane")
    add("ui", "punch line", "audio track base", "punch in/out line vs track lane")
    # -- buttons with a text label: the text color is picked by Ardour
    for name, where in TEXT_BUTTONS:
        for state in ("fill", "fill active"):
            fill = "%s: %s" % (name, state)
            if resolve(theme, fill) is None:
                continue
            add("text", ("btntext", fill), fill, "%s %s: %s" % (name, state, where))
    for name, where in ICON_BUTTONS:
        for state in ("fill", "fill active"):
            fill = "%s: %s" % (name, state)
            if resolve(theme, fill) is None:
                continue
            add("ui", ("btntext", fill, True), fill, "%s %s: %s" % (name, state, where))
    # -- LEDs (ArdourButton::Indicator) against the button's own fill
    for style, where in LED_BUTTONS:
        fill = style + ": fill"
        if resolve(theme, fill) is None:
            fill = "generic button: fill"
        add("ui", style + ": led active", fill, "LED of '%s': %s" % (style, where))
    # -- advisory
    add("info", "generic button: outline", "gtk_background", "button outline vs window (exempt: label/icon identifies the button)")
    add("info", "generic button: fill", "gtk_background", "button fill vs window (exempt: label/icon identifies the button)")
    add("info", ("shade", "gtk_background", 1.4), ("shade", "gtk_background", 0.7),
        "fader fill vs groove (shade() in clearlooks.rc.in 'gain_fader', not a theme color)")
    add("info", ("shade", "gtk_background", 0.7), "gtk_background", "fader groove vs window (same)")
    return R


def checks(theme):
    out = {}
    for fg, bg, label in TEXT_PAIRS:
        f, b = resolve(theme, fg), resolve(theme, bg)
        if f and b:
            out[label] = (ratio(f, b), TEXT_MIN)
    fg, bg = resolve(theme, "gtk_foreground"), resolve(theme, "gtk_background")
    for btn in BUTTONS:
        for state in ("fill", "fill active"):
            fill = resolve(theme, "%s: %s" % (btn, state))
            if fill is None:
                continue  # falls back to generic button colors at runtime
            out["%s (%s)" % (btn, state)] = (max(ratio(fg, fill), ratio(bg, fill)), UI_MIN)
    return out


def run_wcag(theme, name, strict):
    fails = infos = 0
    print("\nWCAG rules (%s%s)" % (name, "" if strict else ", report only"))
    print("%-6s %8s %5s  %s" % ("kind", "ratio", "min", "pair"))
    for level, fg, bg, minimum, label in build_rules(theme):
        f, b = ev(theme, fg), ev(theme, bg)
        if f is None or b is None:
            print("%-6s %8s %5s  %s (color not defined)" % (level, "-", "-", label))
            if strict and level != "info":
                fails += 1
            continue
        r = ratio(f, b)
        if level == "info":
            status = "info"
            infos += 1
        elif r + 1e-9 >= minimum:
            status = "ok"
        else:
            status = "FAIL" if strict else "warn"
            fails += 1
        print("%-6s %7.2f:1 %4.1f  %s  %s" % (level, r, minimum, label, "" if status in ("ok", "info") else "<-- " + status))
    print("%d WCAG %s, %d advisory pair(s)" % (fails, "failure(s)" if strict else "pair(s) below threshold (report only)", infos))
    return fails if strict else 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=os.path.join(os.path.dirname(__file__), "..", ".."))
    ap.add_argument("--theme", default="modern")
    ap.add_argument("--ref", default="dark")
    ap.add_argument("--set", action="append", default=[], metavar="NAME=RRGGBB",
                    help="what-if: override a palette color before checking (repeatable)")
    args = ap.parse_args()
    tdir = os.path.join(os.path.abspath(args.root), "gtk2_ardour", "themes")
    theme = load(os.path.join(tdir, "%s-ardour.colors" % args.theme))
    for item in args.set:
        name, _, value = item.rpartition("=")
        theme[0][name] = value + "ff"
    ref = load(os.path.join(tdir, "%s-ardour.colors" % args.ref))

    got, base = checks(theme), checks(ref)
    fails = warns = 0
    print("%-42s %8s %8s  min" % ("pair", args.theme, args.ref))
    for label, (r, need) in got.items():
        rr = base.get(label, (None,))[0]
        status = "ok"
        if r < need:
            if rr is not None and r + 0.05 < rr:
                status = "FAIL"
                fails += 1
            else:
                status = "warn"
                warns += 1
        print("%-42s %7.2f:1 %7s  %.1f  %s" % (label, r, ("%.2f:1" % rr) if rr else "-", need, status))
    print("\n%d failure(s), %d warning(s)" % (fails, warns))

    wcag_fails = run_wcag(theme, args.theme, strict=(args.theme == "modern"))
    return 1 if (fails or wcag_fails) else 0


if __name__ == "__main__":
    sys.exit(main())
