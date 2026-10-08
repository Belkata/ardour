# UI redesign – handoff

State of the Ardour UI redesign, so a
new session can continue without re-deriving anything. See `CLAUDE.md` for how
to build, run headless and test.

**Status: merged.** Everything below is in `master`:

| PR | Commit | Content |
|---|---|---|
| [#1](https://github.com/Belkata/ardour/pull/1) | `9afe9e82` | the redesign, rounds 1–4 |
| [#2](https://github.com/Belkata/ardour/pull/2) | `ea1f697` | this handoff |
| [#3](https://github.com/Belkata/ardour/pull/3) | `79d6274` | round 5: top bar in one row, Quick Add "Record from" |
| [#4](https://github.com/Belkata/ardour/pull/4) | `b53c75f` | handoff after round 5; Quick Add input picker QA script |
| [#5](https://github.com/Belkata/ardour/pull/5) | (squash) | round 6: menus, dialogs and windows; this handoff update |
| [#6](https://github.com/Belkata/ardour/pull/6) | `b9ae27d2` | round 7: top-bar icons |
| — | `63a377ec` | round 8: MIDI editor (merge of `claude/midi-editor`) |

It sits on top of upstream `608f15a4` (the fork point); `git diff 608f15a4 master`
shows the whole redesign.

Git setup for follow-up work:

- Start each piece of work on a fresh branch from `master` (the session gives the
  branch name). Don't reuse the old `claude/wizardly-brahmagupta-jyh5oi` history:
  its 14 commits were squashed, so git doesn't see them as merged.
- Open a PR and merge it only when the user asks. Squash-merging through the GitHub
  tools worked; pushing straight to `master` isn't possible from a session.
- Tag `fork-point` (on `608f15a4`) is **not on GitHub yet**: cloud sessions can't
  push tags (the server hangs up). The user pushes it from their own clone:
  `git tag -a fork-point 608f15a4 -m "..." && git push origin fork-point`.
- Upstream Ardour: `git remote add upstream https://github.com/Ardour/ardour.git`;
  `git diff upstream/master...master` shows this fork's changes (see `CLAUDE.md`).

## Goal and decisions (from the user)

- "Ardour's UI is ugly and confusing – fix it up", after researching forks.
- Workflow agreed: **drafts as images first**, then implement.
- Scope decided by the user: **colors + layout**, and the font **as in the drafts
  (Inter)**. The cue page redesign was requested later and is in scope too.
- The user wants **visible progress for long builds in the background-task window**,
  without the agent being pinged: run `./waf build -j4 2>&1 | tee <log>` as the
  background task itself (see `CLAUDE.md`); no Monitor.
- The user wants QA so nothing breaks (see `tools/ui-qa/QA.md`).

## Research summary (forks / prior art)

- No published fork restyles or reorganizes Ardour's GTK UI.
  - Harrison **Mixbus** (commercial; different mixer, same editor) and **LiveTrax** (Harrison,
    GPL, stripped-down live-recording Ardour) – simplify by removing features.
  - `friendsgamingyt611-sys/Ardour-Plus` – attempted Qt6/QML rewrite, ~50 of ~250 components,
    one commit. Toolkit rewrite judged unrealistic.
  - `eotter-beep/waveride` – only dialog wording.
  - Community themes (Catppuccin, Gruvbox, Monokai, Nord, base16, OpenColor) – colors only.
- Ardour devs: "you can change the colors, you can't change the visual appearance in
  any fundamental way" (without code); GTK3 rejected (plugin GUIs).
- Useful UX input: Discourse thread "I designed some fixes for a few UX interaction
  pain points" (heavy Add Track dialog → our Quick Add) and the theme color-definitions
  proposal (aliases are hard to edit).
- QA: upstream has only library unit tests (`libs/*/test`, `gtk2_ardour/artest`), nightly
  builds, nightly scan-build, community-tested `-preN` releases. No GUI tests.

## Drafts (ui-drafts/)

`02/03` editor (clean/annotated, 10 numbered changes), `04` mixer, `05` quick add track,
`06` palette (current vs proposed), `07/08` Clips/cue page. Sources in `ui-drafts/src/*.html`,
render with `src/render.js` (Playwright + preinstalled Chromium; commands in `README.md`).
`README.md` has the research table, feasibility table and the **implementation status table**
(keep it updated).

## What is implemented (all in `master`)

### Rounds 1–4 (PR #1)

| Area | Files |
|---|---|
| Defaults: `modern` theme, flat buttons, Inter font, secondary clock off | `gtk2_ardour/ui_config_vars.inc.h` |
| Inter 4.0 (OFL) bundled + registered | `gtk2_ardour/fonts/`, `bundle_env_linux.cc`, `bundle_env_cocoa.cc`, `wscript` (install + `clearlooks.inter.rc`), `tools/*_packaging/*` |
| rc-file fallback to `clearlooks.rc` | `ui_config.cc` |
| Theme: palette, blue selection accent, neutral "selected" state, softer plugin colors, `primary button` / `add track row button` styles | `themes/modern-ardour.colors` |
| Page switcher in one row: Record · Edit · Mix · Clips | `ardour_ui2.cc`, `ardour_ui.cc` (labels) |
| Active edit tool shows its name | `editing_context.{h,cc}` (`update_mouse_mode_button_labels`), `editor_mouse.cc` |
| "Edit: Slide" edit-mode selector | `editor.cc`, `editor_actions.cc` |
| Shorter default ruler set | `editor_actions.cc`, `editor_rulers.cc` |
| Track header color stripe; P/A/G → icons | `route_time_axis.{h,cc}`, `vca_time_axis.cc`, new icons `TrackPlaylist/TrackAutomation/TrackGroup` in `libs/widgets/ardour_icon.{h,cc}` |
| "+ Add Track" row + Quick Add popover; action `Editor/quick-add-track` in Track menu | `quick_add_route.{h,cc}` (new, in `wscript`), `editor.{h,cc}`, `editor_actions.cc`, `ardour.menus.in` |
| Plain-language status bar | `ardour_ui.cc` (`update_cpu_load`, `format_disk_space_label`, `update_sample_rate`) |
| Mixer strip name in track color | `mixer_strip.cc` |
| Clips page tiles: clip-color tint, playing/queued/selected outline, progress bar, green play icon | `triggerbox_ui.{h,cc}` |
| Clips page plain-language labels/tooltips | `trigger_ui.cc`, `slot_properties_box.cc` |
| QA tools | `tools/ui-qa/` |

### Round 5 (PR #3)

The user found the top bar "confusing and a bit messy"; draft 09 was approved as drawn
("implement it as drafted"), then the user asked for the Quick Add input picker.

| Area | Files |
|---|---|
| Top bar draft `09-topbar-redesign.png` | `ui-drafts/src/topbar.html` |
| Top bar in one row: transport grouped (locate · stop/play/range/record · loop/click), auto return beside it, tempo/meter stacked next to the clock, Punch In/Out + record mode in one row, pane toggles in one row | `application_bar.{h,cc}`, `transport_control_ui.cc`, `libs/widgets/tabbable.cc` |
| Sync/shuttle/varispeed hidden by default: new pref `show-toolbar-shuttle` (Appearance › Application Bar) | `ui_config_vars.inc.h`, `rc_option_editor.cc`, `application_bar.cc` |
| MIDI panic no longer on the bar (Transport menu only, also affects the Big Transport window) | `transport_control_ui.cc` |
| Solo / Audition / Feedback alerts shown only while active ("Solo active", "Auditioning", "Feedback loop" / "No alignment") | `application_bar.cc` (`update_alert_visibility`) |
| Only assigned Lua action buttons shown (still limited by `action-table-columns`) | `application_bar.cc` (`update_action_script_visibility`) |
| Quick Add "Record from" picker: Automatic / No input / hardware input (pairs for stereo, "Input 3 + 4"); several audio tracks take consecutive inputs, MIDI tracks share the device; passes `input_auto_connect=false` when an input is chosen | `quick_add_route.{h,cc}` |
| QA: `quick_add_input.sh` (picker connects exactly the chosen inputs; solo pill screenshot), added to `run_gui.sh` | `tools/ui-qa/` |

### Round 7 (PR #6, merged)

The user found the top-bar icons confusing and asked for more standard ones. Draft
`10-topbar-icons.png` (approved; auto return option **B** chosen: playhead + arrow +
faded playhead where playback stopped). Result: `screenshots/topbar-icons-before-after.png`.

| Area | Files |
|---|---|
| Flat 24-unit-grid icons: go to start/end, play range (▶ over a bracket), loop (repeat symbol), auto return (playhead) | `libs/widgets/ardour_icon.cc` |
| New `ArdourIcon::Refresh` (old circular arrow) for the clip picker's refresh button, which used `TransportLoop` | `ardour_icon.h`, `trigger_clip_picker.cc` |
| Fresh configs no longer pre-assign the "Mixer Screenshot" / "List Plugins" Lua buttons | `luainstance.{h,cc}` |
| Tooltip shortcuts fall back to global bindings (a toolbar inside a page only searched the page's bindings, so auto return "7", punch in/out had no shortcut line); clearer auto return tooltip on the action | `libs/gtkmm2ext/gtk_ui.cc`, `ardour_ui_ed.cc` |

Environment notes from this round (Debian trixie container, 2 vCPU, no root):
user-space Xvfb/xdotool/Chromium/ImageMagick in `~/.cache/xroot` (`. ~/.cache/xroot/env.sh`;
`Xvfb` must be the patched `Xvfb-user`, symlinked from `~/.local/bin/Xvfb`), `python`
symlink in `~/.local/bin`, missing jpeg/curl dev headers in `~/.cache/devroot` (configure with
`PKG_CONFIG_PATH`, `CFLAGS`/`CXXFLAGS=-I…`, `LINKFLAGS=-L…`), Inter copied to
`~/.local/share/fonts`. Full `-j2` build: 1 h 10 min. Button text renders slightly smaller
here than in the round-6 screenshots (different system libraries), so compare icons, not text size.

### Round 8 (merged into `master` as `63a377ec`)

The user asked to rework the MIDI editor (bottom pane / Pianoroll window); review in
`screenshots/midi-before/`, draft `11-midi-editor.png` ("go ahead in this order": 1–4),
result `screenshots/midi-before-after.png` and `screenshots/midi-after/`. Demo session:
`tools/ui-qa/screenshots/make_demo_midi.py` + `midi_demo_session.lua` (4 imported MIDI tracks).

| Area | Files |
|---|---|
| 1 Toolbar in words ("Tools", "Grid", "Note length", "Velocity", "Channel", "Drums", "Lanes: Velocity", "Colors: Track", "View" menu with zoom focus + editing policy); active tool shows its name; region list only with >1 region | `pianoroll.{h,cc}`, `cue_editor.cc`, `editing_context.cc` (labels shared with the main editor's Draw mode) |
| 1 Toolbar row never forces the window wider (`ToolbarClip`: requests 1 px width, clips) — fixed the window growing past the screen when the MIDI tools panel opens | `cue_editor.cc` |
| 2 Keyboard: note names on the keys; the "scroomer" (scroll+zoom over 0–127, formerly a wide name column with a red handle) is a 10 px range bar; header 140 → 54 px (100 px while lanes are shown) | `prh_base.{h,cc}`, `prh.cc`, `piano_roll_header.cc`, `themes/modern-ardour.colors` |
| 3 Velocity = brightness of an opaque note color (60–100 %); "note bars for velocity" off by default; selected notes 2 px outline | `note_base.{h,cc}`, `note.{h,cc}`, `hit.{h,cc}`, `ui_config_vars.inc.h` |
| 3 Velocity lane: 127/64/0 scale, chord notes side by side, one lane = ¼ height | `pianoroll.{h,cc}`, `velocity_display.cc` |
| 4 Region › "Edit MIDI in Window…" (Alt+P), top of a MIDI region's context menu; the window zooms to the region after it has its size | `editor_actions.cc`, `editor_selection.cc`, `editor.cc`, `ardour.keys.in`, `pianoroll_window.{h,cc}` |

Not done / known: beat grid inside MIDI regions in the editor (needs new per-track
canvas drawing; editor grid lines are global and follow the grid setting). Ardour shares
a region's saved zoom (samples per pixel) between all its editors, so after using the
Pianoroll window the bottom pane can restore a zoom meant for another width (upstream
behaviour). The Alt+P shortcut works but isn't shown next to the context-menu item.
Testing tips: drive the app on `:96` with `xdotool`; `ArdourButton::set_icon (NoIcon)`
also removes the text element (use `set_icon (0, 0)`).

### Round 9 (pushed to `master` as `99791618`)

After a UX best-practice review (Nielsen heuristics, progressive disclosure, WCAG contrast)
the user said "go ahead with what you think are the prios" and asked for **parallel Sonnet 5.5
workers**. Setup: one git worktree per task, workers run headless via
`/work/tools/run-worker.sh` (`claude -p --model claude-sonnet-5-5`), never build, only
syntax-check single files with `/work/tools/check-cc.sh` (flock-serialized; 2 vCPU / 3 GB);
the orchestrator merges, builds once (`-j2`, 24 min) and runs QA. Shared rules:
`/work/tools/AGENT_BRIEF.md`. (These live outside the repo, in `/work/tools`.)
Result: `screenshots/round9-before-after.png`. `run_static.sh` and `run_gui.sh` pass.

| Area | Files |
|---|---|
| Mixer strip tooltips (Iso/Lock, group, RTA, meter point, gain, peak); Comments button forced visible when a comment exists | `mixer_strip.{h,cc}`, `gain_meter.cc` |
| Default strip visibility without `SoloIsoLock,Comments` (fresh configs) | `ui_config_vars.inc.h` |
| "+ Add plugin" hint under the processor list (same path as double-click) | `processor_box.{h,cc}` |
| Mixer scenes: "Mixer scenes" label + tooltip, "Store current mix" slot, left-click on a free slot stores | `mixer_ui.cc` |
| Track-header fader as slider: new `ArdourFader::TrackHeaderStyle` tweak (horizontal only) | `libs/widgets/ardour_fader.cc`, `fader_widget.{h,cc}`, `gain_meter.{h,cc}`, `route_time_axis.cc` |
| Editor list headers Cue/On/In/RTA + plain tooltips | `route_list_base.cc`, `editor_routes.cc` |
| "Rec: Layered" record-mode selector + tooltip | `application_bar.cc` |
| Command palette `Common/command-palette`, Help menu, Ctrl+Shift+P | `command_palette.{h,cc}` (new), `ardour_ui_ed.cc`, `ardour.menus.in`, `ardour.keys.in`, `wscript` |
| WCAG contrast check + palette fixes | `tools/ui-qa/check_contrast.py`, `QA.md`, `themes/modern-ardour.colors` |

Not done / ideas: dB value beside the header fader (no room at 14 px); input button's bare
number "1" (`io_button.cc`); meter-point/RTA row is not a visibility group yet; bottom status
bar; region cards; mixer plugin cards / send bars. Advisory contrast pairs (stock fader
fill/groove, button outlines) need rc/drawing changes, not theme colors.
QA gotcha: source `~/.cache/xroot/env.sh` first, then put `~/.local/bin` first in PATH,
otherwise `run_gui.sh` starts the unpatched Xvfb and every check fails ("cannot open display").

### Round 10 (branch `claude/round10`, pushed to `master`)

Same worker setup as round 9 (four Sonnet 5.5 workers). The region-cards worker hit the
account's spend limit mid-task; its partial version moved the name bar to the *top* of the
region, which shifts waveforms, fades, xfades, gain lines, sync marks and MIDI notes (and
changed `MidiRegionView::drag_group`). That was dropped as too risky: the bar stays at the
bottom (Ardour's existing name-highlight layout), only restyled.
Result: `screenshots/round10-before-after.png`. `run_static.sh` and `run_gui.sh` pass.

| Area | Files |
|---|---|
| Bottom status bar + health pills (DSP / xruns / disk), `reset_health_counters()` | `ardour_ui_ed.cc`, `ardour_ui.cc`, `ardour_ui.h` |
| Region cards: opaque name bar, contrasting text, rounded border in region color; `show-name-highlight` default on | `time_axis_view_item.cc`, `ui_config_vars.inc.h` |
| Processor rows: corner radius 6, dim "fader" row (+ flat `processor fader: fill`), sends "→ name", 5 px send-level bar | `processor_box.cc`, `modern-ardour.colors` |
| I/O button labels "IN In 1+2" / "OUT Master" / "No input" (also I/O plugin buttons) | `io_button.cc` |
| Meter point + RTA in their own row, visibility id `MeterPoint` (not in the default list → hidden) | `mixer_strip.{h,cc}`, `rc_option_editor.cc` |
| Marker ruler `< + >` buttons blank until hover | `editor.cc` |

Follow-ups: editor list (right sidebar) now opens narrower, so In/R/RS columns are scrolled
off; "DSP  9%" has a padded number; green tint for send rows (new `processor send` style);
the status bar's left labels still say "Record time left:", "I/O Latency:", "PDC:".

## Current state / where it stopped

A working prototype, merged. Iteration loop (draft → approve → implement → build →
QA → screenshot → fix), rounds 1–6, all merged. Round 6 builds; `run_static.sh` and
`run_gui.sh` pass (smoke 10/10, picker, colors, clip states, 150 %). Screenshots:
`screenshots/after/`, `screenshots/before-after.png`, `screenshots/topbar-before-after.png`,
`screenshots/menus-dialogs-before-after.png`.

Round 6 (PR #5): fixes from a tour
of every menu and dialog, sheet in `screenshots/menus-dialogs-before-after.png`.
`run_static.sh` and `run_gui.sh` pass. Details worth knowing:

- Widget drawing lives in `libs/clearlooks-newer` (a GTK engine, shared by all themes):
  checkbox/radio borders mix in the text color; menus draw only a check mark (no box);
  the "etched" copy of insensitive text is skipped on dark backgrounds.
  `fg[INSENSITIVE]` in `clearlooks.rc.in` is a mix of fg and bg.
- **Fonts:** rc `font_name` reaches labels and named widgets, but drop-downs
  (`GtkComboBox` → `GtkCellView`), tree views and unnamed custom widgets end up with
  the toolkit default "Sans 10" (larger than the UI's Inter 8). A rule like
  `widget "*OptionsNotebook*"` does *not* fix the combos. What works: set the cell
  renderers' `font_desc` (see `match_combo_fonts()` in `option_editor.cc`, the plugin list
  in `plugin_selector.cc`). Setting `gtk-font-name` globally would also work but
  shrinks many redesigned buttons, so it was not done.
- `AudioClock` measures its size before realize with a stand-in label's font; it now
  asks for a relayout in `on_realize()` when the real font gives a different size
  (that was the clipped "0:00:00:0" in Session Properties / Locations).
- Dialog buttons: `dialog_buttons.{h,cc}` — `plain_buttons()` (drops stock icons, called
  from `ArdourDialog::on_show` and `ArdourMessageDialog::run/show`) and `set_primary()`
  (rc style `primary_dialog_button`, accent fill).
- View menu: `update_view_menu_pane_items()` in `ardour_ui_ed.cc` hides the pane toggles
  of pages that are not on screen when the menu opens (actions and shortcuts unchanged).
- Selected regions: background blended 60 % towards `selected region base`, waveform
  keeps its track tint (`audio_region_view.cc`, `midi_region_view.cc`,
  `time_axis_view_item.cc`).
- Not done: Preferences sidebar is still larger than the page text (reads as navigation);
  other tree views (editor list, etc.) still use the default font.

Round 5 details worth knowing:

- Top bar = `ApplicationBar` (`application_bar.cc`), one instance per page (editor, mixer,
  recorder, cue page) — layout is a one-row `Gtk::Table`; optional groups are
  `no_show_all` and toggled in `repack_transport_hbox()` from `show-toolbar-*` prefs.
  The page switcher and pane toggles live in the `Tabbable` header (`ardour_ui2.cc`,
  `libs/widgets/tabbable.cc`), not in the bar.
- Alerts: `blink_handler()` → `update_alert_visibility()` shows/hides the pills. The solo
  pill is red (theme's `rude solo`); the draft showed it green — the user wasn't asked
  yet whether to change it (one line in `modern-ardour.colors`).
- Lua buttons: unassigned slots are hidden, so "right-click an empty slot to assign" is
  gone; assigning goes through Edit › Lua Scripts › Script Manager.
- Changing a default in `ui_config_vars.inc.h` affects fresh configs only.
- GTK menu labels built from port names must escape `_` (mnemonics), see `refill_inputs()`.
- Pretty port names come from `AudioEngine::get_pretty_name_by_name()`; the Dummy backend
  has none, so the fallback turns `system:capture_3` into "Input 3".

Rounds 1–4:

- Everything builds; `tools/ui-qa/run_static.sh` and `tools/ui-qa/run_gui.sh` pass
  (smoke 10/10, theme colors in screenshots, clip states, 150 % HiDPI).
- Compatibility: a session saved by stock Ardour opens in the redesign build.
- Round 1: track-colored clip tiles, font lookup in the dev tree, editor defaults, color QA.
- Round 2: faders tinted with the track color, taller clip tiles (22 px), more QA.
- Round 3: master/monitor neutral (tracks get the distinct hues); screenshots of
  playing/queued clips.
- Round 4: queued outline also on clips waiting behind a playing clip
  (`TriggerBox::peek_next_trigger`); `check_cue_states.py`; `run_gui.sh` no longer
  swallows the screenshot color check's exit status; "+ Add Track" button at natural
  width; editor list Name column min 90 px; quieter "Show Sends" fill; cue-page gain
  sliders in the track color.

Harness lessons: after choosing from an `ArdourDropdown` menu the keyboard focus stays on
the drop-down — click the target entry before typing. Launching a cue (F1–F8) starts the transport by itself — pressing Space
afterwards stops it. Queued clips start at the next bar (< 2 s at 120 bpm), so capture
the queued state immediately.

## Next steps (the user picks what to close next)

1. Ask the user which remaining draft gaps to close (they choose; last time they chose
   their own topic, the top bar). Open questions from round 5: solo pill red vs green;
   whether to merge the editor tool row into the top bar. Not implemented yet:
   - mixer plugin "cards", send bars, fader restyle (`processor_box.cc`, `mixer_strip.cc`)
   - region drawing ("region cards")
   - follow actions as one sentence; named scenes; clip library BPM/length
   - bottom status bar
   - editor tool row (Edit mode, tools, Snap) merged into the top bar as in draft 02 (not requested yet)
   - Windows Inter registration (`bundle_env_mingw.cc`; falls back to the system font)
   - left over from round 6: Preferences sidebar text larger than the page text; other
     tree views (editor list, etc.) still in the toolkit default font (see the round 6
     font notes above for the fix pattern)
2. For any change: build, `tools/ui-qa/run_static.sh`, `tools/ui-qa/run_gui.sh`, and
   compare screenshots with `ui-drafts/screenshots/after/` (`visual_diff.sh`). Refresh
   the after shots and `before-after.png` when the look changes.
3. Keep `ui-drafts/README.md` (status table) and this file up to date.

## Expectation set with the user

The real build will match the drafts' colors, font, page switcher, tool labels, rulers,
color stripes and headers, but **not** the drafts' spacing, region cards or mixer strip
internals (estimate: editor 60–70 %, mixer ~40 % of the mockup). Present the gaps honestly
with side-by-side images and let the user choose what to close next.

## Environment notes (cloud container)

- apt downloads stall; use `tools/ui-qa/install_deps.sh`.
- If `git describe` names `fork-point` instead of `9.0-…`, the build fails in
  `set_version`; make the local `fork-point` tag lightweight (`git tag -f fork-point 608f15a4`)
  so `git describe` (annotated tags only) finds `9.0`.
- No git tags in the shallow clone; configure needs `git describe` → local annotated tag
  (see `CLAUDE.md`), never pushed.
- Don't use `pkill -f <pattern>`/`rm` with globs in compound shell commands: `pkill -f`
  matched the agent's own shell, and a safety check blocks `cd … && rm …*`.
- Full build ≈ 34–37 min; header changes rebuild most of `gtk2_ardour`. Editing
  sources while a build runs is fine (waf hashes each file when it reaches it), but run
  `./waf build` once more afterwards and compare object vs source mtimes if unsure.
- Session setup that worked in round 5, in order: `sudo tools/ui-qa/install_deps.sh`
  (~3 min), local `9.0` tag, `./waf configure ...` (see `CLAUDE.md`), then the build as a
  background task with `tee` (user wants to see progress).
- `before-after.png` is a `montage` of before/after editor, mixer, Clips (see README).
- GitHub: no CI runs on this repo; PRs are squash-merged with the GitHub MCP tools
  after `run_static.sh` + `run_gui.sh` pass locally. After a merge, reset the session
  branch to `origin/master` and push it with `--force-with-lease` (it only held merged
  history), otherwise the stop hook reports unpushed commits.
