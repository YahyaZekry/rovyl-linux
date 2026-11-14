# Linux maintenance release — v1.17.1 preparation

> Updated: 2026-11-13. v1.17.1 is published on GitHub and the AUR package source; permissions-fixed AUR revision 1.17.1-2 was published on 2026-11-14; Chaotic-AUR review is pending.

## Release notes

- Area aiming works across the monitor; KDE Wayland Near mouse placement uses compositor coordinates.
- Settings rows are clickable, scrollbar layout stays stable, and optional wheel sounds have a volume control.
- Multi-monitor, fractional and mixed scaling remain untested; see the README checklist.

- Normal manual launches and login startup stay in the tray. Repeated launches leave the running window state unchanged.
- Settings opens through an explicit Settings action; its reveal waits for window initialization so early requests are not lost. First-install onboarding remains unchanged.
- Desktop launches check launcher exit status, try gtk-launch when gio fails, and recover from a stuck launcher after five seconds.
- Linux mouse-trigger settings offer Wheel, Mouse 4, and Mouse 5. Unsupported saved modifier/button bindings are left intact but disabled instead of silently becoming another button.
- Changing trigger timing preserves the selected binding and its supported gesture mode.

Maintenance: upstream updates now generate a review report instead of automatically merging the Windows-native rewrite. This fork remains a Linux Electron app.

## Previous code CI status (2026-11-11)

For code commit `f90ce7a`, local build/tests/package checks passed and the user confirmed
the fix on the current desktop. Hosted Tests, Linux builds and Container image failed
while downloading system dependencies because package servers could not be resolved
(`security.ubuntu.com` / `archive.ubuntu.com`); these runs did not reach code validation.
Failed jobs were retried once; results remain pending. Wayland E2E was still running
when checked. The older successful run links below do not verify this latest commit.

## v1.17.1 publication (2026-11-13)

- [x] User confirmed quiet startup and the current desktop behavior.
- [x] [GitHub Tests](https://github.com/YahyaZekry/rovyl-linux/actions/runs/43450329952) and [tagged Linux builds](https://github.com/YahyaZekry/rovyl-linux/actions/runs/43451197046) passed; the release has one AppImage and four distro-specific deb files.
- [x] [GitHub release v1.17.1](https://github.com/YahyaZekry/rovyl-linux/releases/tag/v1.17.1) published with display-test limitations in the release notes.
- [x] [AUR publish job](https://github.com/YahyaZekry/rovyl-linux/actions/runs/43451858757) pushed commit `841d814`; the AUR PKGBUILD shows 1.17.1 and its SHA-256 matches the published AppImage. The RPC index initially still returned 1.17.0 immediately after publication.
- [ ] [Chaotic-AUR request #4539](https://github.com/chaotic-aur/packages/issues/4539) remains open and needs triage; no acceptance is claimed.
- [x] AUR permission repair revision `1.17.1-2` published as AUR commit `7a50535`; built the actual package with makepkg and verified archive modes. The AUR RPC index initially still showed `1.17.1-1`; launch on the affected machine remains to be tested.
- Multi-monitor, fractional scaling and mixed scaling remain untested.

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

- [x] User confirmed quiet startup on their desktop (2026-11-13).
- [ ] Launch Rovyl manually, including while already running: no Settings window opens.
- [ ] Choose Open Settings from the tray: Settings opens with working controls.
- [ ] Close Settings and confirm the normal mouse/hotkey wheel still opens, launches a shortcut, and dismisses.

On 2026-11-10 the user explicitly corrected the intended behavior: manual launches must also stay quiet. Removed the automatic Settings opening from initial renderer state and duplicate-instance handling. Startup regressions and full build/AppImage packaging pass; corrected AppImage installed with backup. The explicit Settings reveal guard remains. Real desktop acceptance remains pending; no release tag created.

## Publish after acceptance

1. For the next release, bump package.json and package-lock.json together and commit the accepted release state.
2. Tag that commit and push to GitHub. The Linux builds workflow packages and attaches AppImage/deb assets to the release.
3. Verify the release assets and the subsequent AUR publish run; confirm the AUR version and source checksum.
4. GitLab contents are synchronized through the connected GitLab (Beta) account; verify matching file trees after each connector sync. Main commit IDs differ because GitLab creates its own sync commit. Terminal Git push access remains unavailable; no additional key setup is needed for connector-based updates.

## Upstream integration scope

The 73 incoming upstream commits were not merged. The scheduled workflow was changed to review reports after upstream replaced Electron with a Windows-native Rust application. Twenty-eight commits remain on the last compatible Electron-era line through 1.19 (including version/website changes). Selected settings polish and optional sound changes are now ported from five Electron commits (2026-11-11); immediate dismissal was skipped by user choice. KDE pointer placement now uses compositor coordinates and was user-confirmed on the current desktop; multiple monitors, fractional scaling and mixed scaling remain untested; do not report the upstream merge conflicts as resolved by a completed merge.
