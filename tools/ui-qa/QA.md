# UI redesign QA

How we check that the UI redesign (theme, Inter font, layout changes, Quick Add,
cue page) doesn't break anything.

## What upstream Ardour does

There is no published QA process. In practice:

- **Unit tests** exist for the libraries only (`libs/ardour/test`, `libs/pbd/test`,
  `libs/temporal/test`, `libs/evoral/test`, `libs/midi++2/test`,
  `libs/audiographer/tests`): sessions, regions, playlists, tempo math, Lua,
  plugins. Built with `./waf configure --test`, run with `gtk2_ardour/artest`
  (or `./waf test`). They are off in the nightly builds.
- **No automated GUI tests.** `gtk2_ardour` has only manual harnesses
  (`toolbar_test.cc`, `--canvasui`) for eyeballing widgets.
- **Nightly builds** (nightly.ardour.org) of git HEAD, plus a nightly static
  analysis (scan-build) run.
- **Pre-releases** (`-preN`) before major versions, tested by the community.

Our changes are almost entirely in `gtk2_ardour` (plus one file in
`libs/widgets`), i.e. exactly where upstream has no tests, so we add our own.

## Automated checks (`tools/ui-qa/`)

| Script | Needs a build | What it checks |
|---|---|---|
| `run_static.sh` | no | runs the two checks below |
| `check_themes.py` | no | every theme parses; no duplicate names; every alias points at a real palette color; `modern` and `dark` define every alias/modifier of the reference theme; **every color name the C++ code asks for literally is defined** |
| `check_contrast.py` | no | WCAG contrast of `modern`. Part 1: text and button pairs; fails only if below threshold *and* worse than `dark`. Part 2 (**WCAG rules**, see below): ~110 curated pairs that must pass for `modern` |
| `smoke.sh <tree> <out>` | yes | headless launch (Xvfb); creates a session from `qa_session.lua` (import, bus, every edit tool, every page, Quick Add), types a name into Quick Add, saves; then `check_session.py` asserts: imported track, bus, Quick Add track (armed), default rulers, no "Color … not found", no CRITICAL, rc file found |
| `smoke.sh <tree> <out> compat <session>` | yes | opens an existing session, e.g. one saved by stock Ardour, or ours in stock Ardour |
| `visual_diff.sh <base> <new>` | – | pixel diff of screenshots against approved baselines |
| `check_i18n.py` | no | user-visible strings *added on this branch* are wrapped in `_()`/`S_()`/`P_()` (or deliberately `X_()`) |
| `check_screenshot_colors.py <png>…` | – | every large area of a screenshot is a `modern` palette color (or a declared-alpha composite over a background), and none looks like the old `dark` theme – i.e. the theme reaches every widget. Verified to fail on stock screenshots. |
| `check_cue_states.py <shots-dir>` | – | the Clips page shows clip states: green launch icons while scene B plays, an amber outline on **every** queued clip of scene D (also on tracks whose current clip is still playing), no amber when nothing is queued. Counts theme-colored pixels in the cue grid of `cues*.png` from `shoot.sh`. Verified to fail on the round-3 shots (transport stopped by an extra Space; queued outline missing behind playing clips). |
| `screenshots/shoot.sh <tree> <label>` | yes | editor/mixer/cue page screenshots of a reproducible demo session (5 stems, a bus, 19 clips); `UI_SCALE=150` for HiDPI |
| `quick_add_input.sh <tree> <out>` | yes | Quick Add "Record from": picks Stereo + "Input 3 + 4", adds a track, asserts it is connected to exactly `system:capture_3/4`; also screenshots the "Solo active" top-bar pill (`solo.png`) |
| `run_gui.sh [tree] [out]` | yes | smoke test + Quick Add input picker + screenshots + color check + 150 % screenshots |

Run before every push that touches `gtk2_ardour`:

```sh
tools/ui-qa/run_static.sh          # seconds, no build
./waf && tools/ui-qa/run_gui.sh    # ~7 minutes, headless
```

### WCAG rules in `check_contrast.py`

The WCAG rules are built by `build_rules()`: a curated list of (foreground, background)
pairs, each with a label saying where it is used in the C++ code. They are blocking
for the `modern` theme and only reported for any other (`--theme dark`).

| kind | needs | used for |
|------|-------|----------|
| `text` | 4.5:1 | ruler labels, track/strip names, clocks, panner L/R, marker flag names, region names, button labels |
| `large` | 3:1 | clock digits while being edited (large clock font) |
| `ui` | 3:1 | icons on active buttons, LEDs on their button, playhead/punch line vs lane, waveform vs region |
| `info` | – | printed only (exemptions below) |

The pairs follow what the code really draws, not just the palette: button text and
vector-icon colors are chosen the way `ArdourButton::get_contrasting_color` does (better
of `gtk_foreground`/`gtk_background`, with the 0.35 bias for icons); marker and region
names use `contrasting_text_color` (near-white below luminance 0.5, else black);
translucent fills are composited over what is below them with the theme's own
`<Modifier>` alpha. Try a color change without editing the theme:
`check_contrast.py --set alert:green=5ed07b --set "alert:red=f1545e"`.

Exemptions (kept out of the blocking list on purpose):

- insensitive / disabled / bypassed text and widgets (WCAG exempts inactive components);
- text over colors the *user* picks (track, region and marker colors): Ardour chooses black
  or white itself, so the theme cannot fix a mid-tone track color;
- button outline and button fill vs window (`info`, 1.4:1 / 1.3:1): every button is identified
  by its label or icon, whose contrast *is* checked; WCAG 1.4.11 only asks for what is needed
  to identify a control;
- gain fader groove/handle (`info`): drawn with `shade()` of `gtk_background` in
  `clearlooks.rc.in` (`gain_fader`), not from a theme color, so a theme edit cannot change it.
  Fixing it needs a change to that style or to `ArdourFader`;
- decorative lines: grid, separators, borders, frames.

When you add a color name that carries text or an icon, add its pair to `build_rules()`.
If you change a palette color, re-run `check_contrast.py` – shared colors (`alert:green`,
`alert:red`, `alert:ruddy`, `neutral:midground`, selection blue) appear in many pairs.

Things learned while building the GUI harness (Xvfb, no window manager):
the Audio/MIDI dialog needs Return even with Autostart; window titles gain a
`*` when the session is modified; Alt+E is *Export* (no editor shortcut); Lua
`print()` doesn't reach stdout in the GUI; `nil` route groups in Lua segfault;
RulerVisibility is only saved after the user changes a ruler.

Library regressions (not expected from UI work, but cheap insurance before a
release):

```sh
./waf configure --test ... && ./waf && gtk2_ardour/artest
```

## Manual checklist

Things the scripts can't cover here (no real audio hardware, Linux only):

- [ ] **Existing users:** with an existing `~/.config/ardour9/ui_config`, the old
      theme, font and button style are kept (new defaults only apply to fresh configs).
- [ ] **Switching themes** in Preferences > Appearance > Colors: every bundled theme
      still looks right (new UI elements fall back to generic button colors).
- [ ] **HiDPI:** Preferences > Appearance > GUI scaling at 150 % and 200 %: tool labels,
      color stripes, Quick Add popover, cue tiles, progress bar.
- [ ] **Light theme** (e.g. `captain_light`): Quick Add, cue tiles, track headers readable.
- [ ] **Quick Add:** Audio mono/stereo, MIDI with and without instrument, Bus,
      count > 1, Escape closes, clicking elsewhere closes, instrument drop-down
      doesn't close it, "More options…" opens the full dialog.
- [ ] **Edit tools:** keyboard shortcuts (G, R, D, E, T, C, Y) update the label;
      toolbar width changes don't push other controls off-screen on a 1280-px window.
- [ ] **Cue page while playing:** tile tint/outline for playing, queued (next bar),
      selected; progress bar moves; follow actions fire; recording into a slot still blinks.
- [ ] **macOS and Windows:** Inter is registered (macOS) / falls back cleanly
      (Windows); no "Cannot find Inter" spam in the log of a bundled build.
- [ ] **Translations:** a language with long words (e.g. German) doesn't truncate
      the new labels ("Edit: …", "Record time left", follow actions).
- [ ] **Plugin GUIs** are unaffected (they don't use Ardour's theme).
