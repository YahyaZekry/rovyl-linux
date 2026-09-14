# Session Log

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-14

| Date | Summary |
|------|---------|
| 2026-09-14 | Scanned codebase, mapped every Windows touchpoint in `electron-main.js` (~55-65% coupled), produced Linux port plan (5 phases), created `.project-knowledge/` |
| 2026-09-14 | Linux port phases 1–3 + packaging config: platform seams, `rovyl-helper-linux` (C, X11 grabs + XTest, same protocol), `linux-desktop.cjs` (discovery/icons/launch/terminal), app boots on Linux with helpers + trigger armed + discovery verified; all smoke tests pass; deb/AppImage target added. Remaining: real-gesture E2E on X11, dist run, Wayland fallback messaging |
| 2026-09-14 | Wayland research (Exa) → `docs/wayland-port-plan.md` incl. CI/DE-test workflows; Wayland made primary (no X11 machine). Electron 28→44.3.0 upgraded; diagnosed + fixed the native-Wayland boot stall (software rendering kills first paint of transparent windows — ready-to-show never fires); app now boots fully natively on Wayland (window, shortcut, trigger armed) |
| 2026-09-14 | Wayland gesture shipped: `mouse-blocker-evdev` mode in the C helper (evdev `EVIOCGRAB` + uinput reinject, capabilities/identity copied), `POS x y` protocol extension (main feeds cursor position while BLOCK is live — evdev has no absolute position), clickless capture gated off on Wayland (no warp). Found + fixed a 9-token BLOCK parse bug that broke BLOCK in the X11 mode too. Full virtual-device E2E suite green; app runs on Wayland with the helper live |
| 2026-09-14 | First real-grab attempt froze the pointer (REL forwarded as EV_KEY); fixed, added REL assertions to the E2E suite, app relaunched on Wayland with the corrected helper — gesture live for acceptance testing |
| 2026-09-14 | UX fixes from live testing: BLOCK_CLICK (click-away closes wheel/panel, via helper→main→renderer), unified the click/hold boundary at 1 s (helper threshold was 400 ms so slower menu clicks landed in apps). FirstRun overlay pinned to viewport. All verified with the virtual-device rig |
| 2026-09-14 | Helper-respawn self-healing added after my test cleanup killed the live helper (menu dead, log silent). Exit handler → requestBlockerRespawn → re-arm; kill-and-respawn verified live |
