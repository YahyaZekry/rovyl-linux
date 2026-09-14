/**
 * Linux desktop-integration layer: the .desktop twin of Get-StartApps, extract-icon.ps1,
 * explorer.exe AppsFolder activation and the wt/cmd terminal wrapping.
 *
 * Everything here is pure Node so it can be smoke-tested off-Linux (paths and parsing only);
 * the process spawns are guarded by platform at the call sites in electron-main.js.
 */

const fs = require("fs");
const os = require("os");
const path = require("path");

const APPS_SCHEME = "desktop:";

/** XDG application directories, first match wins (priority order). */
function applicationDirs() {
  const home = os.homedir();
  const dataHome = process.env.XDG_DATA_HOME || path.join(home, ".local", "share");
  const dataDirs = (process.env.XDG_DATA_DIRS || "/usr/local/share:/usr/share").split(":");
  const dirs = [dataHome, ...dataDirs, "/var/lib/flatpak/exports", path.join(home, ".local/share/flatpak/exports")];
  return dirs.filter(Boolean).map((d) => path.join(d, "applications"));
}

/**
 * A desktop-file id encodes its subfolder with `-`: `org.foo.Bar.desktop` may live at
 * `org/foo/Bar.desktop`. Try the flat name first, then the nested split at every dash.
 */
function findDesktopFile(id) {
  const clean = String(id || "").replace(/\.desktop$/i, "");
  if (!clean) return null;
  const candidates = [clean];
  for (let i = 0; i < clean.length; i++) {
    if (clean[i] === "-") candidates.push(clean.slice(0, i) + "/" + clean.slice(i + 1));
  }
  for (const dir of applicationDirs()) {
    for (const c of candidates) {
      const p = path.join(dir, `${c}.desktop`);
      try {
        if (fs.statSync(p).isFile()) return p;
      } catch (_) {}
    }
  }
  return null;
}

function parseDesktopFile(file) {
  let text;
  try {
    text = fs.readFileSync(file, "utf8");
  } catch (_) {
    return null;
  }
  const entry = { hidden: false, name: "", icon: "", exec: "", terminal: false };
  let inSection = false;
  for (const raw of text.split(/\r?\n/)) {
    const line = raw.trim();
    if (line.startsWith("[")) {
      inSection = line === "[Desktop Entry]";
      continue;
    }
    if (!inSection || line.startsWith("#") || !line.includes("=")) continue;
    const eq = line.indexOf("=");
    const key = line.slice(0, eq).trim();
    const value = line.slice(eq + 1).trim();
    switch (key) {
      case "Name": if (!entry.name) entry.name = value; break;
      case "Icon": if (!entry.icon) entry.icon = value; break;
      case "Exec": if (!entry.exec) entry.exec = value; break;
      case "NoDisplay": if (value === "true") entry.hidden = true; break;
      case "Hidden": if (value === "true") entry.hidden = true; break;
      case "Terminal": if (value === "true") entry.terminal = true; break;
    }
  }
  return entry;
}

/**
 * The scan behind `get-installed-apps`. Same shape the Windows PowerShell scanner returns:
 * { Name, DisplayName, Path, IconPath } — with Path being `desktop:<id>`, which the launch
 * ladder and the icon pipeline both understand.
 */
function listDesktopApps() {
  const apps = [];
  const seen = new Set();
  const nameFilter = /Help|Feedback|Contact|Support|Manual/i;
  for (const dir of applicationDirs()) {
    let entries = [];
    try {
      entries = fs.readdirSync(dir, { withFileTypes: true });
    } catch (_) {
      continue;
    }
    for (const e of entries) {
      if (!e.isFile() || !e.name.toLowerCase().endsWith(".desktop")) continue;
      const id = e.name.replace(/\.desktop$/i, "");
      if (seen.has(id)) continue;
      const file = path.join(dir, e.name);
      const entry = parseDesktopFile(file);
      if (!entry || entry.hidden || !entry.name || nameFilter.test(entry.name)) continue;
      seen.add(id);
      apps.push({
        Name: entry.name,
        DisplayName: entry.name,
        Path: APPS_SCHEME + id,
        IconPath: APPS_SCHEME + id,
      });
    }
  }
  return apps;
}

/** Icon= can be an absolute path, a theme name, or a bare pixmaps name. */
function resolveIconPath(iconValue) {
  const value = String(iconValue || "").trim();
  if (!value) return null;
  if (path.isAbsolute(value)) {
    return fs.existsSync(value) ? value : null;
  }
  const exts = [".png", ".svg", ".xpm"];
  const home = os.homedir();
  const theme = process.env.XDG_CURRENT_THEME || "";
  const iconRoots = [
    theme && path.join(home, ".icons", theme),
    theme && path.join("/usr/share/icons", theme),
    path.join(home, ".icons", "hicolor"),
    path.join(home, ".local/share/icons", "hicolor"),
    "/usr/share/icons/hicolor",
  ].filter(Boolean);
  const sizes = ["512x512", "256x256", "128x128", "96x96", "64x64", "48x48", "scalable"];
  for (const rootDir of iconRoots) {
    for (const size of sizes) {
      for (const ext of exts) {
        const p = path.join(rootDir, size, "apps", value + ext);
        try {
          if (fs.statSync(p).isFile()) return p;
        } catch (_) {}
      }
    }
  }
  for (const ext of exts) {
    const p = path.join("/usr/share/pixmaps", value + ext);
    try {
      if (fs.statSync(p).isFile()) return p;
    } catch (_) {}
  }
  return null;
}

function readIconDataUrl(desktopId) {
  const file = findDesktopFile(desktopId);
  if (!file) return null;
  const entry = parseDesktopFile(file);
  if (!entry) return null;
  const iconPath = resolveIconPath(entry.icon);
  if (!iconPath) return null;
  try {
    const data = fs.readFileSync(iconPath);
    const ext = path.extname(iconPath).toLowerCase();
    const mime = ext === ".svg" ? "image/svg+xml" : ext === ".xpm" ? "image/x-xpixmap" : "image/png";
    return `data:${mime};base64,${data.toString("base64")}`;
  } catch (_) {
    return null;
  }
}

/**
 * Launch a desktop entry the way the shell would: `gio launch` on the file, falling back to
 * `gtk-launch` on the id. Detached — the child is the app, not ours.
 */
function launchDesktopId(desktopId) {
  const id = String(desktopId || "").replace(new RegExp(`^${APPS_SCHEME}`), "");
  const { spawn } = require("child_process");
  return new Promise((resolve) => {
    const file = findDesktopFile(id);
    const trySpawn = (cmd, args) => {
      const child = spawn(cmd, args, { detached: true, stdio: "ignore" });
      child.on("error", () => resolve(false));
      child.on("spawn", () => {
        child.unref();
        resolve(true);
      });
    };
    if (file) {
      trySpawn("gio", ["launch", file]);
      // gio launch failed synchronously? The error handler already resolved false.
    } else {
      trySpawn("gtk-launch", [id]);
    }
  });
}

/** First present terminal emulator, in the order a desktop user would expect. */
function detectTerminal() {
  const candidates = [
    "gnome-terminal",
    "konsole",
    "xfce4-terminal",
    "kitty",
    "alacritty",
    "xterm",
  ];
  const pathDirs = (process.env.PATH || "").split(":");
  for (const name of candidates) {
    for (const dir of pathDirs) {
      if (!dir) continue;
      try {
        fs.accessSync(path.join(dir, name), fs.constants.X_OK);
        return name;
      } catch (_) {}
    }
  }
  return null;
}

/** Open a terminal in `workingDir`, optionally running a command, Linux flavour. */
function spawnTerminal(terminal, workingDir, command) {
  const { spawn } = require("child_process");
  const wd = String(workingDir || process.cwd());
  const shLine = command
    ? `cd '${wd.replace(/'/g, "'\\''")}' && ${command}; exec bash`
    : null;
  let args;
  switch (terminal) {
    case "gnome-terminal":
      args = shLine ? ["--working-directory=" + wd, "--", "bash", "-c", shLine] : ["--working-directory=" + wd];
      break;
    case "konsole":
      args = shLine ? ["--workdir", wd, "-e", "bash", "-c", shLine] : ["--workdir", wd];
      break;
    case "xfce4-terminal":
      args = shLine ? ["--working-directory=" + wd, "-x", "bash", "-c", shLine] : ["--working-directory=" + wd];
      break;
    case "kitty":
      args = shLine ? ["--directory", wd, "bash", "-c", shLine] : ["--directory", wd];
      break;
    case "alacritty":
      args = shLine ? ["--working-directory", wd, "-e", "bash", "-c", shLine] : ["--working-directory", wd];
      break;
    case "xterm":
    default:
      args = shLine ? ["-e", "bash", "-c", shLine] : [];
      break;
  }
  try {
    const child = spawn(terminal, args, { detached: true, stdio: "ignore", cwd: wd });
    child.on("error", () => {});
    child.unref();
    return true;
  } catch (_) {
    return false;
  }
}

module.exports = {
  APPS_SCHEME,
  applicationDirs,
  findDesktopFile,
  parseDesktopFile,
  listDesktopApps,
  resolveIconPath,
  readIconDataUrl,
  launchDesktopId,
  detectTerminal,
  spawnTerminal,
};
