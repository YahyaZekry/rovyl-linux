<div align="center">

<img src="docs/media/banner.png" alt="" width="720">

# Rovyl for Linux

**One gesture. Any destination.**

A radial launcher for Linux. Hold the middle mouse button, aim, release — over Wayland and X11.

![Platform](https://img.shields.io/badge/platform-Linux-fcc624?style=flat-square&logo=linux&logoColor=black)
![Tested on](https://img.shields.io/badge/tested%20on-KDE%20Plasma%206%20%7C%20Wayland-1d99f3?style=flat-square)
![Electron](https://img.shields.io/badge/Electron-44-47848f?style=flat-square&logo=electron&logoColor=white)

</div>

---

## What it is

Hold a mouse button and a wheel of your shortcuts blooms on screen. Move toward what you
want, release — apps, folders, files, websites, commands. No window between you and your
work.

This is a Linux port of [Rovyl](https://github.com/HenryCauan/rovyl) (originally Windows),
rebuilt for Wayland: the gesture engine is a small C helper that talks to your input
devices directly (evdev) and re-injects events through a virtual pointer, so it works on
KDE Plasma, wlroots compositors (sway, Hyprland, …) and X11 alike. No compositor plugins.

## Tested on

| | |
|---|---|
| **Primary** | Arch-based (Garuda) · KDE Plasma 6 · Wayland · NVIDIA (open kernel module) |
| **Expected to work** | Any Wayland compositor, X11 sessions, Electron-supported distros |
| **Electron** | 44 (native Wayland) |

## Install

**AppImage** (any distro):

```bash
# from a release, or build it yourself (see Building)
chmod +x Rovyl-*.AppImage
./Rovyl-*.AppImage
```

**Deb** (Debian 12/13, Ubuntu 22.04/24.04):

```bash
sudo dpkg -i Rovyl-*.deb
```

**Permissions** — the gesture helper reads your mouse at the kernel level. Add your user
to the `input` group (the deb does this for you; otherwise `sudo usermod -aG input $USER`
and log back in).

**Hardened kernels** — if the window never appears, your kernel blocks Electron's
sandbox: launch with `ELECTRON_DISABLE_SANDBOX=1`.

## Middle click behaviour — read this

One physical button can't be both "native middle click" and "open the menu", so:

- **A quick middle click does the native thing** (closes a browser tab, opens a link in a
  new tab) **and** opens the menu. Both. That's the current behaviour; the menu appearing
  is the known quirk.
- **Holding it past a second** hands the button to the app — that's autoscroll, if your
  browser/desktop has it enabled for the `rovyl-fwd-…` device.
- **Launch at startup** (Settings → Startup) writes a real XDG autostart entry on Linux.

Full control lives in **Settings → Activation**: trigger button, mode, hotkey. If the
double-behaviour bothers you, switch the trigger to a side button or the hotkey and your
middle button stays 100% native.

## Known limitations on Wayland

- **The wheel opens centred on the screen**, not at the cursor — Wayland gives an app no
  global cursor position. ("Follow pointer" currently behaves like "main screen".)
- **A taskbar entry appears while the wheel is open** — Wayland has no skip-taskbar
  protocol for normal windows. It disappears when the wheel closes.
- **It does not appear over fullscreen games** (planned: layer-shell overlay).
- **Autoscroll** needs the KDE per-device toggle enabled for the `rovyl-fwd-…` pointer
  (System Settings → Mouse → pick the Rovyl device). Its pointer speed is also tuned
  separately from your physical mouse.
- **No pointer warping** — the "open without clicking" (dwell) cursor parking is
  unavailable.

See **[docs/wayland-port-plan.md](docs/wayland-port-plan.md)** for why each of these is
the way it is, and what's planned.

## Building

Requires **Node 20+** and, on Linux, `build-essential libx11-dev libxtst-dev` (the C
gesture helper compiles as part of the build).

```bash
git clone https://github.com/YahyaZekry/rovyl
cd rovyl
npm install
npm run dist      # AppImage + deb in build-out/
npm start         # dev server + Electron
```

Google sign-in (licensing) needs your own OAuth credentials — copy `.env.example` to
`.env.local`.

## Credits & licence

Linux port of **[Rovyl](https://github.com/HenryCauan/rovyl)** by Henry Cauan (upstream
active at [arshit09/rovyl](https://github.com/arshit09/rovyl)). All the good
architecture is theirs; the Wayland parts are documented in
[docs/wayland-port-plan.md](docs/wayland-port-plan.md) and
[.project-knowledge/](.project-knowledge/).

Licensed under the **GNU GPL v3.0** — see [LICENSE](LICENSE).

---

<details>
<summary>🧠 AI Context</summary>

This project uses the [project-knowledge](https://github.com/YahyaZekry/project-knowledge-skill) skill to maintain a `.project-knowledge/` folder — a living, AI-readable map of the codebase. Every AI session loads only the files relevant to the current task instead of scanning from scratch.

Built by [Yahya Zekry](https://github.com/YahyaZekry/project-knowledge-skill).

</details>
