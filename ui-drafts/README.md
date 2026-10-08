# Ardour UI drafts

Design drafts for a cleaner, less confusing Ardour UI. They are mockups (HTML rendered to PNG),
not screenshots of working code. Sources live in `src/`. To regenerate:

```sh
PW_MOD=$(npm root -g)/playwright node src/render.js src/editor.html 02-editor-redesign.png 1680 1000 --clean
PW_MOD=$(npm root -g)/playwright node src/render.js src/editor.html 03-editor-redesign-annotated.png
PW_MOD=$(npm root -g)/playwright node src/render.js src/mixer.html 04-mixer-redesign.png
PW_MOD=$(npm root -g)/playwright node src/render.js "src/editor.html#add" 05-quick-add-track.png 1680 1000 --clean
PW_MOD=$(npm root -g)/playwright node src/render.js src/palette.html 06-color-palette.png 1680 660
PW_MOD=$(npm root -g)/playwright node src/render.js src/topbar.html 09-topbar-redesign.png 1680 575   # crops: src/topbar-*.png from screenshots/after/editor.png
PW_MOD=$(npm root -g)/playwright node src/render.js src/topbar-icons.html 10-topbar-icons.png 1680 860   # crop: src/topbar-icons-row.png = editor.png y 26..74
PW_MOD=$(npm root -g)/playwright node src/render.js src/midi.html 11-midi-editor.png 1680 2090   # crops: src/midi-*.png from screenshots/midi-before/
```

## Prior art (what others have tried)

| Project | What it is | Takeaway |
|---|---|---|
| Harrison **Mixbus** / **LiveTrax** | Commercial / GPL products built on Ardour's codebase | Mixbus mostly changes the *mixer* (console-style strips); the editor is largely Ardour's. LiveTrax is a stripped-down Ardour for live recording; it simplifies by *removing* features rather than restyling. |
| `friendsgamingyt611-sys/Ardour-Plus` | Attempted rewrite of the GUI in Qt6/QML | About 50 of about 250 UI components ported, in a single commit. It confirms a toolkit rewrite is a huge effort; not a practical path for us. |
| `eotter-beep/waveride` | Ardour fork | Small wording tweaks to file-resolution dialogs only. |
| Community themes (Catppuccin, Gruvbox, Monokai, Nord, base16, OpenColor) | `.colors` files | Theming covers colors only. The Ardour devs' own view is "you can change the colors, you can't change the visual appearance in any fundamental way" without code changes. |
| Discourse: *"I designed some fixes for a few UX interaction pain points"* | UX designer's proposal | Targets popups and nested menus for simple, repeated actions (e.g. the Add Track dialog). This inspired draft 05. |
| Discourse: *"Ardour's themes color definitions change proposal"* | Theme structure proposal | Aliases make themes hard to edit. Our palette-only approach sidesteps that. |

Nobody has published a fork that restyles or reorganizes the GTK UI itself, so these drafts aren't duplicating existing work.

## Drafts

- `02/03-editor-redesign` – editor layout (annotated copy numbers the 10 changes)
- `04-mixer-redesign` – mixer strips
- `05-quick-add-track` – inline add-track popover
- `06-color-palette` – current vs proposed palette
- `10-topbar-icons` – top-bar icons: play range, loop, auto return (playhead icon, options A/B), Lua example buttons; today vs proposed
- `11-midi-editor` – MIDI editor (bottom pane / Pianoroll window): toolbar, keyboard + range bar, velocity as brightness, velocity lane, Edit MIDI in window (Alt+P), beat grid in MIDI regions

## Feasibility (from reading the code)

| Change | Where | Effort |
|---|---|---|
| New palette | new `gtk2_ardour/themes/modern-ardour.colors` (draft exists; values only) | Small |
| Flat buttons default | `ui_config_vars.inc.h` (`flat-buttons` already exists) | Trivial |
| Fewer default rulers | ruler visibility defaults (already user-toggleable) | Small |
| Page switcher restyle | Record/Edit/Mix/Cue buttons already exist in `application_bar.cc` | Small–medium |
| Toolbar regrouping, single clock with tempo/meter | `application_bar.cc`, `editor_actions.cc` | Medium |
| Edit tools: icon + label on active tool | `ArdourButton` already supports icon+text | Medium |
| Track header layout (color stripe, R/M/S, inline fader) | `route_time_axis.cc`, `time_axis_view.cc` (inline gain slider already exists) | Medium–large |
| Inline "Add track" row + quick popover | new widget; "More options…" reuses `add_route_dialog.cc` | Medium |
| Sidebar horizontal tabs + filter | editor list (currently vertical tabs) | Medium |
| Status bar with plain-language info | status bar already exists in `ardour_ui.cc` | Small–medium |
| Mixer restyle (headers, plugin cards, routing pills) | `mixer_strip.cc`, `processor_box.cc`; IN/OUT buttons (`io_button.cc`) already exist | Medium–large |
| Font (Inter) | `ui-font-family` setting exists; bundling a font is extra work | Small–medium |

## Implementation status

Scope agreed: colors + layout, with Inter as the UI font.

| Draft item | Status | Notes |
|---|---|---|
| Modern palette as default theme | Done | `color-file` defaults to `modern`; selection uses one blue accent; selected page/tool/tab use a neutral highlight instead of green |
| Flat buttons | Done | `flat-buttons` defaults to on |
| Inter UI font | Done | Inter 4.0 (OFL) bundled in `gtk2_ardour/fonts`, registered on Linux/macOS, `clearlooks.inter.rc` style; falls back to the system sans font where Inter is unavailable |
| 1 Transport | Mostly existing | Ardour's transport already groups and color-codes these |
| 2 One clock | Done | secondary clock off by default (still available) |
| 3 Tools: label on active tool | Done | icon for all tools, name on the active one |
| 4 Labelled edit mode / snap | Done | "Edit: Slide"; Snap already had a label |
| 5 Page switcher | Done | single row: Record · Edit · Mix · Clips |
| 6 Fewer rulers | Done | default: one time ruler, sections, markers, loop/punch ranges |
| 7 Track header | Done | color stripe; P/A/G letters replaced by icons |
| 8 Inline "Add track" + quick popover | Done | full dialog still one click away ("More options…") |
| 9 Sidebar tabs | Already in Ardour 9 | horizontal tab buttons at the top of the editor list |
| 10 Plain-language status | Done | e.g. "DSP 12% · 3 xruns", "Record time left: 9 h" |
| Mixer: colored strip headers | Done | name button in track color |
| Mixer: quieter plugin colors | Done | pre/post-fader distinction kept, softened |
| Quick-add "Record from" input picker | Done | "Automatic" (auto-connect preference), "No input", or a hardware input (pairs for stereo); several audio tracks take consecutive inputs, MIDI tracks share the chosen device |
| Clips page: clip tiles tinted by clip color | Done | stronger tint while playing, readable white names |
| Clips page: play / queued / selected states | Done | outline in clip color / amber / white; green launch icon while playing; queued clips waiting behind a playing clip are outlined too |
| Track colors used widely | Done | distinct default palette; faders, mixer headers and Clips-page gain sliders in the track color; master stays neutral |
| Clips page: progress bar on playing clip | Done | thin bar along the bottom of the tile |
| Clips page: plain-language launch options | Done | "Play / Retrigger / Hold / Toggle / Repeat", "Start on", tooltips explain each |
| Clips page: follow actions as words | Partly | options read "Play next clip", "Play again", ...; the panel is relabelled ("When the clip ends", "Then act after"), not rebuilt as one sentence |
| Clips page: column headers in track color | Already in Ardour | |
| Clips page: named scenes, library BPM/length | Not done | scenes keep their letters; library unchanged |
| Top bar (draft 09): single row | Done | transport grouped (locate · stop/play/range/record · loop/click/auto return); tempo/meter beside the clock; Punch In/Out + record mode in one row; pane toggles in one row |
| Top bar: hide cryptic controls | Done | sync "Int.", shuttle, "VS" hidden by default (Preferences › Appearance › Application Bar › "Display Sync Source and Shuttle/Varispeed Controls"); MIDI panic only in the Transport menu; unassigned Lua slots hidden |
| Top bar: alerts only when active | Done | "Solo active" / "Auditioning" / "Feedback loop" appear only while relevant |
| Round 9: mixer plain language | Done | clearer tooltips (Iso/Lock, group, RTA, meter point, gain, peak); solo isolate/safe row and Comments hidden by default (a route with a comment still shows its button); "+ Add plugin" under every processor list; Mixer scenes: "Store current mix", left-click stores — see `screenshots/round9-before-after.png` |
| Round 9: editor affordances | Done | track-header faders drawn as a slider (groove, translucent track-color fill, handle line); "Rec: Layered" record-mode label; editor list headers Cue / On / In / RTA with plain tooltips |
| Round 9: command palette | Done | Help › Find Command… (Ctrl+Shift+P): search every action, shows group and shortcut, Enter runs it |
| Round 9: contrast QA | Done | `tools/ui-qa/check_contrast.py`: WCAG 4.5:1 text / 3:1 non-text for the modern theme (113 pairs); palette tweaks for ruler labels, active icons on green, red pills, panner letters |
| Round 6: menus and dialogs (from a tour of every menu and window) | Done | readable disabled items; visible empty checkboxes, plain check marks in menus; clipped time fields fixed; drop-downs in Preferences match their labels; highlighted main dialog action, no stock icons on dialog buttons; "Create" in New Session; plainer labels; View menu lists only the visible pages' pane toggles; common region actions at the top of the region menu; larger Locations window; selected regions keep their track tint; muted Recorder lanes; outline star for non-favorite plugins, Insert works without "Add" — see `screenshots/menus-dialogs-before-after.png` |

## Screenshots and handoff

- `screenshots/before/` – stock Ardour (this repo, before the redesign), captured with
  `tools/ui-qa/screenshots/shoot.sh . before`.
- `screenshots/after/` – the redesign build, same demo session (`editor`, `mixer`, `cues`,
  plus `cues-playing` / `cues-queued` clip states).
- `screenshots/before-after.png` – the three pages side by side.
- `screenshots/topbar-before-after.png` – the top bar before/after round 5.
- `screenshots/menus-dialogs-before-after.png` – menus and dialogs before/after round 6.
- `screenshots/round9-before-after.png` – mixer, editor, command palette, track fader (round 9); single shots in `screenshots/round9/`.

Rebuild `before-after.png` after refreshing `screenshots/after/`:

```sh
cd ui-drafts/screenshots && montage -background '#111215' -fill '#b6bbc4' -pointsize 16 \
  -label 'Before: editor' before/editor.png -label 'After: editor' after/editor.png \
  -label 'Before: mixer' before/mixer.png -label 'After: mixer' after/mixer.png \
  -label 'Before: Clips' before/cues.png -label 'After: Clips' after/cues.png \
  -tile 2x3 -geometry 840x525+0+0 -depth 8 before-after.png
```
- `HANDOFF.md` – status, decisions, next steps for continuing in a new session.
