# Wayland Support — Research & Plan

> 2026-09-14 · researched via Exa (sources at the bottom) · companion to the Linux port plan in `.project-knowledge/roadmap.md`
>
> **Priority note:** the maintainer runs Wayland-only (no X11 anywhere), so Wayland is the
> PRIMARY path, not a follow-up. X11-side QA moves to community/CI; everything below is
> ordered Wayland-first.
>
> **Bottom line:** the hold-middle-button gesture IS possible on native Wayland — not through
> any Wayland protocol, but through the same kernel-level pattern input-remapper uses
> (`EVIOCGRAB` + uinput reinjection), which works identically on every compositor. The parts
> that stay degraded are the ones Wayland genuinely forbids: exact pointer warp, covering
> fullscreen windows on GNOME, and stealing focus on GNOME. KDE Plasma 6 and wlroots
> compositors can get a near-full experience; GNOME gets a usable but reduced one.

## 1. Why this revises the earlier verdict

The original port plan called Wayland "hotkey-only". That was too pessimistic, and part of it
was based on our pinned Electron 28, where it WAS true. Three findings change it:

1. **Electron went Wayland-native.** As of Electron 38.2 native Wayland is the default path
   (no `--ozone-platform=wayland` dance), and the 41.x series fixed the long-standing
   transparent-frameless problems (CSD, 1px borders, smearing). Rovyl's window model
   (transparent, frameless, `hasShadow: false`) is now the *supported* configuration on
   Wayland — better there than on X11 per the Electron team.
2. **Electron supports the GlobalShortcuts portal** (tracker marks Electron/Chromium/GNOME/KDE
   complete as of mid-2026), so the hotkey fallback becomes a first-class path, not a
   "tell users to add a system shortcut" cop-out.
3. **The gesture doesn't need the compositor's permission at all** if we take the
   input-remapper route: grab the mouse's evdev node and reinject through a uinput copy.
   The compositor simply reads our virtual device. This is production-grade (input-remapper,
   keyd, ydotool all live this way) and compositor-independent.

What remains genuinely impossible or per-desktop, confirmed by protocol support tables
(measured on KWin 6.6.4) and the portal documentation:

| Capability | Wayland reality |
|---|---|
| Global click capture + swallow | ✅ evdev grab + uinput (all compositors; needs `input` group / udev rule) |
| Exact pointer warp | ❌ no protocol; libei RemoteDesktop gives absolute motion (KDE); rel-delta approximation elsewhere |
| Above-fullscreen overlay | ✅ layer-shell on KWin (v5) + wlroots; ❌ GNOME/Mutter (no layer-shell) |
| Always-on-top (plain toplevel) | ❌ not a Wayland concept; KDE can raise via scripting; generally best-effort |
| Foreground window detection | ✅ `zwlr_foreign_toplevel_management` (KWin v3, wlroots); ❌ GNOME (needs a Shell extension; Introspect portal is off by default) |
| Focus steal | ✅ KDE via KWin scripting (`windowactivate`); ✅ wlroots foreign-toplevel activate; ❌ GNOME (xdg-activation only helps when the other app cooperates) |
| Global hotkey | ✅ GlobalShortcuts portal (KDE 6+, GNOME 48+); Electron support complete |
| InputCapture portal | ❌ **wrong shape for us** — its trigger model is edge pointer-barriers (KVM-style); the compositor decides when capture starts. Confirmed in the portal PR: "capture on request" was scoped out |

## 2. Architecture: one new helper mode, everything else adapts

The X11 helper stays. Add a `wayland` personality to the same binary (or a sibling binary
`rovyl-helper-wayland`) speaking the identical line protocol, so `electron-main.js` cannot
tell the difference. Each Windows concept maps like this:

### 2.1 Gesture: EVIOCGRAB + uinput (the input-remapper pattern)

- On `TRIGGER <vk> ...`: enumerate `/dev/input/event*`, pick mouse-class nodes (test for
  `EV_REL`/`BTN_LEFT`/`BTN_MIDDLE`/`BTN_RIGHT` capabilities; also wheel), `EVIOCGRAB` them.
- For each grabbed node, create a uinput device copying **all** capabilities **and** the
  identity metadata (`name`, `vendor/product/bustype`, `input_props`) — input-remapper
  PR #1290 shows skipping `MOUSE_DPI`/hwdb metadata makes pointers "feel wrong". Tag our
  devices with a private `phys` prefix so we never grab our own injections (the
  `SYNTHETIC_TAG` equivalent) and so udev autoload rules don't misfire.
- Forward every event verbatim; swallow exactly what the Windows hook swallows (trigger
  button, BLOCK-outside-rect clicks, recording); synthesise pass-through clicks by writing
  to the uinput device — the event re-enters through our *forwarding* device, which we
  deliberately don't grab. Same state machine, same click-vs-hold rules, zero X11.
- Hotplug: udev monitor to grab newly-appeared mice, release dead ones (input-remapper's
  regrab loop is the reference).
- Permissions: package a udev rule (`ENV{ID_INPUT_MOUSE}=="1"`, `TAG+="uaccess"` or an
  `input`-group postinst prompt). This is the one real packaging tax; deb handles it, AppImage
  needs a setup step.

### 2.2 BLOCK (click protection while the wheel is open)

Free with the grab: swallow clicks outside the allowed rect exactly as the X11 helper does.
No Electron `setIgnoreMouseEvents(forward)` involvement — on Wayland `forward` doesn't work
anyway (no X11 event forwarding), so the helper-owned BLOCK becomes the *only* mechanism.
The renderer's aim pipeline (main polls `screen.getCursorScreenPoint()` and synthesises
`mousemove`) already works — cursor position comes from the compositor, not from swallowed
events.

### 2.3 The wheel window

- Prerequisite: **Electron ≥ 41** (see §3). Transparent + frameless + `hasShadow: false` is
  the blessed configuration.
- v1: ordinary xdg-toplevel, opened at the target monitor. Works everywhere; loses to
  fullscreen games (game-mode detection will close/park it anyway).
- v2 (KDE + wlroots): **layer-shell overlay** (`zwlr_layer_shell_v1`, overlay layer,
  exclusive zone −1) so the wheel floats above fullscreen apps, mirroring the Windows
  always-on-top behaviour. Core Electron has no layer-shell API; the proven pattern is
  COVAS-Labs/electron-overlay: an offscreen BrowserWindow feeding a self-owned layer surface
  (DMA-BUF shared textures, wl_shm fallback). Real engineering — schedule it, don't block on
  it.

### 2.4 Warp / clickless execution

- No warp protocol exists. v1: **disable** `radialInstantActivate` cursor-parking and the
  exact cursor restore on Wayland (the settings stay; the capability flag says no). Clickless
  execution is opt-in and off by default, so the loss is small.
- v2 (KDE): libei via the RemoteDesktop portal gives absolute pointer motion — that is a true
  warp. Costs a one-time portal permission (KDE 6.3+ can persist it). wlroots: `zwlr_virtual_pointer_v1`
  exists there but **not in KWin**, so keep both paths behind capability detection
  (`wayland-info`-style) — exactly what the neru project does.
- Fallback for the restore-the-cursor nicety: rel-delta injection toward the restore point;
  approximate (compositor accel), fine for "put it back roughly".

### 2.5 Foreground / game-mode / focus steal

- KDE + wlroots: `zwlr_foreign_toplevel_manager_v1` (KWin ≥ 6.x implements v3) gives active
  window, app_id, title, state — everything `FG|bounds|exe|title` needs (app_id replaces
  exe; game-detection tokens match against app_id/title). Activation/steal: KDE KWin
  scripting over D-Bus (the kdotool mechanism: `workspace.activeWindow = client`; note
  `loadScriptFromText` is gone in Plasma 6 — use `loadScript(path)` + per-script `run()`,
  as kdotool/wdotool do). wlroots: foreign-toplevel `activate` request.
- GNOME: none of it. No foreign-toplevel, no scripting API outside extensions, Introspect
  portal read-only and off by default. Game-mode protection and focus steal are **off**;
  a GNOME Shell extension is the only real fix (out of scope; noted as future work —
  keymasq demonstrates the pattern).
- Practically: the wheel still opens on GNOME (grab works), launches still work; the
  fullscreen-game guard shows the wheel late or not at all — document it.

### 2.6 Hotkeys

Electron's `globalShortcut` is XGrabKey-based and dead on native Wayland. With Electron ≥ 38
the GlobalShortcuts portal is the supported route (KDE 6+, GNOME 48+; one-time user
prompt). Implementation: register the open/toggle shortcut through the portal when
`process.platform === 'linux' && sessionType === 'wayland'`, keep `globalShortcut`
elsewhere. Users on older GNOME: system custom shortcut → `rovyl --toggle` (add the CLI
flag; trivial and compositors-independent).

### 2.7 Unchanged

Discovery, icons, launching (gio/xdg), tray (StatusNotifier), persistence — all
compositor-agnostic already. `screen.getCursorScreenPoint()` and monitor enumeration work on
native Wayland Electron. The DWM-specific occlusion switches stay harmless no-ops.

## 3. Prerequisite: the Electron upgrade (28 → ≥ 41)

Everything in §2 assumes it; on Electron 28 native Wayland transparency was broken, so
"Wayland support" on 28 would just be XWayland forever. Consequences:

- Touches the **Windows build too** — same package.json. Migration risk is real but the app
  uses a narrow API surface (IPC + preload + BrowserWindow + Tray + globalShortcut +
  shell + sql.js + electron-updater). Budget a full Windows regression pass (windowing
  handshake first — `verify-radial-windowing` guards it).
- 41.x specifically fixed transparent-frameless border/smearing (our exact window type);
  newer 42/43 continue the CSD work (`setDecorationInsets`, frameless shadow behaviour).
  Pick the newest stable at time of work, not a version number from this doc.
- electron-updater on Wayland/Linux: AppImage target keeps autoupdate working.
- Single-instance lock, `requestSingleInstanceLock`, `app.setLoginItemSettings` — all fine
  on Wayland.

## 4. Per-desktop outcome after the plan lands

| Desktop | Gesture | Wheel over fullscreen | Warp/clickless | FG/game-mode | Focus steal | Hotkey |
|---|---|---|---|---|---|---|
| **KDE Plasma 6** | ✅ | ✅ (layer-shell, v2) | ✅ (libei, v2) | ✅ | ✅ (KWin script) | ✅ portal |
| **wlroots (sway, Hyprland, …)** | ✅ | ✅ (layer-shell, v2) | partial (virtual pointer) | ✅ (foreign-toplevel) | ✅ | ✅ portal |
| **GNOME 48+** | ✅ | ❌ (toplevel loses to fullscreen) | ❌ | ❌ | ❌ | ✅ portal |
| **GNOME < 48** | ✅ | ❌ | ❌ | ❌ | ❌ | system-shortcut |
| **COSMIC** | ✅ | best-effort | ❌ | ❌ | ❌ | portal pending (no backend yet) |

## 5. Phases

1. **Electron upgrade** (blocks everything; Windows regression pass included).
2. **Wayland helper mode** — evdev grab + uinput, same protocol, udev rule in the deb,
   hotplug handling, grab-conflict UX (input-remapper/keyd may already own the device:
   detect and tell the user instead of silently failing).
3. **Wayland window mode** — native Wayland launch (`ELECTRON_OZONE_PLATFORM_HINT`),
   capability flags to the renderer (no warp, no clickless, toplevel z-order), settings UI
   honest about it, game-mode degradation on GNOME.
4. **KDE integration** — foreign-toplevel FG, KWin-scripting focus steal, portal hotkey.
5. **CI/CD workflows** — distro builds + compositor E2E (§5a below).
6. **Layer-shell overlay (v2)** — the COVAS-style offscreen-window + layer surface for
   KDE/wlroots; biggest single item, isolated.
7. **libei warp (v2, KDE)** — RemoteDesktop portal session for absolute cursor placement,
   re-enabling clickless execution where possible.

Phases 1–3 make Wayland *usable*; 4–5 make it *verifiable and shippable*; 6–7 make KDE
first-class; GNOME stays honest-degraded.

### 5a. CI/CD & DE test workflows (GitHub Actions)

**`build-linux.yml` — distro build matrix.** One workflow, containerised builds so the deb
matches each target's glibc and dependencies:

| Job | Container | Artifacts |
|---|---|---|
| `deb-debian-12` (bookworm) | `debian:12` | `Rovyl-<ver>-debian12-amd64.deb` |
| `deb-debian-13` (trixie) | `debian:13` | `Rovyl-<ver>-debian13-amd64.deb` |
| `deb-ubuntu-22.04` (jammy) | `ubuntu:22.04` | `Rovyl-<ver>-ubuntu22.04-amd64.deb` |
| `deb-ubuntu-24.04` (noble) | `ubuntu:24.04` | `Rovyl-<ver>-ubuntu24.04-amd64.deb` |
| `appimage` | `ubuntu:22.04` | `Rovyl-<ver>-linux.AppImage` (autoupdate feed) |

Each job: install Node 20 + build-essential + libx11-dev/libxtst-dev → `npm ci` →
`npm run helper:build` (compiles `rovyl-helper-linux` per-distro) → `npm run build` →
`electron-builder --linux deb|AppImage`. Post-build assert step: unpack the deb and check
the udev rule landed in `/lib/udev/rules.d/` and `rovyl-helper-linux` is in the unpacked
set (asarUnpack). Tag builds additionally publish to GitHub Releases.

**`tests.yml` — fast checks on every PR** (`ubuntu-latest`): the pure-JS smoke suite
(`npm run test:*`, all runnable off-Windows), `node --check` on main-process files, the C
helper compiled with `-Wall -Wextra -Werror`, and the helper line-protocol smoke (pipe
BLOCK/TRIGGER/FG round-trips through a fake display-less run with `WAYLAND_DISPLAY` unset —
protocol code paths that don't touch the display).

**`wayland-e2e.yml` — real compositor, real gesture, headless.** The core Wayland QA loop,
runs on PRs touching `backend/` + nightly:

1. Job container: Debian/Ubuntu + `sway` with `WLR_BACKENDS=headless
   WLR_LIBINPUT_NO_DEVICES=1`, `WLR_RENDERER=pixman` — a real wlroots compositor with zero
   hardware. (Alternative images worth keeping in the matrix: `labwc` and `cage`; KWin has
   `kwin_wayland --virtual` for a KDE-side job.)
2. Create a virtual input device via uinput inside the container (small C helper or
   `ydotool` in daemon mode) — this is both the test pointer AND the exact device class the
   Rovyl helper must grab.
3. Launch Rovyl with `WAYLAND_DISPLAY` pointed at the headless sway socket
   (`ELECTRON_OZONE_PLATFORM_HINT=auto`, software rendering, `--no-sandbox`).
4. Drive the gesture: uinject middle-button hold via the virtual device → assert via
   `swaymsg -t get_tree` that the Rovyl surface resized to the output geometry (wheel open)
   → release → assert it collapsed.
5. `grim` screenshots of open/closed states as artifacts; assert non-zero non-transparent
   pixels on the open shot (wheel actually rendered, not a black surface).
6. Same job asserts the helper grabbed (not killed by) the virtual device and that a
   second grab attempt returns EBUSY (conflict-detection path).

This workflow is also what replaces the "real-gesture test on X11" item — the maintainer's
machine is Wayland-only, so CI IS the X11-free regression harness. Desktop-specific
behaviour that headless sway can't see (KWin scripting steal, portal dialogs, GNOME
degradation) stays a **manual QA matrix**:

| DE / session | Install source | Manual checklist |
|---|---|---|
| Ubuntu 24.04 GNOME 46+ | deb | gesture, hotkey portal prompt, wheel-not-over-fullscreen caveat, launch/discovery |
| Fedora KDE / KDE neon (Plasma 6.x) | deb | gesture, layer-shell v2 items, KWin steal, foreign-toplevel FG, portal permission persistence |
| Debian 12/13 GNOME / Xfce (XWayland) | deb | XWayland path still healthy |
| Hyprland / sway (latest) | AppImage | wlroots full path, hotplug (reconnect BT mouse), grab-conflict message with input-remapper running |
| COSMIC (Pop!_OS) | AppImage | gesture + hotkey only, degrade messaging |

Nightly optional extra: one QEMU VM boot per desktop ISO with the deb installed, running
the same sway-E2E asserts through SSH — only worth it after the containerised matrix is
green.

## 6. Risks

- **udev/permissions UX** — the deb postinst can add the rule; AppImage users must be told.
  Alternative nobody should want: shipping setuid.
- **Pointer feel** — uinput forwarding must preserve hwdb/DPI metadata (input-remapper
  PR #1290 is the checklist); test with a gaming mouse.
- **Latency** — one extra userspace hop for every mouse event while grabbed. input-remapper
  ships this to many users; measure anyway (the helper already has a 15 ms tick budget).
- **Interop** — if the user also runs input-remapper/keyd/ydotool, the grab is contended.
  Detect `EBUSY` on grab, name the conflicting tool, degrade to hotkey-only.
- **Bluetooth mice** — device nodes change on reconnect; hotplug handling is load-bearing,
  not optional.
- **Electron upgrade regressions on Windows** — the radial windowing handshake is the
  sensitive part; the verify script exists precisely for this.
- **CI without /dev/input** — GitHub runners don't expose uinput by default; the e2e job
  must load `uinput` (available in the runner kernel) and create `/dev/uinput` with mknod,
  or run the compositor job inside a privileged container. Budget for this when writing
  `wayland-e2e.yml`; if uinput stays blocked, fall back to `WLR_LIBINPUT_NO_DEVICES=1` plus
  `zwlr_virtual_pointer_v1` for compositors that support it (sway does) — the helper's grab
  path then needs a container-only alternate trigger (protocol-level test double).

## 7. Key sources

- xdg-desktop-portal InputCapture spec + PR #714 discussion (barrier-trigger model, libei transport)
- KWin MR !5742 (input-capture portal backend), xdg-desktop-portal-kde MRs 283/313/326 (permissions/persistence)
- Plasma 6.1 = first KDE release with InputCapture; GNOME 46 = first with InputCapture (pop-os COSMIC issue #217)
- input-remapper: injector design (grab + forwarded uinput, capabilities copying), PR #1290 (DPI metadata)
- Electron: "How Electron went Wayland-native" (38.2 default, transparency), PR #49295/#49885 (CSD),
  issue #50541 + #50605 (transparent frameless borders/smearing fix in 41.x)
- COVAS-Labs/electron-overlay (X11/wayland-electron/wayland-layer-shell backends, DMA-BUF feeding)
- neru docs/LINUX_DESKTOPS.md — measured KWin 6.6.4 protocol table (layer-shell v5 ✅,
  foreign-toplevel v3 ✅, virtual-pointer ❌, libei/RemoteDesktop as the KDE injection path; GNOME unsupported)
- kdotool + wdotool PR #48 (KWin scripting on Plasma 6: loadScript(path), per-script run(); windowactivate)
- GlobalShortcuts portal tracker (2026-06): Electron, Chromium, GNOME, KDE complete; KDE 6+/GNOME 48+
