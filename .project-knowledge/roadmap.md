# Roadmap

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-11-11
> Forward-looking only. TODO.md is the source of truth for its own backlog (stable §.n ids).

## Current Goal

Maintain the Linux Electron app independently of upstream's Windows-native rewrite.

1. User confirmed quiet startup and current desktop behavior on 2026-11-13; keep multi-monitor and scaling acceptance open.
2. Upstream integration remains incomplete: 73 incoming commits were not merged; the workflow only reports them. Twenty-eight commits belong to the last compatible Electron-era line through 1.19. Selected settings polish and optional sounds are now ported (five Electron commits); dismissal changes were skipped at the user’s request. Keep the Windows-native rewrite out of this Linux app; no full upstream merge is claimed.
3. v1.17.1 is published on GitHub and AUR; monitor Chaotic-AUR triage and test the display combinations listed below.
4. Keep deb udev/postinst polish and Windows compatibility checks separate from this Linux release.

**Recently shipped:** v1.16 upstream merge (by hand); Linux system dock with live readings
and desktop-aware panels; panel clearance for docks; workspace Root wheel (Launcher/Slide);
taskbar icon fix; two-sided calibration + double-click option; settings category cleanup
(Position consolidated in Appearance, no duplicates).

---

## Known Bugs

- [ ] Verify launch as an unprivileged user on the affected machine. AUR `1.17.1-1` installed owner-only folders; revision `1.17.1-2` fixes their permissions and was published in AUR commit `7a50535`. The public RPC index still returned `1.17.1-1` immediately after publication.

- User confirmed the Near mouse/Area fix on the current KDE desktop on 2026-11-11. This is not acceptance of every display configuration.
- [ ] Multiple monitors: correct monitor choice and Near mouse placement on each monitor.
- [ ] Fractional scaling (125% / 150%, where supported): placement, clipping and Area selection.
- [ ] Mixed scaling across monitors: repeat the same checks on each screen.
- [ ] Other desktop acceptance remains separate: Pointer hit testing and audible feedback. Quiet login/manual startup checks remain above. Other Wayland desktops fall back to center for cursor placement.

- [ ] Chaotic-AUR acceptance remains pending: GitHub #4539 is open (`needs-triage`), while GitLab MR !3910 is closed without merge. AUR package source is live at 1.17.1-1 (published 2026-11-13). *(checked: 2026-11-10)*

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
