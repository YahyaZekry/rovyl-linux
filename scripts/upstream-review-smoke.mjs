import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { mkdtempSync, mkdirSync, writeFileSync, rmSync, readFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { fileURLToPath } from "node:url";

const dir = mkdtempSync(join(tmpdir(), "rovyl-upstream-review-"));
const git = (...args) => execFileSync("git", args, { cwd: dir, encoding: "utf8", stdio: ["ignore", "pipe", "pipe"] }).trim();
const report = () => execFileSync(process.execPath, [fileURLToPath(new URL("./upstream-review.mjs", import.meta.url))], { cwd: dir, encoding: "utf8" });
try {
  git("init");
  git("config", "user.name", "Test");
  git("config", "user.email", "test@example.invalid");
  git("commit", "--allow-empty", "-m", "base");
  git("update-ref", "refs/remotes/upstream/main", "HEAD");
  assert.match(report(), /\*\*0 commits/);
  mkdirSync(join(dir, "backend"));
  writeFileSync(join(dir, "backend", "example.cjs"), "// compatible fix\n");
  git("add", ".");
  git("commit", "-m", "fix: Electron launch handling");
  git("commit", "--allow-empty", "-m", "feat: native Windows rewrite");
  git("update-ref", "refs/remotes/upstream/main", "HEAD");
  git("checkout", "--detach", "HEAD~2");
  writeFileSync(join(dir, "local.txt"), "keep me\n");
  const before = [git("rev-parse", "HEAD"), git("status", "--porcelain")];
  const text = report();
  assert.match(text, /\*\*2 commits/);
  const legacy = text.split("## Electron changes")[1].split("## All incoming")[0];
  assert.match(legacy, /fix: Electron launch handling/);
  assert.doesNotMatch(legacy, /feat: native Windows rewrite/);
  assert.match(text, /feat: native Windows rewrite/);
  assert.deepEqual([git("rev-parse", "HEAD"), git("status", "--porcelain")], before);
  const workflow = readFileSync(new URL("../.github/workflows/sync-fork.yml", import.meta.url), "utf8");
  assert.match(workflow, /contents: read/);
  assert.doesNotMatch(workflow, /git (?:merge|push)|issues: write|contents: write/);
  console.log("upstream-review-smoke: OK (empty, categorized changes, untouched checkout, read-only workflow)");
} finally {
  rmSync(dir, { recursive: true, force: true });
}
