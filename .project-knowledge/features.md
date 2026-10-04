# Features & Workflows

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-30

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
- **Double middle click opens the menu** (optional, click mode) — helper TRIGGER carries dblMs=300: a single fast click is held back through the window then lands natively (~300 ms late); a second press inside the window cancels it and emits `TRIGGER_DOUBLE`, the second gesture is swallowed natively, and main opens the menu. Off by default. While armed, the threshold slider and the calibration row hide (a single press can't open the menu, so the threshold is untestable)
- **System dock on Linux** — status service (`backend/system-status.cjs`) has a Linux backend: wpctl (pactl fallback) for volume/mute, `/sys/class/power_supply` for battery, `/sys/class/net` + `/proc/net/wireless` for network/signal; same STATUS contract + 1s cadence as the Windows helper. Panel clicks open the desktop's own settings (XDG_CURRENT_DESKTOP → KDE/GNOME/Cinnamon kcms, standalone fallbacks)
- **Panel clearance** — `dockPanelClearance` (0–120px slider, Settings → Appearance → Position): lifts both docks off their screen edge; on Wayland the compositor reports no panel struts, so the user states the thickness. Shortcut dock only draws with items in it (`items.length > 0`)
- **Root wheel mode** (Workspaces, 2+ workspaces) — `workspaceRootMode`: **Launcher** (upstream's home launcher: root shows every workspace, aim to slide in) or **Slide** (root opens on the current workspace's shortcuts; hub-scroll cycles). `getRootRadialApps` + `rootIsPicker` + the scroll guard all honor it; workspace keys and hub-back work in both shapes
- **v1.16 upstream features** — recorded mouse triggers (shared `mouse-trigger.cjs`; modifier combos are Windows-only on Linux — our TRIGGER carries menuMin+dblMs instead of modMask), recorded workspace keys (`workspaceKeyBindings` replaced the old picker/keys toggle), drag-any-file-to-workspace (`inspect-drop-paths`), area-wedges targeting (`'angle'` mode removed, configs rewritten on read), settings-corner gear, GitHub row (repointed to YahyaZekry/rovyl-linux)
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
