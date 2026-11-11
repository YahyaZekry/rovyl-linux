import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
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
const writes = [];
const timers = new Map();
let nextTimer = 0;
const state = vm.createContext({
  isWaylandNative: true, waylandCursorPoint: null, waylandCursorWaiters: null,
  radialFollowsCursorMonitor: false, radialOpensAtCursor: false, radialFullBleed: false,
  radialMouseBlockerReady: true,
  radialMouseBlocker: { stdin: { writable: true, write: command => writes.push(command) } },
  screen: { getPrimaryDisplay: () => primary, getDisplayNearestPoint: p => p.x >= 1920 ? secondary : primary,
    getCursorScreenPoint: () => { throw new Error('Wayland must use the helper'); } },
  setTimeout: fn => { const id = ++nextTimer; timers.set(id, fn); return id; },
  clearTimeout: id => timers.delete(id),
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
assert.equal(state.queryWaylandCursor(), first, 'concurrent opens share one request');
assert.deepEqual(writes, ['CURSORQ\n']);
state.waylandCursorWaiters.resolve({ x: 2200, y: 500 });
assert.equal((await first).x, 2200);
assert.equal(timers.size, 0, 'reply cancels timeout');
const second = state.queryWaylandCursor();
assert.equal(writes.length, 2, 'cached coordinates do not suppress fresh queries');
[...timers.values()][0]();
assert.equal((await second).x, 2100, 'timeout falls back to last coordinates');
assert.equal(state.waylandCursorWaiters, null);
state.radialMouseBlocker.stdin.write = () => { throw new Error('closed'); };
assert.equal((await state.queryWaylandCursor()).x, 2100);
assert.equal(state.waylandCursorWaiters, null);
assert.equal(timers.size, 0);
state.radialOpensAtCursor = true;
assert.equal(state.radialOpenBounds(secondary).width, 1920, 'Wayland draws within a monitor surface');
state.isWaylandNative = false;
assert.equal(state.radialOpenBounds(secondary).width, 600, 'other platforms retain compact windows');
state.screen.getCursorScreenPoint = () => ({ x: 2500, y: 200 });
assert.equal(state.currentCursorPoint().x, 2500);
console.log('wayland-pointer-smoke: OK (fresh queries, timeout, display selection and placement)');
