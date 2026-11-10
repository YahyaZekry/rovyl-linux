import { execFileSync } from "node:child_process";

const git = (...args) => execFileSync("git", args, { encoding: "utf8" }).trim();
const upstream = "upstream/main";
const base = git("merge-base", "HEAD", upstream);
const range = `HEAD..${upstream}`;
const count = git("rev-list", "--count", range);
const tip = git("rev-parse", upstream);
const log = (...paths) => git("log", "--max-count=80", "--format=%h %s", range, "--", ...paths);

console.log(`# Upstream changes for Linux review

Upstream has **${count} commits outside this fork's history**. Selectively ported commits can still appear here.
Upstream's current app is Windows-native; this Linux app stays on Electron. No merge, push, or code change is performed.

[Compare upstream changes](https://github.com/arshit09/rovyl/compare/${base}...${tip})

## Electron changes to inspect for compatible ports

\`\`\`text
${log("backend/", "archive/electron/", ":(glob)src/**/*.tsx", ":(glob)src/**/*.ts", ":(glob)src/**/*.css") || "None."}
\`\`\`

## All incoming commits (up to 80)

\`\`\`text
${log() || "None."}
\`\`\`

Review Linux input, calibration, click-away, tray behavior, config compatibility, and packaging before porting a change.
`);
