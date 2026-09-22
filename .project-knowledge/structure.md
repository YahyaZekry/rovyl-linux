# Project Structure

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-14

## File Tree (key paths)

```
backend/                  Electron main process + platform helpers
  electron-main.js              9,767 lines: windows, IPC, gesture, discovery, icons, licensing, updates
  electron-preload.js           the entire renderer-facing API surface (platform-neutral)
  native-helper/rovyl-helper.cs C# source of rovyl-helper.exe (mouse blocker + foreground focus)
  mouse-blocker.ps1             PowerShell fallback: WH_MOUSE_LL hook
  foreground-focus.ps1 / get-foreground-exe.ps1 / extract-icon.ps1 / taskbar-control.ps1 /
  simulate-keys.ps1 / find_lnks.ps1
  win32-launch.js               command parsing/quoting for launching (pure string logic)
  linux-desktop.cjs             Linux desktop integration: .desktop scan, theme icons, gio/gtk-launch, terminal handling
  native-helper-linux/rovyl-helper.c   C source of rovyl-helper-linux (X11 grabs + XTest, same line protocol as the C# helper)
  rovyl-helper-linux            built helper (checked in, mirrors rovyl-helper.exe convention)
  game-detection.cjs, foreground-snapshot.cjs, fullscreen-bounds.cjs, taskbar-overlay.cjs,
  tray-menu.cjs, icon-store.cjs, page-title.cjs, persistence-normalize.cjs   (pure Node, testable anywhere)
  rovyl-helper.exe              built helper (checked in)
src/                      Renderer (React)
  App.tsx                       orchestration: state, persistence, IPC, window modes
  components/RadialMenu.tsx     the wheel: layout, aiming, gestures, slices, dwell engine
  components/PrecisionSettings.tsx  settings panel
  components/RadialHud.tsx, FirstRun.tsx, IconPicker.tsx, installedApps.tsx, SmartIcon.tsx,
  WheelPreview.tsx, PanelTransition.tsx, ErrorOverlays.tsx, ErrorBoundary.tsx, selectMenu.ts
  hooks/useIconHealing.ts, utils/* (workspaceRadial, taskbarOverlay, windowsLaunchCommand, …)
  i18n/ (languages, translations 10 langs, useTranslation), strings.ts (6 live wheel strings)
  iconMap.ts (curated + lazy lucide set), iconRef.ts, persistenceMirror.ts, siteFavicon.ts, types.ts, defaults.ts
scripts/                  build, launch, verify, ~25 smoke tests, release
nsis/installer.nsh        installer customisation
docs/ARCHITECTURE.md      non-obvious design notes — read before touching main-process behaviour
docs/wayland-port-plan.md Wayland research + phased plan + CI/DE-test workflow design
TODO.md                   improvement backlog with stable §.n ids (source of truth for known issues)
```

## Key Files

| File | Purpose |
|------|---------|
| `backend/electron-main.js` | Everything main-process: window lifecycle, helper spawn/protocol, discovery, icons, launch ladder, licensing, updater |
| `backend/electron-preload.js` | IPC surface only, zero platform coupling |
| `backend/native-helper/rovyl-helper.cs` | Native gesture engine: line protocol `BLOCK/TRIGGER/RECORD/SHORTCUT_TRIGGER/WARP/EXIT` → `TRIGGER_DOWN/UP/HOLD`, `FG|bounds|exe|title` |
| `src/components/RadialMenu.tsx` | The wheel and the entire dwell/instant-activate engine |
| `docs/ARCHITECTURE.md` | Why the windowing handshake and trigger hook are shaped the way they are |
| `TODO.md` | Backlog with stable ids (`§.n`); open items are the known-bugs list |
