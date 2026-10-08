# UI redesign – handoff

State of the Ardour UI redesign, so a
new session can continue without re-deriving anything. See `CLAUDE.md` for how
to build, run headless and test.

**Status: merged.** The redesign was squash-merged into `master` as `9afe9e82`
([Belkata/ardour#1](https://github.com/Belkata/ardour/pull/1)), on top of upstream
`608f15a4` (the fork point). `git diff 608f15a4 master` shows the whole redesign.

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

## What is implemented (all in `master`, commit 9afe9e82)

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

### Round 5 (branch `ccr-d562df24-x2zqd5`, not merged yet)

| Area | Files |
|---|---|
| Top bar draft `09-topbar-redesign.png` | `ui-drafts/src/topbar.html` |
| Top bar in one row: transport grouped (locate · stop/play/range/record · loop/click), auto return beside it, tempo/meter stacked next to the clock, Punch In/Out + record mode in one row, pane toggles in one row | `application_bar.{h,cc}`, `transport_control_ui.cc`, `libs/widgets/tabbable.cc` |
| Sync/shuttle/varispeed hidden by default: new pref `show-toolbar-shuttle` (Appearance › Application Bar) | `ui_config_vars.inc.h`, `rc_option_editor.cc`, `application_bar.cc` |
| MIDI panic no longer on the bar (Transport menu only, also affects the Big Transport window) | `transport_control_ui.cc` |
| Solo / Audition / Feedback alerts shown only while active ("Solo active", "Auditioning", "Feedback loop" / "No alignment") | `application_bar.cc` (`update_alert_visibility`) |
| Only assigned Lua action buttons shown (still limited by `action-table-columns`) | `application_bar.cc` (`update_action_script_visibility`) |
| Quick Add "Record from" picker: Automatic / No input / hardware input (pairs for stereo, "Input 3 + 4"); several audio tracks take consecutive inputs, MIDI tracks share the device; passes `input_auto_connect=false` when an input is chosen | `quick_add_route.{h,cc}` |

## Current state / where it stopped

Round 5 (top bar + Quick Add "Record from") builds; `run_static.sh` and `run_gui.sh` pass.
Picker verified headless: choosing "Input 3 + 4" connects the new track to exactly
`system:capture_3/4`. Screenshots: `screenshots/after/`, `screenshots/topbar-before-after.png`.
Menu labels from port names must escape `_` (GTK mnemonics) — see `refill_inputs`.

A working prototype, merged. Iteration loop (screenshot → QA → fix → rebuild), rounds 1–4:

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

Harness lessons: launching a cue (F1–F8) starts the transport by itself — pressing Space
afterwards stops it. Queued clips start at the next bar (< 2 s at 120 bpm), so capture
the queued state immediately.

## Next steps (the user picks what to close next)

1. Ask the user which remaining draft gaps to close. Not implemented yet:
   - mixer plugin "cards", send bars, fader restyle (`processor_box.cc`, `mixer_strip.cc`)
   - region drawing ("region cards")
   - follow actions as one sentence; named scenes; clip library BPM/length
   - bottom status bar
   - editor tool row (Edit mode, tools, Snap) merged into the top bar as in draft 02 (not requested yet)
   - Windows Inter registration (`bundle_env_mingw.cc`; falls back to the system font)
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
- No git tags in the shallow clone; configure needs `git describe` → local annotated tag
  (see `CLAUDE.md`), never pushed.
- Don't use `pkill -f <pattern>`/`rm` with globs in compound shell commands: `pkill -f`
  matched the agent's own shell, and a safety check blocks `cd … && rm …*`.
- Full build ≈ 37 min; header changes rebuild most of `gtk2_ardour`.
