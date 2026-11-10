# Linux maintenance release — proposed v1.17.1

> Updated: 2026-11-10. Not tagged or published; package version remains 1.17.0 until acceptance.

## Release notes

- Login startup stays in the tray instead of opening Settings; manual launches wait for window initialization so their first Settings reveal is not dropped.
- A late duplicate login launch stays silent; a manual relaunch opens Settings through the shared reveal path.
- Desktop launches check launcher exit status, try gtk-launch when gio fails, and recover from a stuck launcher after five seconds.
- Linux mouse-trigger settings offer Wheel, Mouse 4, and Mouse 5. Unsupported saved modifier/button bindings are left intact but disabled instead of silently becoming another button.
- Changing trigger timing preserves the selected binding and its supported gesture mode.

Maintenance: upstream updates now generate a review report instead of automatically merging the Windows-native rewrite. This fork remains a Linux Electron app.

## Verified

- [x] Local production build, regression checks, and AppImage packaging.
- [x] Packaged main process and Linux launcher match checked source; updated AppImage installed atomically with backups. Running app was not interrupted.
- [x] [GitHub test suite](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42904366057).
- [x] [Linux packages: AppImage, Debian 12/13, Ubuntu 22.04/24.04](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42804378614).
- [x] [Isolated gestures and visible Settings on a cold manual Wayland launch](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42904366011). The test now requires the native Settings-on-screen event; its captured screenshot was inspected and shows Settings. The earlier black idle screenshot did not cover this behavior.
- [x] [Read-only upstream report generated and downloaded](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42803749588).

## Remaining acceptance

- [ ] At the next real sign-in, Rovyl remains in the tray without opening Settings.
- [ ] Launch Rovyl manually while it is already running: Settings opens with working controls.
- [ ] Close Settings and confirm the normal mouse/hotkey wheel still opens, launches a shortcut, and dismisses.

On 2026-11-10 the user reported that quitting and manually relaunching did not show Settings. The early show-window IPC was dropped before mainWindow assignment; it now awaits ensureMainWindow. Regression checks and visible Settings in headless Wayland passed, and the corrected AppImage is installed. User retest on the real desktop and real login behavior remain pending. No release tag until confirmed.

## Publish after acceptance

1. Bump package.json and package-lock.json together to 1.17.1 and commit the accepted release state.
2. Tag that commit v1.17.1 and push to GitHub. The Linux builds workflow packages and attaches AppImage/deb assets to the release.
3. Verify the release assets and the subsequent AUR publish run; confirm the AUR version and source checksum.
4. GitLab contents are synchronized through the connected GitLab (Beta) account; verify matching file trees after each connector sync. Main commit IDs differ because GitLab creates its own sync commit. Terminal Git push access remains unavailable; no additional key setup is needed for connector-based updates.
