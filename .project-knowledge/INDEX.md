# Rovyl — Knowledge Index

> Last updated: 2026-09-14
> Status: Active
> Stack: Electron 28 + React 18 + TypeScript + Vite 8 + Tailwind 4 (main process: plain CJS JS)
> Current goal: Dual-platform (Windows + Linux/Wayland-first) — gesture, discovery and packaging work on Linux; remaining: CI workflows, deb polish, Windows regression pass after the Electron 44 jump

## What This Project Does

Rovyl is a radial launcher for Windows: hold the middle mouse button anywhere, a wheel of
shortcuts blooms mid-screen, aim, release to launch apps/folders/files/URLs/commands.
Fully offline except license checks and favicons. This fork is being ported to Linux.

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
| `roadmap.md` | Linux port plan, known bugs, TODO ids | Starting any task — pre-flight check |
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
