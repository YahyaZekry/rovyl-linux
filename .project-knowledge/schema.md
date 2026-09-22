# Schema (Persistence)

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-14
> All paths under Electron `userData`. Source of truth: `backend/electron-main.js`, `backend/persistence-normalize.cjs`, `backend/icon-store.cjs`, `src/persistenceMirror.ts`.

## Files in userData

| File | Shape | Notes |
|------|-------|-------|
| `settings.json` | flat settings object | written on settings save |
| `config-v2.json` (+ `.bak`, `.broken-*`) | full wheel config (workspaces, shortcuts) | debounced rewrite + `.bak`; migration from legacy dirs (`Rovyl`, `Zenith OS`, `zenith-radial-menu`) |
| `icon-cache.json` | map: AUMID/target → `rovyl-icon://` reference | small since §3.5; migration deferred 3 s |
| `icons/<sha256>.<ext>` | content-addressed icon bytes | served over custom `rovyl-icon://` protocol; GC sweep of orphans |
| `.env` / `.env.local` | project-root env (not userData) | Google OAuth + GPU flags |

## localStorage mirror (renderer, all-or-none)

| Key | Contents |
|-----|----------|
| `zenith_config` | full config mirror |
| `zenith_user` | user/settings mirror |
| (icon cache key) | icon reference mirror |

Write path: `src/persistenceMirror.ts` — three keys or none; on failure clear all and report via `savePersistenceLog`. Never write mirror before the authoritative disk save.

## Icon references

`customIconUrl` holds `rovyl-icon://<sha256>.<ext>` (85 B), never base64 (§3.4). Conversion from legacy base64 happens on the read path.

## VS Code MRU (read-only, external)

`sql.js` reads `<IDE globalStorage>/state.vscdb`, key `history.recentlyOpenedPathsList`; WASM loaded from asar.unpacked fallback. `file:///` URIs converted to Windows paths (Linux port: POSIX paths, `~/.config/<IDE>`).
