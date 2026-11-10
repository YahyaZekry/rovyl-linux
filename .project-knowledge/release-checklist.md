# Linux maintenance release — proposed v1.17.1

> Updated: 2026-11-10. Not tagged or published; package version remains 1.17.0 until acceptance.

## Release notes

- Login startup stays in the tray instead of opening Settings.
- A late duplicate login launch stays silent; a manual relaunch opens Settings through the shared reveal path.
- Desktop launches check launcher exit status, try gtk-launch when gio fails, and recover from a stuck launcher after five seconds.
- Linux mouse-trigger settings offer Wheel, Mouse 4, and Mouse 5. Unsupported saved modifier/button bindings are left intact but disabled instead of silently becoming another button.
- Changing trigger timing preserves the selected binding and its supported gesture mode.

Maintenance: upstream updates now generate a review report instead of automatically merging the Windows-native rewrite. This fork remains a Linux Electron app.

## Verified

- [x] Local production build, regression checks, and AppImage packaging.
- [x] Packaged main process and Linux launcher match checked source; updated AppImage installed atomically with backups. Running app was not interrupted.
- [x] [GitHub test suite](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42804378472).
- [x] [Linux packages: AppImage, Debian 12/13, Ubuntu 22.04/24.04](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42804378614).
- [x] [Isolated gesture scenarios and headless Wayland startup](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42804379598). Startup checks production document load and the main-window ready event; the captured black idle frame does not prove visible Settings or a successful real desktop login.
- [x] [Read-only upstream report generated and downloaded](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42803749588).

## Remaining acceptance

- [ ] At the next real sign-in, Rovyl remains in the tray without opening Settings.
- [ ] Launch Rovyl manually while it is already running: Settings opens with working controls.
- [ ] Close Settings and confirm the normal mouse/hotkey wheel still opens, launches a shortcut, and dismisses.

The user reported these checks are not tested yet on 2026-11-10. No release tag until they are confirmed.

## Publish after acceptance

1. Bump package.json and package-lock.json together to 1.17.1 and commit the accepted release state.
2. Tag that commit v1.17.1 and push to GitHub. The Linux builds workflow packages and attaches AppImage/deb assets to the release.
3. Verify the release assets and the subsequent AUR publish run; confirm the AUR version and source checksum.
4. GitLab contents are synchronized through the connected GitLab (Beta) account; verify matching file trees after each connector sync. Main commit IDs differ because GitLab creates its own sync commit. Terminal Git push access remains unavailable; no additional key setup is needed for connector-based updates.
