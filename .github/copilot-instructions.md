## Overview
- Clock & calendar homebrew for Nintendo DS built with devkitARM/libnds; all runtime entry is in `source/main.c`.
- Top screen uses a framebuffer in VRAM bank A for analog clock & calendar; bottom screen switches between console UI and bitmap split-mode.
- Rendering + layout logic lives entirely in C under `source/`; assets or emulator configs are outside this folder and shouldn't be edited for logic changes.

## Build & Run
- Use the Nix shell (`nix develop`) to get devkitARM, desmume, and melonDS with `DEVKITARM` exported automatically.
- Primary build outputs `clock_app.nds`; run `make` inside the shell or invoke `./run-melon.sh` to build and boot melonDS.
- `./run.sh` still references `dsi-app.nds`; either update that script or launch DeSmuME manually: `make && nix run nixpkgs#desmume -- clock_app.nds`.
- CI packaging in `flake.nix` sets `TARGET=dsi-app`; keep this in mind if you rename outputs or add new derivations.

## Runtime flow
- `main` sets video modes, initializes consoles, caches theme/config structs, then enters the libnds event loop.
- Inputs are read once per frame with `scanKeys()`; toggles update state flags (`theme`, `rotation`, `split_mode`) and force redraw by resetting `last_second`.
- `configure_layout` centralizes the clock/calendar geometry for split vs combined layouts—extend this when adding new screen modes.
- Drawing is throttled to once per second via `last_second`; if you need higher-frequency updates, revisit this guard to avoid redundant full-screen fills.
- Split mode reallocates the sub-screen as a 16-bit bitmap background; remember to call `bgUpdate()` after drawing to the bottom buffer.

## Graphics system
- `GraphicsContext` carries a framebuffer pointer, rotation, logical size, and pivot; call `gfx_init` whenever the target buffer or transform changes.
- All primitives route through `gfx_plot`, which applies rotation math and bounds-checks for a 256×192 screen; adjust those constants if you ever target other surfaces.
- `gfx_clear` temporarily disables rotation to fill rectangles—avoid bypassing it, because direct loops must account for rotation and pivot manually.
- Thick lines are approximated by orthogonal offsets; when drawing new shapes, prefer composing from `gfx_draw_*` helpers instead of bespoke loops.
- Bottom-screen drawing passes a NULL framebuffer when the text console is active; gate any rendering on `ctx->framebuffer` to avoid crashes.

## Clock module
- `clock_init_config` + `clock_init_light/dark_theme` set defaults; new styles should extend these functions to keep theme switching consistent.
- `clock_draw_face` clears only the clock bounding box before rendering numbers/markers, so keep any new decorations within that square or expand the cleared region.
- `clock_draw_hands` recomputes cos/sin every redraw; if you add animations, reuse these helpers to keep rotation-aware plotting correct.

## Calendar module
- Calendar layout is grid-based with 13px cells; adjust `cell_width/height` in `calendar_init_config` if you redesign spacing.
- Day header colors come from the clock theme to stay in sync; when adding new palettes, thread them through `calendar_init_*_theme`.
- `calendar_draw` computes month metadata via Zeller’s congruence; re-use those helpers (`get_days_in_month`, `get_first_day_of_month`) when extending to different calendars.

## Conventions & tips
- Source is plain C99 with libnds types; stick to `<nds.h>` utilities and avoid heap allocations—the current code is entirely stack-based.
- Respect VRAM bank assignments: top screen uses bank A framebuffer, bottom bitmap uses bank C background slot 2.
- `build/` artifacts are generated; do not check in edits there—focus changes under `source/` and scripts.
- When introducing new input mappings, add instructions in `print_instructions` so the bottom console reflects the feature.
- Test on hardware/emulator after modifying rendering; many issues won’t appear until `bgUpdate()` or VRAM banks are misconfigured.
