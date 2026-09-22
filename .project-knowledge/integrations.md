# External Integrations & Data Contracts

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-14

## License API (`rovyl-red.vercel.app`)

- Google OAuth identity → license check/activation via IPC in main
- TODO §1.4: contradicts the "fully offline" README claim — known issue

## Favicons

- `src/siteFavicon.ts`: `unavatar.io` and `google.com/s2/favicons` for web shortcuts (network on config load)

## Weather HUD

- `RadialMenu.tsx` fetches `wttr.in` (orphaned feature per TODO §2.3)

## Updater

- electron-updater → GitHub releases `arshit09/rovyl`; NSIS feed on Windows; Linux needs AppImage/deb feed decision

## VS Code family `state.vscdb`

- Read-only via `sql.js` (WASM). Key: `history.recentlyOpenedPathsList`. Paths: `%APPDATA%\<IDE>\User` on Windows → `~/.config/<IDE>/User` on Linux

## Native helper protocol (in-process contract, not external)

- Spawned with parent PID; exits when parent dies
- Exact line grammar in `systems.md`; any Linux helper MUST reproduce it byte-for-byte (JS side parses with simple split/trim)
