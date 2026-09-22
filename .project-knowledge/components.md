# Component Inventory

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-14

## src/components/

- `RadialMenu.tsx` — the wheel: slices, aiming (`resolveAimAtPoint`), hub, dwell engine, workspace switching
- `RadialHud.tsx` — weather/battery/clock HUD (orphaned, TODO §2.3)
- `PrecisionSettings.tsx` — settings panel (lazy chunk)
- `FirstRun.tsx` — onboarding
- `IconPicker.tsx` — lucide icon + custom icon picker (lazy, full icon set chunk)
- `installedApps.tsx` — installed-app scanner UI (lazy)
- `WheelPreview.tsx` — workspace preview
- `SmartIcon.tsx` — resolves icon refs (custom/rovyl-icon://, lucide, favicon)
- `selectMenu.ts` — custom dropdown matching the panel
- `PanelTransition.tsx`, `ErrorOverlays.tsx` — lazy framer-motion isolates
- `ErrorBoundary.tsx`, `RovylLogo.tsx`

## Dead (TODO §2.2, do not add to)

- ~~`SettingsModal.tsx`~~ (deleted), `WelcomeScreen.tsx`, `SystemCenter.tsx`, `AppSelector.tsx`
