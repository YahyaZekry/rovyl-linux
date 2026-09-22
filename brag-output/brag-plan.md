# Brag Plan: Rovyl for Linux

## What is this app?
Rovyl is a radial launcher for Linux: hold the middle mouse button anywhere and a wheel
of your shortcuts blooms on screen — aim, release, launched. It works over Wayland and
X11 via a tiny C evdev helper that reads the mouse at the kernel level and re-injects
events through a virtual pointer. No compositor plugins.

## The angle
The middle mouse button is the most wasted button on your mouse — one job, closing tabs.
Rovyl turns it into a launcher without taking that job away (fast clicks stay native,
deliberate holds open the wheel, and the app calibrates that boundary from your own
clicks). The video is a gesture trailer: the boring button becomes the fastest path to
anything on the desktop. Every visual is the real product UI — the wheel tiles, the
orange aim tooltip, the "Main · Esc" chip, the calibration press meter — recreated in
the app's exact palette and fonts.

## Hook (first 2-3 seconds)
Black screen. Line slams in: "Your middle mouse button has one job." Beat. Second line,
dry: "Closing tabs." — then the cursor moves in and holds the button anyway.

## Key moments (the middle)
- The bloom: middle-click press (ring pulse) → center hub scales in → 8 shortcut tiles
  pop out radially one by one on the beat grid — dark rounded tiles, white glyphs,
  hairline borders, exactly like the real wheel.
- Aim & release: cursor drags toward the green music tile; the tile swells, an orange
  pill tooltip ("Spotify") chases the cursor like in the real screenshot; release → the
  tile bursts and a "launched" check pops. Copy: "Aim. Release. Launched."
- Born on Wayland: slam line + four desktop chips (KDE Plasma, Hyprland, sway, X11) and
  the claim "Kernel-level. No compositor plugins."
- Calibrate from your own clicks: five real press bars on a timeline — three short
  (fast) presses, two long (slow) presses — then a threshold line drops between them:
  "your threshold · 350 ms". This is a real Settings → Mouse feature.

## Outro / punchline
The wheel blooms once more, small and dim, behind the logo. "Rovyl" in Space Grotesk,
tagline verbatim: "One gesture. Any destination." Small line: "Free & open source · Linux".

## User flow worth showing
Press-and-hold the middle button anywhere → the wheel blooms → aim toward a slice
(tooltip pill follows) → release → the app launches. Then the setup flow: five presses
→ threshold learned. The centerpiece scenes ARE this flow.

## Tone
- Preset: cinematic
- Creative direction: "gesture trailer — the most wasted button on your mouse becomes a launcher"
- Interpretation: wide dark frames, big display type, dramatic but restrained motion;
  the wheel reveal is the one big effects moment; copy stays dry and confident.

## Format: landscape — 1920x1080
## Duration: 22.5s

## Visual identity (from the project)
- Background: near-black `#0a0a0b` with the real desktop's ambient orange glow
  (radial, from bottom-left, `#e85d04` → transparent) — matches the wallpaper in the
  real wheel screenshot `docs/media/wheel.png`.
- Surface/tiles: `#151515` with `rgba(255,255,255,.055)` raise, hairline border
  `rgba(255,255,255,.13)`, rounded ~14px (from `src/index.css`).
- Text: `rgba(255,255,255,.93)`, secondary `rgba(255,255,255,.55)`.
- Accent: orange tooltip pill (Breeze orange `#ff7a1a` family, white label) — the real
  aim tooltip; Spotify green `#1db954` only for the music tile/launch payoff.
- Display font: Space Grotesk Variable (bundled in the app, `--font-display`).
- Body/UI font: Inter Variable (`--font-ui`); wheel labels: Instrument Sans Variable
  (`--font-radial`).
- Strongest visual element: the radial wheel itself — hub, spokes, dark icon tiles,
  orange tooltip pill, "Main · Esc" chip. Reference: `docs/media/wheel.png`.

## Share copy (draft)
Your middle mouse button has one job. Rovyl gives it a wheel: hold, aim, release —
anything launches, on Wayland and X11. No compositor plugins. Free & open source.

## Audio direction
- Role: cinematic support — a steady, clean bed under a few big, physical accents.
- Music: `happy-beats-business-moves-vol-12-by-ende-dot-app.mp3` (117s, ~110 BPM — the
  steady/clean track, listed best for cinematic). Volume ~0.25, gentle fade-in, fade
  under the final logo.
- Music treatment: start at 0.0; let the bloom, the launch payoff, and the logo land on
  strong cues; duck nothing (no voiceover).
- Music cue guidance: bundled preset at
  `<skill-dir>/assets/music/cues/happy-beats-business-moves-vol-12-by-ende-dot-app.music-cues.json`
  — ~110 BPM, beat grid ≈ every 0.55s. Strong cues in window: 8.74, 10.93, 13.11,
  17.47, 18.56, 19.66, 22.37. Plan: tile pops walk the beat grid (~4.9s→8.7s, every
  beat — icons, not read-text); launch payoff beat-locked near 10.93; logo beat-locked
  near 22.37. Chip/press sequences snap to nearby beats.
- Audio-reactive treatment: subtle — the orange ambient glow breathes with music RMS;
  the logo gets a faint glow swell on strong moments. No waveforms, no equalizers.
- SFX posture: sparse-moderate, cinematic (2-3 big ones): a real mouse click on the
  press, soft pops for tile arrivals (accent first/last, not all eight), a selection
  tick as the aim settles, one bell hit on the launch payoff, one on the logo.
- Audio-coupled moments: middle-click press (mouse click sfx), tile pops (soft drops on
  beat grid), tooltip lock + release (select + bell), calibration presses (5 key/click
  ticks), threshold drop (soft impact), logo landing (bell + fade).
- Restraint rule: SFX never stack on top of each other; no sound during pure text holds
  unless it lands with a visual event.

## Storyboard

### Scene 1 — Hook — 3.3s (0.0–3.3)
Black frame, faint orange glow bottom-left. "Your middle mouse button has one job."
slams in big (display type), holds. Beat. Second line, smaller and dry: "Closing tabs."
A cursor glyph slides in from the right edge during the second line.
Sequential/interaction: none.
Audio intent: near-silence with the bed fading in; dry and confident.
Audio-coupled idea: none — let the type carry it.
Music: low, steady bed fading in.
Transition mood: hard cut → Scene 2.

### Scene 2 — The bloom — 5.4s (3.3–8.7)
The recreated desktop: near-black with the orange ambient glow. The cursor reaches
center, a press ring pulses around it (middle-click sfx), the center hub scales in,
then 8 dark tiles pop outward one by one on the beat grid, spokes fading in behind
them. "Main · Esc" chip fades in below. Text bottom-left: "Hold it anywhere."
Sequential/interaction: yes — 8 tile pops in radial order on the beat grid; hub first;
press ring first of all.
Audio intent: build — each arrival adds a little energy, ending on the full wheel.
Audio-coupled idea: tile pops on the beat grid (accent first + last with soft drops);
press ring with a real mouse click.
Music: bed established; energy rising into the pops.
Transition mood: clean carry-over (wheel stays put, text changes) → Scene 3.

### Scene 3 — Aim & release — 3.8s (8.7–12.5)
The cursor drags from center toward the left tile (green music glyph). The tile swells
and brightens; the orange pill tooltip ("Spotify") chases the cursor and locks onto the
tile — exactly the real screenshot's moment. Copy center-top: "Aim. Release. Launched."
On release the tile bursts softly and a check mark pops in the pill.
Sequential/interaction: yes — simulated hold-drag-release; tooltip follows the cursor.
Audio intent: tension into a small, satisfying payoff.
Audio-coupled idea: selection tick as the tooltip locks; one bell hit on the release/
launch (beat-locked near 10.93).
Music: bed continues.
Transition mood: quick wipe → Scene 4.

### Scene 4 — Born on Wayland — 3.5s (12.5–16.0)
Text-forward frame on the dark/glow background. Slam: "Born on Wayland." Sub-line:
"Kernel-level. No compositor plugins." Four chips pop in on beats below: KDE Plasma ·
Hyprland · sway · X11 — and hold as a full set.
Sequential/interaction: yes — 4 chips pop quickly then hold.
Audio intent: confident, declarative.
Audio-coupled idea: chip pops on nearby beats (short labels revealed fast, set held).
Music: bed continues.
Transition mood: hard cut → Scene 5.

### Scene 5 — Calibrate from your own clicks — 3.6s (16.0–19.6)
The real calibration meter: a horizontal timeline. Five press bars drop in — three
short (fast presses: tick, tick, tick), two long (slow presses) — then a vertical
threshold line drops between the groups with the label "your threshold · 350 ms".
Copy top: "Calibrate from your own clicks" (real settings label).
Sequential/interaction: yes — five presses land one by one, then the threshold.
Audio intent: tactile, metronomic ticks, then a small landing.
Audio-coupled idea: a click tick per press bar; soft impact on the threshold line.
Music: bed continues.
Transition mood: dramatic wipe → Scene 6.

### Scene 6 — Outro — 2.9s (19.6–22.5)
The wheel blooms once more, small and dim, behind the logo. "Rovyl" lands big (Space
Grotesk), tagline verbatim below: "One gesture. Any destination." Small line: "Free &
open source · Linux". Bell hit, everything fades to black.
Sequential/interaction: none.
Audio intent: resolve — one bell, then quiet.
Audio-coupled idea: logo landing beat-locked near 22.37 with the bell.
Music: bed fades under the logo.
Transition mood: fade to black.

**Music mood for this video:** cinematic-clean (steady bed, physical accents)
**Audio summary:** a quiet dry hook, a beat-grid tile bloom, one bell on the launch
payoff, tactile ticks in calibration, and a final bell under the fading logo.

## Music cue guidance
Track: happy-beats-business-moves-vol-12 (~110 BPM). Preset cue JSON bundled with the
brag skill (path above). Strong-cue locks: launch payoff near 10.93s, logo near 22.37s.
Beat grid for tile pops (~0.55s apart) from ~4.9s; chips and press bars snap to nearby
beats. Text holds override the grid whenever readability demands it.
