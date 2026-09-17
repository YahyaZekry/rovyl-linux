<!-- What does this PR change? One or two sentences. -->

## Platform

- [ ] Linux · Wayland
- [ ] Linux · X11
- [ ] Windows (unchanged? say so explicitly — the Electron upgrade left it untested)

## Checks

- [ ] `npm run build` passes (it runs the radial-windowing handshake verifier — if it fails, the handshake was broken, not the test)
- [ ] Smoke tests pass (`npm run test:*` that apply to the touched area)
- [ ] Helper changes: `scripts/e2e/run-gesture-e2e.sh` passes (needs `input` group + uinput)
- [ ] The helper's stdin/stdout line protocol is unchanged, or this PR documents the extension

## Notes for the reviewer

<!-- Anything non-obvious: why the change, what you ruled out, known gaps. -->
