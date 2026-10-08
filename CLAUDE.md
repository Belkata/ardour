# CLAUDE.md

Fork of the Ardour DAW (`belkata/ardour`). Current work: a UI redesign
(colors + layout + Inter font). **Read `ui-drafts/HANDOFF.md` first** — it has
the status, decisions and next steps.

## Layout

- `gtk2_ardour/` – the GUI (GTK2 via the bundled `ytk`/`ytkmm` toolkit). Almost all UI work happens here.
- `gtk2_ardour/themes/*-ardour.colors` – color themes: a palette (`<Colors>`), names mapped onto it (`<ColorAliases>`), alpha `<Modifiers>`.
- `gtk2_ardour/ui_config_vars.inc.h` – GUI preference defaults (theme, font, flat buttons, …).
- `gtk2_ardour/clearlooks.rc.in` – GTK widget style template (fonts/padding for dialogs, lists, menus).
- `libs/widgets/` – Ardour's own widgets (`ArdourButton`, `ArdourIcon` vector icons).
- `libs/ardour/` – engine/session model. `libs/canvas/` – the editor/cue canvas.
- `ui-drafts/` – redesign mockups (HTML → PNG), status table, before/after screenshots.
- `tools/ui-qa/` – QA scripts for the redesign (see below).

## Fork point / diff against upstream

The redesign starts from upstream commit `608f15a4` (tag `fork-point`). After merging
into `master`, see our changes with:

```sh
git remote add upstream https://github.com/Ardour/ardour.git   # once
git fetch upstream master
git diff upstream/master...master   # only this fork's changes, also after syncing upstream
git diff fork-point master          # vs the original fork point
```

Use a tag, not a branch, for the fork point (branches move). `check_i18n.py` picks its
base the same way (upstream merge base → `fork-point` → `608f15a4`).

## Build (Linux, this cloud container)

```sh
sudo tools/ui-qa/install_deps.sh        # apt is unreliable here; this works around it
git tag -a 9.0 -m local <oldest-commit>  # ONLY if `git describe` fails (shallow clone, no tags):
                                         # configure parses the version from it. Never push this tag.
./waf configure --optimize --no-phone-home --with-backends=dummy,alsa --no-lrdf --no-vst3 --no-lxvst
./waf build -j4                          # full build ≈ 40 min on 4 cores
```

- Touching widely-included headers (`editor.h`, `editing_context.h`, `ui_config_vars.inc.h`,
  `libs/widgets/widgets/ardour_icon.h`) recompiles most of `gtk2_ardour` (~30–40 min).
  Prefer `.cc`-only changes when iterating.
- New source files must be added to the list in `gtk2_ardour/wscript`.
- Long builds: run `./waf build -j4 2>&1 | tee <log>` itself as the *background task*
  (not redirected to a file only), so the user sees the live `[n/total] Compiling ...`
  lines in the background-task window. Don't use Monitor for build progress: it wakes
  the agent on every line and expires after 5 minutes. You get one notification when
  the task exits; check errors with `grep error: <log>`.
- Run from the build tree: `gtk2_ardour/ardev` (sets up paths for `build/`).

## Run headless

There is no sound card and no window manager in the container:

- Use a fresh config with `tools/ui-qa/preseed_config.sh <dir>` + `XDG_CONFIG_HOME=<dir>`
  (skips the first-run wizard, selects the **Dummy** backend, suppresses the memlock dialog).
- The Audio/MIDI Setup dialog still appears; focus it and press Return (Start).
- Find windows with `xdotool search --name`, focus with `xdotool windowfocus --sync`
  before sending keys (`getactivewindow` doesn't work without a WM).
- `--template` takes the *name* of a Lua `SessionInit` script found in the script
  path (e.g. `$XDG_CONFIG_HOME/ardour9/scripts/`), not a file path.
- In Lua, pass `ARDOUR.RouteGroup ()` instead of `nil` for route-group arguments —
  `nil` segfaults Ardour.
- Shortcuts: Alt+M mixer, Alt+C cue page, Alt+R recorder, Ctrl+S save. **Alt+E is "Export"**,
  not the editor (there is no show-editor shortcut). A modified session's window title
  gains a `*` prefix (`*name - Ardour`).

## QA (tools/ui-qa, see QA.md)

```sh
tools/ui-qa/run_static.sh                      # theme integrity + contrast, no build needed
tools/ui-qa/smoke.sh . /tmp/ui-qa              # headless GUI smoke test (needs a build)
tools/ui-qa/smoke.sh . /tmp/c compat <x.ardour># open an existing session
tools/ui-qa/screenshots/shoot.sh . after       # editor/mixer/cues screenshots
tools/ui-qa/visual_diff.sh <base-dir> <new-dir>
```

Upstream Ardour only has library unit tests (`./waf configure --test`, `gtk2_ardour/artest`);
there are no upstream GUI tests.

## Conventions

- Match surrounding code: tabs, `space before (`, Ardour's comment style, `_("...")` for
  user-visible strings, `X_("...")` for non-translated ones.
- Theme changes: only edit `modern-ardour.colors` for the redesign; keep other themes
  working (unknown button style names fall back to `generic button` colors).
  Run `tools/ui-qa/check_themes.py` after any theme or color-name change.
- Changing defaults in `ui_config_vars.inc.h` only affects fresh configs; existing users
  keep their saved preferences.
- Keep display-string changes display-only: session files store enums, never labels.
- Commit messages end with the attribution lines given by the session; never include
  model identifiers in commits or code.
- Work on the designated `claude/...` branch; don't open PRs unless asked.
