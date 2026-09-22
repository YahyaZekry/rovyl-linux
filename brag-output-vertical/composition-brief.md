# Hyperframes Composition Brief: Rovyl for Linux — Instagram cut (9:16, no music)

Variant of `../brag-output/composition-brief.md`. Same objective, source material,
creative direction, visual identity, and storyboard contract. Deltas:

## Output
- Composition directory: `brag-output-vertical/composition/`
- Rendered video: `brag-output-vertical/brag.mp4`
- Format: vertical 9:16 — 1080x1920
- Duration: 23 seconds

## Audio (changed)
- Audio role: sparse professional accents only — **music omitted at the user's
  request** (Instagram cut). No music file in `assets/`, no volume automation.
- Audio-reactive treatment: **none** — the glow-breathing treatment sampled the
  omitted music track, so it is removed entirely (documented skip, not a failure).
- SFX: unchanged from the master brief (mouse click, tile drops, select tick,
  launch bell, chip rollovers, 5 calibration ticks, threshold impact, logo bell),
  same timestamps. All local under `assets/sfx/`.

## Copy that must appear verbatim (changed hook)
- "One gesture," / "every app on your Linux desktop." (hook, user-supplied)
- "Hold it anywhere."
- "Aim. Release. Launched."
- "Born on Wayland." / "A kernel-level gesture engine. No compositor plugins."
- "Calibrate from your own clicks" / "Settings → Mouse" / "your threshold · 350 ms"
- "One gesture. Any destination." / "Rovyl" / "Free & open source · Linux"
- Tooltip pill: "Spotify" · Wheel chip: "Main" + "Esc"

## Layout adaptations (9:16)
- Wheel center (540, 850), radius 200, 104px tiles; aim target (music tile) at x 340.
- Hook and outro text centered; Wayland scene left-anchored at x 90 with chips
  wrapping 2x2; calibration meter 900px wide at x 90 with threshold at x 463;
  captions in the lower third ("Hold it anywhere." at y 1680).
- Persistent orange glow from the bottom-left, faint counter-glow top-right, vignette.

## Hyperframes Instructions
Unchanged from the master brief: load the five Hyperframes domain skills, /brag
owns the workflow, readability beats the grid, `hyperframes check` is the single
pre-render gate. With no music present, beat-sync and audio-reactive requirements
are waived for this run (documented above).
