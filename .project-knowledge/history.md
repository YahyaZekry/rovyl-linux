# History

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-09-14
> Append-only.

## Decisions

- **Trigger must be a hook, not a poll** — polling `GetAsyncKeyState` only observed the button; the event still reached windows underneath (autoscroll while aiming). Whatever swallows the event must also detect it (docs/ARCHITECTURE.md)
- **Synthetic click signature** — helper-tagged `dwExtraInfo`/evdev value so the hook never re-interprets its own injections
- **Confirmation resolves from live pointer, never React state** — release mid-flight confirmed stale slices
- **Hit area must equal paint** — slice wrappers are `pointer-events:none`; visible tiles take clicks
- **Dwell engine: 5 rules** — arming from real mousemoves only, delay from first paint, target is `{level,index,itemId}`, synchronous cancel, clock measures settled pointer
- **§3.4/3.5 icons out of JSON** — content-addressed `rovyl-icon://` store; config 456 KB → 12.7 KB, icon-cache 144 KB → 664 B
- **active-win deleted** — 4 Win32 calls it provided came free from the warm `foreground-focus` helper; p50 0.31 ms
- **Left/right mouse buttons rejected as trigger** — would collide with primary click/context menu
- **2026-09-14: Linux port approach** — X11-first (Wayland cannot support global grab/warp by design); keep helper line protocol, replace binary; `deb` primary target, AppImage for autoupdate; taskbar overlay/AUMID/registry paths dropped on Linux
- **2026-09-14: Linux helper = X11 grabs, not evdev** — passive `XGrabButton(..., GrabModeSync)` on root is the WH_MOUSE_LL analogue: the frozen press reaches us and one `XAllowEvents` decides swallow (AsyncPointer) vs pass-through (ReplayPointer), exactly the hook's return-1/CallNextHookEx pair. XTest re-synthesises handed-back clicks; the C# `dwExtraInfo` signature becomes an expected-synthetic counter replayed at the grab. Needs only DISPLAY access — no udev/input-group packaging burden. Consequence: helper sees only X11 clients (XWayland included), which IS the Wayland fallback story
- **2026-09-14: launch ladder on Linux** — `desktop:<id>` picker entries launch via `gio launch`/`gtk-launch`; all other app commands run through one new `exec_sh` method (`/bin/sh -c`, detached); Windows AUMID/IDE heuristics bypassed by a chained platform branch, not parametrised
- **2026-09-14: Wayland verdict revised after research** — "hotkey-only" was too pessimistic. Gesture works on all compositors via evdev grab + uinput reinjection (compositor-independent, no Wayland protocol needed); Electron ≥41 makes the transparent wheel a supported native-Wayland window; GlobalShortcuts portal replaces XGrabKey hotkeys; KDE/wlroots reach near-full parity (layer-shell, foreign-toplevel, KWin scripting), GNOME stays degraded (no layer-shell/foreign-toplevel/injection). InputCapture portal explicitly rejected: its trigger model is screen-edge barriers (KVM-style), compositor decides when capture starts — wrong shape for a hold-button gesture. Full plan: `docs/wayland-port-plan.md`
- **2026-09-14: Wayland is the PRIMARY target** — maintainer runs Wayland-only (no X11 sessions available); X11 QA moves to CI/community
- **2026-09-14: Electron 28 → 44.3.0** — no removed-API usage in the codebase (`protocol.handle` was already modern); build + all smokes green; native Wayland boot verified end-to-end
- **2026-09-14: software rendering breaks first paint on native Wayland** — with `app.disableHardwareAcceleration()` + ozone/wayland (+NVIDIA), a transparent `show:false` window never produces its first frame, so `ready-to-show` never fires and the boot promise chain stalls silently (main process alive, renderer alive, no error anywhere). Hardware GL paints in ~400ms. Diag warns when the flag is set under ozone/auto. On X11/electron-28 software rendering was harmless — this trap is new

## Fixed

- See `TODO.md` checked items (§1.1, §1.3, §2.1, §2.6, §3.1–3.11) with detailed write-ups in that file
- **2026-09-14: Wayland gesture = evdev grab + uinput reinjection** — every mouse event node is `EVIOCGRAB`ed and re-emitted through a paired uinput device copying full capabilities AND identity (vendor/product/bustype/props) so libinput applies the same DPI/hwdb rules; forward = write to the pair, swallow = don't. Injected events flow out of our own devices, which the scan skips by `rovyl-fwd-` name prefix — no signature scheme needed because there is no feedback path
- **2026-09-14: `POS x y` protocol extension** — evdev events carry only deltas, but BLOCK must judge clicks by absolute position; main (which owns cursor truth via Electron screen APIs) feeds positions at 30 ms while the wheel is open. Unknown commands are ignored by the other backends, so the extension is safe
- **2026-09-14: 9-token BLOCK parse bug** — both `apply_command` parsers capped at 8 argv tokens while the BLOCK command has 9 (`BLOCK x y w h mx my mw mh`); BLOCK was silently dropped in the X11 mode AND the new one. Cap raised to 10 in both
- **2026-09-14: inotify dropped for a 1 s poll** — uinput node creation does not raise IN_CREATE on /dev/input on this kernel (zen 7.2.3), so hotplug rescans run on a select timeout instead; simpler and cannot miss
- **2026-09-14: clickless execution off on native Wayland** — no pointer-warp protocol exists; the capture gate skips WARP-dependent paths (see docs/wayland-port-plan.md §2.4)
- **2026-09-14: first live grab froze the maintainer's pointer — REL forwarding bug** — `ev_handle_rel` relayed motion through `inject_button`, which writes type=EV_KEY; every mouse-move was injected as a garbage key event, so with the real mouse grabbed the compositor cursor never moved. Fixed to `emit_uinput(EV_REL, …)`. Lesson: the virtual-device E2E suite only asserted KEY/WHEEL paths — motion assertions (REL 0 10 on the forwarded node) are now part of the scenario list. Kernel releases EVIOCGRAB on process death, so killing the helper instantly restores the pointer
