# Roadmap

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-11-10
> Forward-looking only. TODO.md is the source of truth for its own backlog (stable §.n ids).

## Current Goal

Maintain the Linux Electron app independently of upstream's Windows-native rewrite.

1. Verify that the installed startup fix stays in the tray at the next real login, then manually relaunch Rovyl to confirm Settings opens.
2. Deploy the review-only upstream workflow and inspect its first report; do not merge the upstream archive/Rust restructuring.
3. Release the startup, launcher-error and supported-binding fixes after live acceptance. Packaging and automated checks are recorded in sessions.md.
4. Keep deb udev/postinst polish and Windows compatibility checks separate from this Linux release.

**Recently shipped:** v1.16 upstream merge (by hand); Linux system dock with live readings
and desktop-aware panels; panel clearance for docks; workspace Root wheel (Launcher/Slide);
taskbar icon fix; two-sided calibration + double-click option; settings category cleanup
(Position consolidated in Appearance, no duplicates).

---

## Known Bugs

- [ ] Hotplug rescan is flaky when a device appears within ~100 ms of helper start (udev ACL lands after node creation); fine for the boot case, matters for BT reconnect — needs a retry-until-grab loop per device *(found: 2026-09-14)*
- (found 2026-09-14) `@electron/get` extract silently produced a broken `dist/` on this machine — manual unzip + `path.txt` (`electron`, no newline) fixed; watch for it after fresh `npm install`
- Wayland requires evdev/uinput permissions for native-client gestures. The old XWayland-only limitation predates the shipped evdev backend; it is not the current architecture (verified in code 2026-11-10).
- See `TODO.md` open items (unchecked): §1.2 dead `fixedPosition` key, §1.4 offline claim, §1.5 cursor-follow architecture, §2.3–2.5 HUD/licensing decisions (the §2.2 component files are already absent), plus §4+ sections

---

## Active TODOs

- [x] Repo-rename follow-ups — done 2026-09-21 (remote URL, package.json repository/publish, README clone URL)
- [ ] deb udev/postinst polish: ship the udev rule + input-group in the package scripts *(added: 2026-09-19)*
- [ ] Windows regression pass (trigger capture, settings, launch) on a Windows box *(added: 2026-09-21)*
- [ ] `helper:build:linux` npm script wrapping the gcc command *(added: 2026-09-21)*

---

## Planned Features

- (none committed)
