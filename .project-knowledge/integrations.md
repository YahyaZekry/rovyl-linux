# External Integrations & Data Contracts

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-11-10

## License API (`rovyl-red.vercel.app`)

- Google OAuth identity → license check/activation via IPC in main
- TODO §1.4: contradicts the "fully offline" README claim — known issue

## Favicons

- `src/siteFavicon.ts`: `unavatar.io` and `google.com/s2/favicons` for web shortcuts (network on config load)

## Weather HUD

- `RadialMenu.tsx` fetches `wttr.in` (orphaned feature per TODO §2.3)

## Updater

- electron-updater → GitHub releases `YahyaZekry/rovyl-linux` (package.json publish configuration); NSIS feed on Windows; Linux needs AppImage/deb feed decision

## VS Code family `state.vscdb`

- Read-only via `sql.js` (WASM). Key: `history.recentlyOpenedPathsList`. Paths: `%APPDATA%\<IDE>\User` on Windows → `~/.config/<IDE>/User` on Linux

## Native helper protocol (in-process contract, not external)

- Spawned with parent PID; exits when parent dies
- Exact line grammar in `systems.md`; any Linux helper MUST reproduce it byte-for-byte (JS side parses with simple split/trim)

## Distribution status (verified 2026-11-10)

- **AUR**: [rovyl-bin](https://aur.archlinux.org/packages/rovyl-bin) is live at **1.17.0-1**, maintained by `TheBearCode`; AUR RPC reports no out-of-date flag. This is the already-published release, not the pending maintenance build.
- **Chaotic-AUR GitHub request**: [chaotic-aur/packages#4539](https://github.com/chaotic-aur/packages/issues/4539) remains **open**, labelled `request:new-pkg`, `needs-triage`, and `info:prebuilt`. The author's Linux-port explanation is present, and `waiting:issuer-feedback` was removed; no acceptance is recorded.
- **Chaotic-AUR GitLab submission**: [pkgbuilds !3910](https://gitlab.com/chaotic-aur/pkgbuilds/-/merge_requests/3910) is **closed without merge** (`merged_at=null`, closed 2026-10-07). The `rovyl-bin` directory lookup on upstream `main` returns 404. Do not describe this as accepted or available from Chaotic-AUR.
- **GitHub release flow**: tagged Linux build completion triggers `aur-publish.yml` via `workflow_run`, with manual dispatch for backfills. The workflow downloads the release AppImage, updates its version/checksum and .SRCINFO, then pushes the AUR package using its configured SSH secret.
- **GitLab source copy**: [YahyaZekry/rovyl-linux](https://gitlab.com/YahyaZekry/rovyl-linux) is synchronized through the connected GitLab (Beta) account. First sync commit `28dcb362` was fetched locally and its complete Git tree exactly matched GitHub `cd9539d`; other branches/tags also matched. The connector creates a separate sync commit, so main commit IDs/history differ even when all file contents and modes match. Terminal HTTPS/SSH push credentials remain unavailable; use the connector for content updates or restore Git credentials for an exact Git-history mirror. The earlier prepared local deploy key is unused and does not need to be added for connector-based updates.

Chaotic-AUR MR !3910 maintainer feedback is now verified through the connected account: the maintainer asked for a templated GitHub issue and said maintainers add the package files if accepted. Issue #4539 and its Linux-port explanation already exist; no new issue/comment/MR was created.
