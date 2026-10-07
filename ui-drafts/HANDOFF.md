# UI redesign – handoff

State of the Ardour UI redesign at the end of the first working session, so a
new session can continue without re-deriving anything. See `CLAUDE.md` for how
to build, run headless and test.

Branch: `claude/wizardly-brahmagupta-jyh5oi` (push only there; no PR opened).

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

## What is implemented (commits b7e48fc9, 9f09a753)

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

## Current state / where it stopped

- Stock baseline built and screenshotted: `ui-drafts/screenshots/before/{editor,mixer,cues}.png`.
- The redesign changes were applied and an incremental `./waf build` was **running**
  (≈85 %, no compile errors yet) when the session ended. Except `quick_add_route.cc`,
  **none of the redesign C++ has been confirmed to compile.** First thing to do:
  `./waf build -j4` and fix errors.
- `check_themes.py` and `check_contrast.py` pass (3 pre-existing warnings in upstream
  themes blueberry_milk, captain_light, diehard3 – not ours).

## Next steps

1. Finish the build; fix compile errors (watch especially `quick_add_route.cc` API use,
   `triggerbox_ui.cc` outline members, `editing_context.cc` labels).
2. `tools/ui-qa/screenshots/shoot.sh . after` → compare with `ui-drafts/screenshots/before/`
   and the drafts; send the user before/after/draft side by side. Commit the after shots to
   `ui-drafts/screenshots/after/`.
3. `tools/ui-qa/smoke.sh . /tmp/ui-qa` and fix failures. Check: Quick Add creates and arms
   "QA Track", rulers default, no "Color … not found", no CRITICAL.
   (Rec-enable may not be stored in the session file under `Controllable name="rec-enable"` –
   verify and adjust `check_session.py` if needed.)
4. Compatibility: open the `before` session in the new build and a new session in stock
   (`smoke.sh … compat <session>`).
5. Visual tuning after seeing real screenshots (expected gaps): toolbar density (still
   Ardour's controls), 4 px color stripe, Inter at Ardour's small default sizes (font scale),
   tool label width changes shifting the toolbar.
6. Remaining draft items not implemented: mixer plugin "cards"/send bars/fader restyle,
   region drawing, follow actions as one sentence, named scenes, clip library BPM/length,
   Quick Add "Record from" input picker, bottom status bar. Windows Inter registration
   (`bundle_env_mingw.cc`) not done (falls back to system font).
7. When stable: update `ui-drafts/README.md` status table, remove "WIP" in a final commit
   message, ask the user before opening a PR.

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
