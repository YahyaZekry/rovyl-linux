# Features & Workflows

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-21

## Features

- **Hold-to-open wheel** — hold trigger (MMB/side button/global hotkey), wheel blooms centre-screen; click-vs-hold discrimination passes short presses through as normal clicks (docs/ARCHITECTURE.md "Mouse trigger")
- **Workspaces** — separate wheels, switch with number keys / scroll on hub
- **Two aiming modes** — `angle` (direction from anywhere) and `cursor` (precision, pointer under icon); confirmation always resolves from the live pointer
- **Launch without clicking (dwell)** — `radialInstantActivate: 'dwell'` launches after holding aim; 5-rule engine in `RadialMenu.tsx` (see history)
- **App discovery / onboarding** — reads Start Menu apps with real icons into first-run wheel
- **Shortcut types** — apps, folders, files, URLs (favicon via web), custom commands, terminal-wrapped commands, IDE MRU files (VS Code family `state.vscdb`)
- **Settings panel** — `PrecisionSettings.tsx`: trigger, monitors, aiming, appearance, i18n, backups (import/export with inlined icons)
- **Focus protection** — won't open over fullscreen games (game-detection)
- **Middle-click calibration** — Settings → Mouse (click mode) → "Calibrate from your own clicks": two-sided, 3 fast presses (tab close) then 2 slower presses (menu intent); the threshold is the midpoint between slowest-fast and fastest-slow (fallback slowest + 150 ms). Main streams every press duration, the renderer owns the phases, and a steps modal shows progress. Applied live (save path re-arms the helper TRIGGER). Bands: < threshold pure native click, threshold…1 s menu (held back, CLICK_CONSUMED cancels), > 1 s autoscroll handover
- **Double middle click opens the menu** (optional, click mode) — helper TRIGGER carries dblMs=300: a single fast click is held back through the window then lands natively (~300 ms late); a second press inside the window cancels it and emits `TRIGGER_DOUBLE`, the second gesture is swallowed natively, and main opens the menu. Off by default
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
