import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
import ts from 'typescript';

const text = readFileSync(new URL('../src/components/RadialMenu.tsx', import.meta.url), 'utf8');
const ast = ts.createSourceFile('RadialMenu.tsx', text, ts.ScriptTarget.Latest, true, ts.ScriptKind.TSX);
function declaration(name, source = ast) {
  let found;
  function visit(node) {
    if (ts.isVariableDeclaration(node) && node.name.getText(source) === name) found = node.getText(source);
    ts.forEachChild(node, visit);
  }
  visit(source);
  assert.ok(found, `find actual ${name}`);
  return `const ${found};`;
}
const sectors = ts.transpile(readFileSync(new URL('../src/utils/radialSectors.ts', import.meta.url), 'utf8'), { module: ts.ModuleKind.CommonJS });
const code = ts.transpile(declaration('resolveAimAtPoint') + declaration('handleMouseUp') + '\nthis.click = handleMouseUp;', {target:ts.ScriptTarget.ES2022});
class Target { classList = { contains: name => name === 'zn-radial-scrim' }; }
function run(mode, {button = 0, direction = false, empty = false, x = 0, y = -300} = {}) {
  const closed = [];
  const item = { id:'app', label:'Fictional shortcut', type:'app' };
  const context = vm.createContext({
    exports: {}, useCallback: fn => fn,
    stateRef: { current: { isOpen: true, config:{radialSelectionMode:mode}, position:{x:0,y:0},
      currentLevelApps:empty?[]:[item], actualMenuRadius:140, actualIconSize:64, folderStack:[], apps:[item],
      onClose: (...args) => closed.push(args) } },
    aimGateRef: {current:60}, directionModeRef:{current:direction},
    quarantineUntilRef:{current:0}, gestureConsumedRef:{current:false},
    disarmDwell:()=>{}, trackAimPoint:p=>p, logRadialConfirm:()=>{},
    isWorkspacePickItem:()=>false, window:{}, Element:Target,
  });
  vm.runInContext(sectors, context);
  context.sectorIndexForDelta = context.exports.sectorIndexForDelta;
  vm.runInContext(code,context);
  context.click({button,clientX:x,clientY:y,target:new Target()});
  return closed;
}
assert.equal(run('area')[0]?.[0], 'app', 'Area confirms a sector far outside the icon');
assert.equal(run(undefined)[0]?.[0], 'app', 'default mode retains area behavior');
assert.equal(run('cursor')[0]?.[0], null, 'Pointer dismisses a scrim click');
assert.equal(run('cursor',{direction:true})[0]?.[0], 'app', 'direction aiming overrides Pointer');
assert.equal(run('area',{x:0,y:0})[0]?.[0], '__CENTER__', 'hub still closes');
assert.equal(run('area',{empty:true})[0]?.[0], null, 'an empty wheel can still be dismissed');
assert.equal(run('area',{button:1}).length, 0, 'middle release is handled by main only');
assert.equal(run('area',{button:2}).length, 0, 'right click does not launch');
console.log('area-click-smoke: OK (actual aim and mouseup handlers)');

const settingsText = readFileSync(new URL('../src/components/PrecisionSettings.tsx', import.meta.url), 'utf8');
const settingsAst = ts.createSourceFile('PrecisionSettings.tsx', settingsText, ts.ScriptTarget.Latest, true, ts.ScriptKind.TSX);
const triggerCode = ts.transpile(['keyboardTriggerOn','mouseTriggerOn','toggleTrigger'].map(name => declaration(name, settingsAst)).join('\n') + '\nthis.toggle = toggleTrigger; this.mouseOn = mouseTriggerOn;', {target:ts.ScriptTarget.ES2022});
function toggle(config, key, isLinux = true) {
  let next;
  const context = vm.createContext({config,isLinux,setConfig:fn=>{next=fn(config);},showToast:()=>{}});
  vm.runInContext(triggerCode, context);
  context.toggle(key);
  return { next, mouseOn: context.mouseOn };
}
let result = toggle({enableKeyboardTrigger:false,enableMouseTrigger:true,middleClickOpensMenu:false}, 'enableMouseTrigger');
assert.equal(result.mouseOn, false, 'legacy disabled menu gate is shown as off');
assert.equal(result.next.enableMouseTrigger, true);
assert.equal(result.next.middleClickOpensMenu, true, 'one mouse switch re-enables both gates');
result = toggle({enableKeyboardTrigger:true,enableMouseTrigger:false,middleClickOpensMenu:false}, 'enableKeyboardTrigger');
assert.equal(result.next.enableKeyboardTrigger, false);
assert.equal(result.next.enableMouseTrigger, true);
assert.equal(result.next.middleClickOpensMenu, true, 'last-trigger fallback actually enables the mouse route');
result = toggle({enableKeyboardTrigger:false,enableMouseTrigger:true,middleClickOpensMenu:true}, 'enableMouseTrigger');
assert.equal(result.next.enableKeyboardTrigger, true);
assert.equal(result.next.enableMouseTrigger, false);
assert.equal(result.next.middleClickOpensMenu, true, 'disabling the trigger leaves its legacy preference intact');
result = toggle({enableKeyboardTrigger:true,enableMouseTrigger:true,middleClickOpensMenu:false}, 'enableMouseTrigger', false);
assert.equal(result.mouseOn, true, 'Windows keeps independent trigger and menu controls');
assert.equal(result.next.middleClickOpensMenu, false);
console.log('area-click-smoke: trigger settings OK (legacy flags and last-trigger fallback)');
