# Features & Workflows

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-14

## Features

- **Hold-to-open wheel** — hold trigger (MMB/side button/global hotkey), wheel blooms centre-screen; click-vs-hold discrimination passes short presses through as normal clicks (docs/ARCHITECTURE.md "Mouse trigger")
- **Workspaces** — separate wheels, switch with number keys / scroll on hub
- **Two aiming modes** — `angle` (direction from anywhere) and `cursor` (precision, pointer under icon); confirmation always resolves from the live pointer
- **Launch without clicking (dwell)** — `radialInstantActivate: 'dwell'` launches after holding aim; 5-rule engine in `RadialMenu.tsx` (see history)
- **App discovery / onboarding** — reads Start Menu apps with real icons into first-run wheel
- **Shortcut types** — apps, folders, files, URLs (favicon via web), custom commands, terminal-wrapped commands, IDE MRU files (VS Code family `state.vscdb`)
- **Settings panel** — `PrecisionSettings.tsx`: trigger, monitors, aiming, appearance, i18n, backups (import/export with inlined icons)
- **Focus protection** — won't open over fullscreen games (game-detection)
- **Tray** — pause/resume, workspaces, settings, quit; autostart toggle
- **Licensing** — Google OAuth, gate screen without license (vestigial per TODO §2.4)

## Workflows

**Wheel open (hold mode)**
1. Helper emits `TRIGGER_DOWN` → main starts hold timer
2. Past delay: `BLOCK` region + window sized to monitor → handshake → reveal → bloom
3. Main polls cursor 8 ms → IPC `mmb-cursor` → renderer synthesises `mousemove`
4. Release: `TRIGGER_HOLD` → confirm slice → launch → `UNBLOCK` + hide

**Icon pipeline**
1. Discovery resolves target → `extract-icon.ps1`/`getFileIcon` → bytes
2. `icon-store.cjs` stores `icons/<sha256>.<ext>` → `rovyl-icon://` ref in config
