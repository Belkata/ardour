# Ardour UI drafts

Design drafts for a cleaner, less confusing Ardour UI. They are mockups (HTML rendered to PNG),
not screenshots of working code. Sources live in `src/`. To regenerate:

```sh
PW_MOD=$(npm root -g)/playwright node src/render.js src/editor.html 02-editor-redesign.png 1680 1000 --clean
PW_MOD=$(npm root -g)/playwright node src/render.js src/editor.html 03-editor-redesign-annotated.png
PW_MOD=$(npm root -g)/playwright node src/render.js src/mixer.html 04-mixer-redesign.png
PW_MOD=$(npm root -g)/playwright node src/render.js "src/editor.html#add" 05-quick-add-track.png 1680 1000 --clean
PW_MOD=$(npm root -g)/playwright node src/render.js src/palette.html 06-color-palette.png 1680 660
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
