# Systems

> Part of Rovyl/.project-knowledge/ | Last updated: 2026-11-10

| System | Status | Details |
|--------|--------|---------|
| Gesture engine | Active | `rovyl-helper.exe mouse-blocker <pid>` (fallback `mouse-blocker.ps1`), spawned ~electron-main.js:2168. Line protocol over stdin/stdout: in `BLOCK x y w h mx my mw mh`, `TRIGGER vk mode slop [holdMs dragPx]`, `TRIGGER OFF`, `RECORD ON/OFF`, `SHORTCUT_TRIGGER vk mask`, `WARP x y`, `EXIT`; out `READY`, `TRIGGER_DOWN/UP/HOLD`, `SHORTCUT_DOWN/UP`, `RECORD_MOUSE <btn> <mask>`. Click-vs-hold: short stationary press synthesises the button back (signature in `dwExtraInfo`). Gesture state machine + hold cursor poll are pure JS in main (6150-6360) |
| Radial windowing | Active | Transparent/frameless/always-on-top("screen-saver") window; handshake `prepare-radial-show` → cover paint → `radial-open-paint-done` → reveal → bloom. Guarded by `scripts/verify-radial-windowing.mjs` in `npm run build` — break the handshake, break the build |
| Foreground/focus | Active | Helper `foreground-focus` mode: `FG` → `FG|bounds|exe|title`; `FOCUS <hwnd>` → foreground steal (AttachThreadInput). Game-detection gates opening during fullscreen games |
| App discovery | Active | Get-StartApps + Start Menu .lnk parsing via inline PowerShell; icon extraction via `extract-icon.ps1` (IShellItemImageFactory → data: URL), fallback `app.getFileIcon` |
| Launch ladder | Active | `shell.openExternal` → `shell.openPath` → `start ""` → `explorer.exe shell:AppsFolder` → terminal wrap (wt/powershell/cmd) → detached spawn. Parsing in `win32-launch.js` |
| Licensing | Active | Google OAuth + license API (`rovyl-red.vercel.app`); `LicenseGate.tsx` shows locked wheel without license |
| Updater | Active | electron-updater (NsisUpdater), GitHub releases; helpers stopped before `quitAndInstall` |
| Tray | Active | `tray-menu.cjs` builds template (pure JS); dark/light icon variants |
| Taskbar overlay | Windows-only | hides taskbar regions while wheel is up (`taskbar-control.ps1` + `taskbar-overlay.cjs`) |
| Autostart | Active | Linux: XDG `~/.config/autostart/rovyl.desktop`; Windows: `app.setLoginItemSettings`. Both pass `--opened-at-login` to keep login starts in the tray; late login duplicates do not reveal Settings. Regression-tested 2026-11-10; live login not verified. |
| i18n | Active | 10 languages in `src/i18n/translations.ts`; wheel's 6 live strings in `src/strings.ts` (bundle guard in build) |

## Linux port mapping

| Windows | Linux |
|---------|-----------------|
| `rovyl-helper.exe` (WH_MOUSE_LL, SendInput, SetCursorPos) | C helper: evdev `EVIOCGRAB` on `/dev/input/event*` + `XWarpPointer`, same line protocol |
| Get-StartApps / .lnk | parse `/usr/share/applications` + `~/.local/share/applications` `.desktop` |
| `extract-icon.ps1` | `.desktop` `Icon=` via hicolor theme lookup |
| launch ladder (cmd/explorer) | `gio launch` / `gtk-launch` / `xdg-open`; terminal detect gnome-terminal/konza/kitty/xterm |
| `FG`/`FOCUS` (hwnd) | X11 `_NET_ACTIVE_WINDOW`, `_NET_ACTIVE` message, `_NET_WM_PID`/`_NET_WM_NAME` |
| machine GUID (registry) | `/etc/machine-id` |
| taskbar overlay, AUMID, registry uninstall probe | no equivalent — skip on Linux |
| Wayland | evdev/uinput helper handles gestures and passive keyboard hotkeys; requires input-device access. Cursor placement is approximate; no general native-Wayland pointer warp. |

## Upstream review (2026-11-10)

`.github/workflows/sync-fork.yml` runs Mondays at 03:00 UTC (05:00 Cairo) or on manual dispatch. It fetches upstream and runs `scripts/upstream-review.mjs`; the Markdown report is the job summary and an artifact. Read-only repository permissions; no merges, pushes, or issue comments. Incoming counts include selectively ported commits whose original hashes are not ancestors. Upstream 2.x is Windows-native; compatible changes are ported selectively into this Linux Electron app.

Linux desktop launches wait for `gio` to exit successfully, with `gtk-launch` as fallback on failure or a missing launcher. Each launcher has a five-second timeout; success confirms launcher acceptance, not the launched application's later health.
