#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/src" "$tmp/pkg"
source "$root/aur/PKGBUILD"
cat > "$tmp/src/rovyl-${pkgver}.AppImage" <<'MOCK'
#!/usr/bin/env bash
set -euo pipefail
[[ "$1" == --appimage-extract ]]
mkdir -p squashfs-root/resources/app.asar.unpacked/backend
printf '#!/bin/sh\nexit 0\n' > squashfs-root/rovyl
printf '#!/bin/sh\nexit 0\n' > squashfs-root/resources/app.asar.unpacked/backend/rovyl-helper-linux
printf 'data' > squashfs-root/resources/app.asar
printf 'icon' > squashfs-root/rovyl.png
printf '[Desktop Entry]\nExec=old\nIcon=old\n' > squashfs-root/rovyl.desktop
chmod 700 squashfs-root squashfs-root/resources squashfs-root/resources/app.asar.unpacked squashfs-root/resources/app.asar.unpacked/backend
chmod 700 squashfs-root/rovyl squashfs-root/resources/app.asar.unpacked/backend/rovyl-helper-linux
chmod 600 squashfs-root/resources/app.asar
MOCK
chmod 755 "$tmp/src/rovyl-${pkgver}.AppImage"
srcdir="$tmp/src" pkgdir="$tmp/pkg" package
for path in opt/Rovyl opt/Rovyl/resources opt/Rovyl/resources/app.asar.unpacked opt/Rovyl/resources/app.asar.unpacked/backend opt/Rovyl/rovyl opt/Rovyl/resources/app.asar.unpacked/backend/rovyl-helper-linux; do
  mode=$(stat -c '%a' "$tmp/pkg/$path")
  (( (8#$mode & 005) == 005 )) || { printf 'Not accessible to regular users: %s (%s)\n' "$path" "$mode" >&2; exit 1; }
done
mode=$(stat -c '%a' "$tmp/pkg/opt/Rovyl/resources/app.asar")
(( (8#$mode & 004) == 004 ))
test -x "$tmp/pkg/usr/bin/rovyl"
printf 'aur-package-smoke: OK (regular users can traverse, read and execute)\n'

if command -v vercmp >/dev/null; then
  awk '/^          CURRENT_VER=/{copy=1} /^          cp \.\.\/aur\/PKGBUILD/{exit} copy{print}' "$root/.github/workflows/aur-publish.yml" > "$tmp/guard.sh"
  test -s "$tmp/guard.sh"
  mkdir -p "$tmp/current"
  check_revision() {
    printf 'pkgver=%s\npkgrel=%s\n' "$1" "$2" > "$tmp/current/PKGBUILD"
    VER="$3" REL="$4" bash -c 'cd "$1"; source "$2"' _ "$tmp/current" "$tmp/guard.sh"
  }
  check_revision 1.17.1 1 1.17.1 2
  check_revision 1.17.1 2 1.17.2 1
  if check_revision 1.17.1 2 1.17.1 1 >/dev/null 2>&1; then
    echo 'AUR revision downgrade was not rejected' >&2; exit 1
  fi
  if check_revision 1.17.2 1 1.17.1 2 >/dev/null 2>&1; then
    echo 'AUR version downgrade was not rejected' >&2; exit 1
  fi
  echo 'aur-package-smoke: OK (published versions cannot be downgraded)'
fi
