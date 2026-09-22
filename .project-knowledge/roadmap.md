# Roadmap

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-14
> Forward-looking only. TODO.md is the source of truth for its own backlog (stable §.n ids).

## Current Goal

Port Rovyl to Linux: X11 full experience, Wayland = hotkey-only fallback.

**Done (2026-09-14):**
- Phase 1 seams: `nativeHelperEnabled()` + platform-aware `getNativeHelperExePath()` (`rovyl-helper-linux`), Linux gates on blocker/trigger/FG-helper call sites, Linux `stealForegroundForMainWindow` (4-byte X11 window handle)
- Phase 2 helper: `backend/native-helper-linux/rovyl-helper.c` — X11 passive Sync grabs + XTest, same line protocol; builds via `npm run helper:build` (gcc, `-lX11 -lXtst`); FG mode live-tested
- Phase 3: `backend/linux-desktop.cjs` (`.desktop` scan/icon/launch/terminal) wired into `scanInstalledApps`, `extractIconUncached` (theme icons → data URL), launch ladder (`desktop:` ids → gio/gtk-launch; `exec_sh` for command lines), terminal detection + auto-commands, `get-onboarding-apps`/`get-startup-apps` discovery ("Found 5 apps" verified live)
- App boots and runs on Linux (electron 28, XWayland): main window up, helpers spawn, trigger armed, discovery populated config
- electron-builder `linux` target (deb+AppImage) added to package.json

**Remaining:**
1. Wayland helper mode (evdev grab + uinput) — **DONE 2026-09-14, live on the maintainer's Wayland session** (helper in `mouse-blocker-evdev` mode, trigger armed, forwarded pointer registered). Virtual-device E2E suite verified: click passthrough, hold, drag mid-hold injection, BLOCK swallow/forward by POS. Awaiting only human acceptance: hold MMB anywhere. Next: deb packaging (udev rule + input-group postinst), CI workflows
2. CI workflows per plan §5a; local `npm run dist`
3. ~~Electron upgrade~~ **DONE 2026-09-14: 28.3.3 → 44.3.0** — build clean, all 17 smoke tests + node tests pass, app boots on XWayland AND native Wayland (window, Alt+Z globalShortcut registration, trigger armed). Electron 44's globalShortcut.register is instant on Wayland (no portal hang)
4. Wayland launch recipe (verified): `ELECTRON_OZONE_PLATFORM_HINT=auto` + `ELECTRON_DISABLE_SANDBOX=1` (zen-kernel sandbox blocks GPU without it), and **never** `ZENITH_DISABLE_HARDWARE_ACCELERATION=1` on native Wayland — software rendering never paints the first frame of a transparent window, `ready-to-show` never fires, boot stalls silently. Diag warning added for that combination
5. `@electron/get` extract bug recurred on upgrade (fixed again by rerunning `node node_modules/electron/install.js`; if `dist/electron` missing, that's the workaround)

## Known Bugs

- See `TODO.md` open items (unchecked): §1.2 dead `fixedPosition` key, §1.4 offline claim, §1.5 cursor-follow architecture, §2.2–2.5 dead code/licensing decisions, plus §4+ sections
- (found 2026-09-14) `@electron/get` extract silently produced a broken `dist/` on this machine — manual unzip + `path.txt` (`electron`, no newline) fixed; watch for it after fresh `npm install`
- (found 2026-09-14) On Wayland sessions the XWayland gesture grab only sees X11 clients' pointer events; native Wayland windows never trigger and are immune to BLOCK — by design, documented fallback is hotkey

## Active TODOs

- [ ] Phase 1–5 of the Linux port (see Current Goal) — 1–3 + packaging config done *(added: 2026-09-14)*
- [ ] Wayland support plan researched and written: `docs/wayland-port-plan.md`. Key findings: gesture IS possible via evdev grab + uinput (input-remapper pattern, all compositors); requires Electron 28 → ≥41 upgrade first; KDE/wlroots near-full (layer-shell, foreign-toplevel, KWin-script steal, portal hotkey), GNOME honest-degraded. InputCapture portal ruled out (barrier-triggered). 7 phases in the doc, **Wayland is the primary path — maintainer has no X11** *(added: 2026-09-14)*
- [ ] CI workflows (plan §5a): `build-linux.yml` deb matrix (Debian 12/13, Ubuntu 22.04/24.04 containers) + AppImage + udev-rule assert; `tests.yml` smoke suite; `wayland-e2e.yml` headless sway + virtual uinput device driving a real gesture with `swaymsg`/`grim` assertions — replaces the impossible-on-this-machine X11 gesture test *(added: 2026-09-14)*

## Planned Features

- (none beyond the port)
