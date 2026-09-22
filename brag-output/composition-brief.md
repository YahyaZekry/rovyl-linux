# Hyperframes Composition Brief: Rovyl for Linux

## Objective
Create a short launch-style brag video for Rovyl, a radial launcher for Linux.

## Output
- Composition directory: `brag-output/composition/`
- Rendered video: `brag-output/brag.mp4`
- Format: landscape — 1920x1080
- Duration: 22.5 seconds

## Source Material
- Project root: `/home/monst3r/Projects/rovyl`
- Primary files read: `README.md`, `src/index.css`, `src/fonts.css`, `src/RadialApp.tsx`,
  `docs/media/wheel.png` (real product screenshot), `package.json`
- Product name: Rovyl
- Tagline / strongest claim: "One gesture. Any destination." — hold the middle mouse
  button, aim, release; works on Wayland and X11, no compositor plugins.
- Key UI or visual moment to recreate: the radial wheel blooming over the dark desktop,
  then the aim-drag with the orange tooltip pill locking onto the music tile (as in the
  real screenshot `docs/media/wheel.png`).
- Copy that must appear verbatim:
  - "Your middle mouse button has one job."
  - "Closing tabs."
  - "Hold it anywhere."
  - "Aim. Release. Launched."
  - "Born on Wayland."
  - "Kernel-level. No compositor plugins."
  - "Calibrate from your own clicks"
  - "your threshold · 350 ms"
  - "One gesture. Any destination."
  - "Rovyl"
  - "Free & open source · Linux"
  - Tooltip pill label: "Spotify"
  - Wheel chip: "Main · Esc"

## Creative Direction
- Tone preset: cinematic
- Creative direction: "gesture trailer — the most wasted button on your mouse becomes a launcher"
- Interpretation: wide dark frames, big Space Grotesk type, dramatic but restrained
  motion. The wheel bloom is the single big effects moment; everything else stays dry
  and confident. Fast-in then hold on all text.
- Angle: the middle mouse button has one boring job; Rovyl turns it into the fastest
  path to anything, without taking the native click away.
- Hook: "Your middle mouse button has one job." → beat → "Closing tabs."
- Outro / punchline: "Rovyl — One gesture. Any destination." over a dim wheel bloom.
- Avoid:
  - Generic SaaS language
  - Abstract filler visuals
  - Unrelated visual redesign

## Visual Identity
- Background: `#0a0a0b` with an ambient radial orange glow from the bottom-left
  (`#e85d04` → transparent), matching the real desktop wallpaper in the product
  screenshot; scenes 2–3 carry it strongest, text scenes keep it faint.
- Tiles/surfaces: `#151515` fill, `rgba(255,255,255,.055)` raised overlay, hairline
  border `rgba(255,255,255,.13)`, corner radius ~14px, soft deep shadow.
- Text: `rgba(255,255,255,.93)` primary, `rgba(255,255,255,.55)` secondary.
- Accent: orange tooltip pill (`#ff7a1a` background, white label, pill radius) — the
  real aim tooltip; `#1db954` green only for the music tile glyph and launch check.
- Display font: Space Grotesk Variable (local woff2 provided in `assets/fonts/`).
- Body/UI font: Inter Variable (local woff2 provided); wheel labels: Instrument Sans
  Variable (local woff2 provided).
- Visual references from the project: `docs/media/wheel.png` (copied to
  `assets/img/wheel.png`) — hub-and-spoke wheel, 6–8 dark icon tiles, orange "Spotify"
  pill left of the aimed tile, "Main · Esc" chip below the wheel. Icons are Lucide
  glyphs in the real app; use inline SVG Lucide-style glyphs (terminal, globe,
  gamepad, mail, folder, music, github, chrome).

## Storyboard
Use the storyboard in `brag-output/brag-plan.md` as the creative contract.

Scene summary:
1. Hook — 3.3s — "Your middle mouse button has one job." / "Closing tabs."
2. The bloom — 5.4s — press ring → hub → 8 tiles pop on the beat grid; "Hold it anywhere."
3. Aim & release — 3.8s — drag to music tile, orange "Spotify" pill locks, burst + check; "Aim. Release. Launched."
4. Born on Wayland — 3.5s — slam line, sub-line, 4 desktop chips (KDE Plasma, Hyprland, sway, X11)
5. Calibrate — 3.6s — 5 press bars (3 fast, 2 slow) on a timeline, threshold line drops, "your threshold · 350 ms"
6. Outro — 2.9s — dim wheel bloom behind "Rovyl" + "One gesture. Any destination." + "Free & open source · Linux"

## Audio
- Audio role: cinematic support — steady clean bed under a few big physical accents.
- Audio arc: near-dry hook → bed rises into the tile bloom → bell on the launch payoff →
  tactile ticks in calibration → final bell under the fading logo.
- Music: `happy-beats-business-moves-vol-12-by-ende-dot-app.mp3` (copied to
  `assets/music/`).
- Music treatment: start 0.0, volume ~0.25, gentle fade-in over ~0.5s, fade out across
  the outro (22.5s end).
- Music cue guidance: bundled preset
  `happy-beats-business-moves-vol-12-by-ende-dot-app.music-cues.json` (in the brag
  skill's `assets/music/cues/`; copy next to the track if needed). ~110 BPM, beat grid
  ≈ 0.55s apart. Strong cues: 8.74, 10.93, 13.11, 17.47, 18.56, 19.66, 22.37. Lock the
  launch payoff near 10.93 and the logo near 22.37; walk the tile pops along the beat
  grid from ~4.9s.
- Audio-reactive treatment: subtle — the orange ambient glow breathes with music RMS;
  optional faint glow swell on the logo at strong moments. No waveform/equalizer
  visuals.
- Audio-coupled moments:
  - Middle-click press (scene 2) — mouse click sound with the press ring
  - Tile pops (scene 2) — soft drops on the beat grid; accent first + last, not all 8
  - Tooltip lock + release (scene 3) — selection tick, then one bell at the launch
  - Chip pops (scene 4) — light accents on nearby beats
  - Calibration presses (scene 5) — 5 click ticks; soft impact on the threshold drop
  - Logo landing (scene 6) — one bell, then fade
- SFX selection guidance: use the brag skill's `assets/sfx/sfx-analysis.md`; prefer low
  high-frequency-risk files for repeated ticks (calibration presses).
- Exact SFX choice: Hyperframes should choose filenames, timestamps, density, and
  volume based on the implemented animation.
- Audio files: copy the chosen music and any Hyperframes-selected SFX into
  `brag-output/composition/assets/`.

## Hyperframes Instructions
Load the composition-building Hyperframes domain skills — `hyperframes-core`,
`hyperframes-animation`, `hyperframes-creative`, `hyperframes-keyframes`, and
`hyperframes-cli`. /brag is its own workflow: do not enter the `hyperframes`
entry-point intent interview and do not route into its generic promo / launch-video
workflow. Prefer native Hyperframes conventions over anything in `/brag`.

Requirements:
- Show at least one real UI, copy, or visual element from the source project — the
  recreated radial wheel (hub, tiles, tooltip pill, "Main · Esc" chip) is the
  centerpiece and must match the app's palette, radius, and fonts as specified above.
- Keep all text readable in the final render; fast-in then hold.
- Keep the video within 15-25 seconds (plan: 22.5s).
- Include the planned music/SFX layer.
- Treat `/brag` audio notes as guidance, not a fixed cue sheet. Choose SFX after the
  visual animation exists.
- Treat music cue metadata as optional timing hints; readability and the product story
  win over the grid.
- Major reveals may move toward nearby strong cues within about 0.15s; smaller
  entrances within about 0.10s. Use only 1-3 strong cue locks.
- Use SFX to support motion and interaction; restraint when the edit is busy.
- Honor the planned music treatment (fade-in, fade under final logo).
- When music is present, consider the Hyperframes audio-reactive workflow: extract
  audio data and use RMS energy for the ambient glow's subtle breathing. If extraction
  is unavailable, note it in the brief and skip audio-reactive — do not block the
  render.
- Use local assets for audio and fonts (all provided under `assets/`).
- Run `hyperframes check` before render — it is brag's single gate.
