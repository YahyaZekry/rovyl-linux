import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
const source = readFileSync(new URL('../backend/electron-main.js', import.meta.url), 'utf8');
const names = ['applyRadialMonitorSetting', 'currentCursorPoint', 'radialTargetDisplay', 'queryWaylandCursor', 'radialOpenBounds'];
const functions = names.map(name => {
  const start = source.indexOf(`function ${name}(`);
  const end = source.indexOf('\n}', start) + 2;
  assert.ok(start >= 0 && end > start, name);
  return source.slice(start, end);
}).join('\n');
const primary = { id: 1, bounds: { x: 0, y: 0, width: 1920, height: 1080 } };
const secondary = { id: 2, bounds: { x: 1920, y: 0, width: 1920, height: 1080 } };
const calls = [];
const state = vm.createContext({
  isWaylandNative: true, waylandCursorPoint: null, waylandCursorWaiters: null,
  radialFollowsCursorMonitor: false, radialOpensAtCursor: false, radialFullBleed: false,
  getNativeHelperExePath: () => '/helper',
  execFile: (file, args, options, callback) => calls.push({file,args,options,callback}),
  screen: { getPrimaryDisplay: () => primary, getDisplayNearestPoint: p => p.x >= 1920 ? secondary : primary,
    getCursorScreenPoint: () => { throw new Error('Wayland must use compositor coordinates'); } },
  fullBleedBounds: bounds => bounds, radialModeBounds: () => ({ width: 600, height: 600 }),
});
vm.runInContext(functions, state);
state.applyRadialMonitorSetting('cursor');
assert.equal(state.radialFollowsCursorMonitor, true);
assert.equal(state.currentCursorPoint(), null);
assert.equal(state.radialTargetDisplay().id, 1);
state.waylandCursorPoint = { x: 2100, y: 400 };
assert.equal(state.radialTargetDisplay().id, 2);
const first = state.queryWaylandCursor();
assert.equal(state.queryWaylandCursor(), first, 'concurrent opens share one compositor query');
assert.equal(calls.length, 1);
assert.equal(calls[0].args[0], 'cursor-query');
assert.equal(calls[0].options.timeout, 2000);
calls[0].callback(null, 'CURSOR 2200 500\n');
assert.equal((await first).x, 2200);
assert.equal(state.waylandCursorWaiters, null);
const second = state.queryWaylandCursor();
assert.equal(calls.length, 2, 'query remains fresh after a successful read');
calls[1].callback(new Error('timed out'), '');
assert.equal(await second, null, 'failed reads never reuse a drifting or stale point');
assert.equal(state.currentCursorPoint(), null);
assert.equal(state.radialTargetDisplay().id, 1, 'unavailable compositor falls back to primary');
for (const output of ['CURSOR NaN 1', 'CURSOR 999999999999999999999 0', 'untrusted output', 'CURSOR 1 2\nextra']) {
  const query = state.queryWaylandCursor();
  calls.at(-1).callback(null, output);
  assert.equal(await query, null);
}
state.getNativeHelperExePath = () => null;
assert.equal(await state.queryWaylandCursor(), null);
state.radialOpensAtCursor = true;
assert.equal(state.radialOpenBounds(secondary).width, 1920, 'Wayland draws within a monitor surface');
state.isWaylandNative = false;
assert.equal(state.radialOpenBounds(secondary).width, 600, 'other platforms retain compact windows');
state.screen.getCursorScreenPoint = () => ({ x: 2500, y: 200 });
assert.equal(state.currentCursorPoint().x, 2500);
console.log('wayland-pointer-smoke: OK (compositor queries, failure fallback, validation and placement)');

const helper = fileURLToPath(new URL('../backend/rovyl-helper-linux', import.meta.url));
const missing = spawnSync(helper, ['cursor-query'], {encoding:'utf8',timeout:2500,env:{...process.env,DBUS_SESSION_BUS_ADDRESS:'unix:path=/no-such-rovyl-session-bus'}});
assert.equal(missing.status, 1, 'missing compositor/bus exits without a guessed position');
assert.equal(missing.stdout, '');
if (/kde/i.test(process.env.XDG_CURRENT_DESKTOP || '') && process.env.DBUS_SESSION_BUS_ADDRESS) {
  const live = spawnSync(helper, ['cursor-query'], {encoding:'utf8',timeout:2500});
  assert.equal(live.status, 0, live.stderr);
  assert.match(live.stdout, /^CURSOR -?\d+ -?\d+\n$/);
  console.log('wayland-pointer-smoke: live KDE compositor query OK');
}

let answer;
let held = true;
let targetReads = 0;
const openStart = source.indexOf('function showMenuAtCursor(');
const openEnd = source.indexOf('\n}', openStart) + 2;
const opening = vm.createContext({
  ensureOverlayWindow:async()=>({isDestroyed:()=>false}),cancelIdleMemoryCleanup:()=>{},
  isWaylandNative:true,radialOpensAtCursor:true,radialFollowsCursorMonitor:true,
  queryWaylandCursor:()=>new Promise(resolve=>{answer=resolve;}),waylandCursorPoint:null,
  radialTargetDisplay:()=>{targetReads++;throw new Error('cancelled gesture must not open');},
});
vm.runInContext(source.slice(openStart,openEnd), opening);
opening.showMenuAtCursor('mmb',()=>held);
await new Promise(setImmediate);
held = false;
answer({x:100,y:100});
await new Promise(setImmediate);
assert.equal(targetReads, 0, 'release during compositor query cancels the delayed opening');
console.log('wayland-pointer-smoke: cancelled hold does not reopen');
