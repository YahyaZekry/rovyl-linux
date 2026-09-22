# Session Log

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-14

| Date | Summary |
|------|---------|
| 2026-09-14 | Scanned codebase, mapped every Windows touchpoint in `electron-main.js` (~55-65% coupled), produced Linux port plan (5 phases), created `.project-knowledge/` |
| 2026-09-14 | Linux port phases 1–3 + packaging config: platform seams, `rovyl-helper-linux` (C, X11 grabs + XTest, same protocol), `linux-desktop.cjs` (discovery/icons/launch/terminal), app boots on Linux with helpers + trigger armed + discovery verified; all smoke tests pass; deb/AppImage target added. Remaining: real-gesture E2E on X11, dist run, Wayland fallback messaging |
| 2026-09-14 | Wayland research (Exa) → `docs/wayland-port-plan.md` incl. CI/DE-test workflows; Wayland made primary (no X11 machine). Electron 28→44.3.0 upgraded; diagnosed + fixed the native-Wayland boot stall (software rendering kills first paint of transparent windows — ready-to-show never fires); app now boots fully natively on Wayland (window, shortcut, trigger armed) |
