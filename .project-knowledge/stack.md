# Stack

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-14

## Tech Stack

| Category | Details |
|----------|---------|
| Language | JavaScript (main process, CJS) + TypeScript (renderer) |
| Runtime | Node ≥ 20, Electron 44.3.0 (upgraded from 28 on 2026-09-14 for native Wayland) |
| Framework | React 18 + Vite 8 (rolldown) + @vitejs/plugin-react |
| Styling | Tailwind CSS 4 + hand-written tokens in `src/index.css` |
| State Mgmt | Plain React state in `App.tsx` (see TODO §3.10) |
| Key runtime deps | `electron-updater`, `node-global-key-listener`, `sql.js` (only reads VS Code `state.vscdb`) |
| Native layer (Windows) | `rovyl-helper.exe` (C#, source `backend/native-helper/rovyl-helper.cs`) + PowerShell scripts |
| Icons/fonts | lucide-react (curated static set + lazy full set), @fontsource-variable (dev-only) |
| Packaging | electron-builder → NSIS (win), AppX optional; `build.files` excludes node_modules from explicit list but packs prod closure |
| Testing | ~25 smoke scripts (`npm run test:*`), pure-JS, runnable off-Windows except `test:trigger-gesture` (PowerShell) and `test:win32-launch` |

## Dev Commands

| Command | What It Does |
|---------|-------------|
| `npm run dev` | Vite dev server (renderer only) |
| `npm run electron` | Launch Electron against dev server (`scripts/launch-electron.js`) |
| `npm run start` | Packaged-style prod run (`scripts/start-prod.cjs`) |
| `npm run build` | helper build + tsc + vite build + verify-radial-windowing + verify-renderer-budget + win icon/appx assets |
| `npm run dist` | build + electron-builder (NSIS) |
| `npm run helper:build` | Build native helper (`scripts/build-native-helper.cjs`) |
| `npm run test:*` | Smoke tests (see package.json; most are pure JS) |
| `npm run verify:radial-windowing` / `verify:renderer-budget` | Build guards for the reveal handshake and bundle size |

## Environment Variables

| Variable | Used In | What It Enables |
|----------|---------|----------------|
| `GOOGLE_CLIENT_ID` / `GOOGLE_WEB_CLIENT_ID` / `GOOGLE_CLIENT_SECRET` | main process (`.env`/`.env.local` in project root) | Google OAuth for licensing |
| `ZENITH_AGGRESSIVE_GPU` | main process | Aggressive GPU flags (perf debugging) |
| `ZENITH_DISABLE_HARDWARE_ACCELERATION` | main process | Disable GPU accel |
| `ZENITH_MOUSE_HOOK_DELAY_MS` | main process | Delay before installing the mouse hook |
