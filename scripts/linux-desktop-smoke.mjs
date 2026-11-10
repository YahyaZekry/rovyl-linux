import assert from "node:assert/strict";
import { createRequire } from "node:module";
import { mkdtempSync, mkdirSync, writeFileSync, readFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

const require = createRequire(import.meta.url);
const { launchDesktopId } = require("../backend/linux-desktop.cjs");
const dir = mkdtempSync(join(tmpdir(), "rovyl-desktop-launch-"));
const savedEnv = { ...process.env };
try {
  const bin = join(dir, "bin");
  mkdirSync(bin);
  mkdirSync(join(dir, "applications"));
  const id = `rovyl-test-${process.pid}`;
  writeFileSync(join(dir, "applications", `${id}.desktop`), "[Desktop Entry]\nName=Test\nExec=unused\n");
  process.env.PATH = bin;
  process.env.XDG_DATA_HOME = dir;
  process.env.XDG_DATA_DIRS = dir;
  process.env.ROVYL_LAUNCH_TEST_LOG = join(dir, "calls");

  async function scenario(gio, gtk, expected, commands) {
    writeFileSync(process.env.ROVYL_LAUNCH_TEST_LOG, "");
    for (const [name, code] of [["gio", gio], ["gtk-launch", gtk]]) {
      const file = join(bin, name);
      rmSync(file, { force: true });
      if (code !== null) writeFileSync(file, `#!/bin/sh\nprintf '%s\\n' '${name}' >> "$ROVYL_LAUNCH_TEST_LOG"\nexit ${code}\n`, { mode: 0o755 });
    }
    assert.equal(await launchDesktopId(id), expected);
    assert.equal(readFileSync(process.env.ROVYL_LAUNCH_TEST_LOG, "utf8"), commands);
  }
  await scenario(0, 0, true, "gio\n");
  await scenario(7, 0, true, "gio\ngtk-launch\n");
  await scenario(7, 9, false, "gio\ngtk-launch\n");
  await scenario(null, 0, true, "gtk-launch\n");
  await scenario(null, null, false, "");
  writeFileSync(join(bin, "gio"), '#!/bin/sh\nprintf "gio\\n" >> "$ROVYL_LAUNCH_TEST_LOG"\nexec /bin/sleep 30\n', { mode: 0o755 });
  writeFileSync(join(bin, "gtk-launch"), '#!/bin/sh\nprintf "gtk-launch\\n" >> "$ROVYL_LAUNCH_TEST_LOG"\n', { mode: 0o755 });
  writeFileSync(process.env.ROVYL_LAUNCH_TEST_LOG, "");
  assert.equal(await launchDesktopId(id), true, "a stuck launcher times out and falls back");
  assert.equal(readFileSync(process.env.ROVYL_LAUNCH_TEST_LOG, "utf8"), "gio\ngtk-launch\n");
  rmSync(join(dir, "applications", `${id}.desktop`));
  await scenario(0, 0, true, "gtk-launch\n");
  console.log("linux-desktop-smoke: OK (launcher exits, fallback, timeout, missing tools, desktop ID)");
} finally {
  process.env = savedEnv;
  rmSync(dir, { recursive: true, force: true });
}
