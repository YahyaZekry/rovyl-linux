import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import path from "node:path";
import vm from "node:vm";

const source = readFileSync(new URL("../backend/electron-main.js", import.meta.url), "utf8");
const start = source.indexOf("  const syncLoginItemSettings = (openAtLogin) => {");
const end = source.indexOf("\n  /** Set by `loadSettings`", start);
assert.ok(start >= 0 && end > start, "find the real login registration function");
const register = source.slice(start, end);
const loginArg = /const LOGIN_LAUNCH_ARG = ("[^"]+");/.exec(source);
assert.ok(loginArg, "find the argument used to detect a login launch");

function run({ desktop, appimage, enabled = true } = {}) {
  const files = new Map();
  const removed = [];
  const context = {
    process: { platform: "linux", env: { APPIMAGE: appimage }, argv: ["/usr/bin/electron", "/checkout"] },
    path,
    os: { homedir: () => "/home/test" },
    fs: {
      readFileSync: () => { if (desktop) return desktop; throw new Error("missing"); },
      mkdirSync: () => {},
      writeFileSync: (file, text) => files.set(file, text),
      unlinkSync: (file) => removed.push(file),
    },
    diagLog: (message) => { throw new Error(message); },
    enabled,
  };
  vm.runInNewContext(`const LOGIN_LAUNCH_ARG = ${loginArg[1]};\n${register}\nsyncLoginItemSettings(enabled);`, context);
  return { files, removed };
}

for (const [options, command] of [
  [{ desktop: '[Desktop Entry]\nExec="/opt/Rovyl/rovyl" --no-sandbox\n' }, '"/opt/Rovyl/rovyl" --no-sandbox'],
  [{ appimage: "/home/test/My Apps/Rovyl.AppImage" }, '"/home/test/My Apps/Rovyl.AppImage"'],
  [{}, '"/usr/bin/electron" "/checkout"'],
]) {
  const text = run(options).files.get("/home/test/.config/autostart/rovyl.desktop");
  assert.ok(text?.includes(`Exec=${command} --opened-at-login\n`), "login launches keep their command and stay in the tray");
}
const disabled = run({ enabled: false });
assert.equal(disabled.files.size, 0);
assert.deepEqual(disabled.removed, ["/home/test/.config/autostart/rovyl.desktop"]);

const handlerStart = source.indexOf('  app.on("second-instance",');
const handlerEnd = source.indexOf("\n  });", handlerStart) + "\n  });".length;
assert.ok(handlerStart >= 0 && handlerEnd > handlerStart);
let handler;
const calls = [];
vm.runInNewContext(`const LOGIN_LAUNCH_ARG = ${loginArg[1]};\n${source.slice(handlerStart, handlerEnd)}`, {
  app: {
    on: (_name, callback) => { handler = callback; },
    whenReady: async () => { calls.push("ready"); },
  },
  ensureMainWindow: async () => { calls.push("ensure-window"); },
  openSettingsFromMainProcess: () => { calls.push("open-settings"); },
  diagLog: () => {},
  console,
});
handler(null, ["rovyl", "--opened-at-login"]);
await new Promise(setImmediate);
assert.deepEqual(calls, [], "a duplicate login launch stays in the tray");
handler(null, ["rovyl"]);
await new Promise(setImmediate);
assert.deepEqual(calls, ["ready", "ensure-window", "open-settings"], "manual relaunch uses the shared Settings reveal path");

const showStart = source.indexOf('ipcMain.on("show-window",');
const showEnd = source.indexOf('\n});', showStart) + '\n});'.length;
assert.ok(showStart >= 0 && showEnd > showStart);
let showHandler;
let finishCreatingWindow;
const windowReady = new Promise((resolve) => { finishCreatingWindow = resolve; });
let reveals = 0;
const win = {
  isDestroyed: () => false,
  show: () => { reveals += 1; },
  focus: () => {},
  setOpacity: () => {},
  webContents: { setBackgroundThrottling: () => {}, focus: () => {}, invalidate: () => {} },
};
const showContext = {
  ipcMain: { on: (_name, callback) => { showHandler = callback; } },
  mainWindow: null,
  windowBuriedPassive: true,
  ensureMainWindow: async () => { await windowReady; showContext.mainWindow = win; return win; },
  applyMousePolicyAfterReveal: () => {},
  diagLog: () => {},
};
vm.runInNewContext(source.slice(showStart, showEnd), showContext);
const pendingReveal = showHandler();
assert.equal(reveals, 0, "a startup reveal waits for the window to be assigned");
finishCreatingWindow();
await pendingReveal;
assert.equal(reveals, 1, "the first renderer request must not be lost during window creation");
await showHandler();
assert.equal(reveals, 2, "later requests still reveal the existing window");
showContext.ensureMainWindow = async () => { throw new Error("creation failed"); };
await showHandler();
assert.equal(reveals, 2, "window creation failure does not reveal a stale window or reject the IPC handler");
console.log("autostart-smoke: OK (login registration, duplicate/manual launch, early reveal, creation failure)");
