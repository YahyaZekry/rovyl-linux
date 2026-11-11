# Linux maintenance release — proposed v1.17.1

> Updated: 2026-11-11. Not tagged or published; package version remains 1.17.0 until acceptance.

## Release notes

- Normal manual launches and login startup stay in the tray. Repeated launches leave the running window state unchanged.
- Settings opens through an explicit Settings action; its reveal waits for window initialization so early requests are not lost. First-install onboarding remains unchanged.
- Desktop launches check launcher exit status, try gtk-launch when gio fails, and recover from a stuck launcher after five seconds.
- Linux mouse-trigger settings offer Wheel, Mouse 4, and Mouse 5. Unsupported saved modifier/button bindings are left intact but disabled instead of silently becoming another button.
- Changing trigger timing preserves the selected binding and its supported gesture mode.

Maintenance: upstream updates now generate a review report instead of automatically merging the Windows-native rewrite. This fork remains a Linux Electron app.

## Verified

- [x] Local production build, regression checks, and AppImage packaging.
- [x] Packaged main process and Linux launcher match checked source; updated AppImage installed atomically with backups. The latest build was restarted cleanly and its live main/helper were verified against source.
- [x] [GitHub test suite](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42904366057).
- [x] [Linux packages: AppImage, Debian 12/13, Ubuntu 22.04/24.04](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42804378614).
- [x] [Isolated gestures and visible Settings on a cold manual Wayland launch](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42904366011). The test now requires the native Settings-on-screen event; its captured screenshot was inspected and shows Settings. The earlier black idle screenshot did not cover this behavior.
- [x] [Read-only upstream report generated and downloaded](https://github.com/YahyaZekry/rovyl-linux/actions/runs/42803749588).

- [x] User confirmed the latest Near mouse placement/Area fix on the current KDE Wayland desktop (2026-11-11); this does not verify additional monitors or scaling combinations.

## Remaining acceptance

- [ ] Multiple monitors: screen selection, Near mouse placement, edges and Area selection on each monitor.
- [ ] Fractional scaling (125% / 150%, where supported) and mixed scaling across monitors: placement, clipping and Area selection.

- [ ] At the next real sign-in, Rovyl remains in the tray without opening Settings.
- [ ] Launch Rovyl manually, including while already running: no Settings window opens.
- [ ] Choose Open Settings from the tray: Settings opens with working controls.
- [ ] Close Settings and confirm the normal mouse/hotkey wheel still opens, launches a shortcut, and dismisses.

On 2026-11-10 the user explicitly corrected the intended behavior: manual launches must also stay quiet. Removed the automatic Settings opening from initial renderer state and duplicate-instance handling. Startup regressions and full build/AppImage packaging pass; corrected AppImage installed with backup. The explicit Settings reveal guard remains. Real desktop acceptance remains pending; no release tag created.

## Publish after acceptance

1. Bump package.json and package-lock.json together to 1.17.1 and commit the accepted release state.
2. Tag that commit v1.17.1 and push to GitHub. The Linux builds workflow packages and attaches AppImage/deb assets to the release.
3. Verify the release assets and the subsequent AUR publish run; confirm the AUR version and source checksum.
4. GitLab contents are synchronized through the connected GitLab (Beta) account; verify matching file trees after each connector sync. Main commit IDs differ because GitLab creates its own sync commit. Terminal Git push access remains unavailable; no additional key setup is needed for connector-based updates.

## Upstream integration scope

The 73 incoming upstream commits were not merged. The scheduled workflow was changed to review reports after upstream replaced Electron with a Windows-native Rust application. Twenty-eight commits remain on the last compatible Electron-era line through 1.19 (including version/website changes). Selected settings polish and optional sound changes are now ported from five Electron commits (2026-11-11); immediate dismissal was skipped by user choice. KDE pointer placement now uses compositor coordinates and was user-confirmed on the current desktop; multiple monitors, fractional scaling and mixed scaling remain untested; do not report the upstream merge conflicts as resolved by a completed merge.
