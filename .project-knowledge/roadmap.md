# Roadmap

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-21
> Forward-looking only. TODO.md is the source of truth for its own backlog (stable §.n ids).

## Current Goal

The Linux/Wayland port is done and user-verified (wheel, launches, autoscroll, hotkey,
click-away, autostart, two-sided calibration, double-click option). Remaining work is
polish and release mechanics:

1. ~~**Repo rename**~~ — **done 2026-09-21**: remote, package.json repository/publish, and the README clone URL now point at YahyaZekry/rovyl-linux (the C# Windows-side `build.win` config still builds; only its publish target was shared).
2. **deb polish** — udev rule + `input` group membership in postinst (currently manual).
3. **Windows regression pass** — the 7-arg TRIGGER silently broke trigger capture there
   (C# parsers cap at 6 fields) and is now platform-branched (5 args on win32), but the
   Windows build hasn't been touched since.
4. **Shelf** — `npm run helper:build` is the Windows csc script; add `helper:build:linux`
   (gcc `-lX11 -lXtst -ludev`) so the Linux helper isn't compiled by hand.

**Recently shipped (2026-09-21):** two-sided middle-click calibration (3 fast + 2 slow
presses → midpoint threshold), optional double-middle-click menu (helper dblMs window +
TRIGGER_DOUBLE), preload bridge for all newer renderer APIs, BLOCK_CLICK / CLICK_CONSUMED
parsing in main, Windows TRIGGER branch fix. E2E scenarios A–J green.

---

## Known Bugs

- [ ] Hotplug rescan is flaky when a device appears within ~100 ms of helper start (udev ACL lands after node creation); fine for the boot case, matters for BT reconnect — needs a retry-until-grab loop per device *(found: 2026-09-14)*
- (found 2026-09-14) `@electron/get` extract silently produced a broken `dist/` on this machine — manual unzip + `path.txt` (`electron`, no newline) fixed; watch for it after fresh `npm install`
- (found 2026-09-14) On Wayland sessions the XWayland gesture grab only sees X11 clients' pointer events; native Wayland windows never trigger and are immune to BLOCK — by design, documented fallback is hotkey
- See `TODO.md` open items (unchecked): §1.2 dead `fixedPosition` key, §1.4 offline claim, §1.5 cursor-follow architecture, §2.2–2.5 dead code/licensing decisions, plus §4+ sections

---

## Active TODOs

- [x] Repo-rename follow-ups — done 2026-09-21 (remote URL, package.json repository/publish, README clone URL)
- [ ] deb udev/postinst polish: ship the udev rule + input-group in the package scripts *(added: 2026-09-19)*
- [ ] Windows regression pass (trigger capture, settings, launch) on a Windows box *(added: 2026-09-21)*
- [ ] `helper:build:linux` npm script wrapping the gcc command *(added: 2026-09-21)*

---

## Planned Features

- (none committed)
