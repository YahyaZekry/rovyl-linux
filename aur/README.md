# AUR / Chaotic-AUR publishing

`PKGBUILD` + `.SRCINFO` are the source of truth for the `rovyl-bin` AUR package.

## Release flow

1. Push a tag: `git tag v1.x.x && git push --tags`
2. `build-linux.yml` builds deb + AppImage and attaches them to the GitHub Release
3. `aur-publish.yml` (runs on release published) downloads the AppImage,
   recomputes `pkgver`/`sha256sums`, regenerates `.SRCINFO`, and pushes to
   `ssh://aur@aur.archlinux.org/rovyl-bin.git`

## One-time setup

1. Create the `rovyl-bin` package page at https://aur.archlinux.org and push the
   initial `PKGBUILD`/`.SRCINFO` from this folder to its git remote
2. Add an SSH private key for your AUR account as the GitHub secret
   `AUR_SSH_PRIVATE_KEY` (Settings → Secrets → Actions)

## Local update (manual fallback)

```bash
cd aur
# edit pkgver, refresh sha256sums:
updpkgsums
makepkg --printsrcinfo > .SRCINFO
```

## Chaotic-AUR

Chaotic-AUR's package list now lives on GitLab (`gitlab.com/chaotic-aur/pkgbuilds`,
GitHub `chaotic-aur/pkgbuilds` is archived, `chaotic-aur/packages` is a push-only mirror).

- One-time: fork `gitlab.com/chaotic-aur/pkgbuilds`, add a `rovyl-bin/` folder with
  `PKGBUILD`, `.SRCINFO`, and `.CI/config` (`CI_PKGBUILD_SOURCE=aur` +
  `CI_PKGBUILD_TIMESTAMP`), and open an MR.
- After that **no per-release PR is needed**: their CI checks the AUR every ~30 min
  and rebuilds automatically when `rovyl-bin` updates.

## Current version

The AUR package follows the GitHub Release version. v1.17.0 is the first release
published by `aur-publish.yml` (the initial import was a manual v1.16.0 PKGBUILD).
