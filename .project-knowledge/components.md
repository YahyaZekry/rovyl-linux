# Component Inventory

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-11-11

## src/components/

- `RadialMenu.tsx` — the wheel: slices, aiming (`resolveAimAtPoint`), hub, dwell engine, workspace switching
- `RadialHud.tsx` — weather/battery/clock HUD (orphaned, TODO §2.3)
- `SoundTryWheel.tsx` — interactive sound practice wheel, keyboard and pointer input; shares voices/highlight rules with RadialMenu
- `PrecisionSettings.tsx` — optional Sound section, clickable rows and stable scrollbar space; settings panel (lazy chunk); Linux trigger selection offers Wheel / Mouse 4 / Mouse 5; Windows retains the recorder
- `FirstRun.tsx` — onboarding
- `IconPicker.tsx` — lucide icon + custom icon picker (lazy, full icon set chunk)
- `installedApps.tsx` — installed-app scanner UI (lazy)
- `WheelPreview.tsx` — workspace preview
- `SmartIcon.tsx` — resolves icon refs (custom/rovyl-icon://, lucide, favicon)
- `selectMenu.ts` — custom dropdown matching the panel
- `PanelTransition.tsx`, `ErrorOverlays.tsx` — lazy framer-motion isolates
- `ErrorBoundary.tsx`, `RovylLogo.tsx`

## Removed (absence verified 2026-11-10)

- ~~`SettingsModal.tsx`, `WelcomeScreen.tsx`, `SystemCenter.tsx`, `AppSelector.tsx`~~ — already absent; the old TODO §2.2 deletion request is stale.
