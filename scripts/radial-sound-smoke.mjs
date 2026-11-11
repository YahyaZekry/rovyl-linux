import assert from 'node:assert/strict';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { pathToFileURL } from 'node:url';
import { build } from 'vite';

const outDir = mkdtempSync(join(tmpdir(), 'rovyl-sound-'));
try {
  await build({ logLevel: 'warn', build: {
    ssr: 'src/utils/radialSound.ts', outDir, emptyOutDir: true, target: 'node20',
    rollupOptions: { output: { format: 'es', entryFileNames: 'entry.mjs' } },
  } });
  const sound = await import(pathToFileURL(join(outDir, 'entry.mjs')).href);
  const { resolveRadialSounds, normalizeRadialSound, normalizeRadialVolume, noteForHighlight, HUB_TARGET } = sound;
  assert.deepEqual(resolveRadialSounds({}), { open: null, hover: null });
  assert.deepEqual(resolveRadialSounds({ radialSounds: false }), { open: null, hover: null });
  const enabled = resolveRadialSounds({ radialSounds: true });
  assert.deepEqual(enabled, { open: 'sub-tick', hover: 'thump' });
  assert.deepEqual(resolveRadialSounds({ radialSounds: true, radialOpenSound: false, radialHoverSoundId: 'pluck' }), { open: null, hover: 'pluck' });
  for (const value of ['constructor', '__proto__', 'future-sound', null, 3]) {
    assert.equal(normalizeRadialSound(value, 'thump'), 'thump');
  }
  for (const { id } of sound.RADIAL_SOUNDS) assert.equal(normalizeRadialSound(id, 'thump'), id);
  for (const [input, expected] of [[undefined,100], [NaN,100], [Infinity,100], ['50',100], [-5,0], [120,100], [49.8,50], [0,0]]) {
    assert.equal(normalizeRadialVolume(input), expected);
  }
  assert.equal(noteForHighlight('app', true, 149, enabled), null);
  assert.equal(noteForHighlight(null, true, 300, enabled), null);
  assert.equal(noteForHighlight(HUB_TARGET, false, 300, enabled), null);
  assert.equal(noteForHighlight(HUB_TARGET, true, 300, enabled), 'sub-tick');
  assert.equal(noteForHighlight('app', false, 150, enabled), 'thump');
  assert.equal(noteForHighlight('app', true, 300, resolveRadialSounds({})), null);
  console.log('radial-sound-smoke: OK (defaults, validation, volume and highlight rules)');
} finally { rmSync(outDir, { recursive: true, force: true }); }
