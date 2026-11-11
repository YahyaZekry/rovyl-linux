# Rovyl — Knowledge Index

> Last updated: 2026-11-11 (selected Electron settings/sound ports and experimental Wayland follow-pointer)
> Status: Active
> Stack: Electron 44.3.0 + React 18 + TypeScript + Vite 8 + Tailwind 4 (main process: plain CJS JS)
> Current goal: verify quiet startup, selected settings/sound ports and experimental Wayland targeting; full upstream integration remains incomplete. AUR is live and Chaotic-AUR acceptance remains pending

## What This Project Does

This fork maintains Rovyl as a Linux radial launcher: hold the middle mouse button, aim,
and release to launch apps, folders, files, URLs, or commands. It uses Electron/React with
a native Linux input helper; upstream Windows-native changes are reviewed for selective ports.

---

## Files in This Folder

| File | Contents | Load when... |
|------|----------|--------------|
| `stack.md` | Tech stack, dev commands, env vars | Setting up, adding deps, checking env vars |
| `structure.md` | File tree, entry points, key files | Navigating the codebase, adding files |
| `schema.md` | Persistence shapes (settings/config/icons/localStorage) | Touching persistence or backups |
| `systems.md` | Gesture engine, windowing handshake, focus, licensing, updater, tray | Touching the main process or native layer |
| `features.md` | User-facing features and workflows | Understanding what's built |
| `integrations.md` | License API, favicons, weather, updater feed, VS Code MRU | Touching network calls or external data |
| `components.md` | Renderer component inventory | Adding/changing UI |
| `roadmap.md` | Linux maintenance plan, known bugs, TODO ids | Starting any task — pre-flight check |
| `release-checklist.md` | Proposed maintenance release, verified CI and remaining acceptance | Preparing or publishing the next Linux release |
| `history.md` | Decisions, past fixes, removals | Debugging, reviewing why code is the way it is |
| `sessions.md` | Session log | Reviewing work history |

---

## Context Loading Guide

| Task | Load these files |
|------|-----------------|
| Touching the gesture/native helper | `systems.md` + `history.md` |
| Persistence / config / icons | `schema.md` |
| Renderer UI work | `components.md` + `features.md` |
| Launching / discovery / icons | `systems.md` + `integrations.md` |
| Linux port work | `roadmap.md` + `systems.md` |
| General orientation | This file → pick by task |

*Maintained with [project-knowledge](https://github.com/YahyaZekry/project-knowledge-skill) · by [Yahya Zekry](https://github.com/YahyaZekry)*
