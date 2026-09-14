const { execFileSync } = require("child_process");
const fs = require("fs");
const path = require("path");

function buildWindowsHelper(projectRoot, binDir) {
  const src = path.join(projectRoot, "backend", "native-helper", "rovyl-helper.cs");
  const out1 = path.join(binDir, "rovyl-helper.exe");
  const out2 = path.join(projectRoot, "backend", "rovyl-helper.exe");

  const candidates = [
    "C:\\Windows\\Microsoft.NET\\Framework64\\v4.0.30319\\csc.exe",
    "C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\csc.exe",
  ];
  const csc = candidates.find((c) => fs.existsSync(c));
  if (!csc) {
    console.warn("[build-native-helper] csc.exe not found on system. Skipping native helper build.");
    return;
  }

  console.log("[build-native-helper] Compiling rovyl-helper.cs with", csc);
  try {
    execFileSync(
      csc,
      [
        "/target:winexe",
        "/platform:x64",
        "/optimize+",
        "/r:System.Windows.Forms.dll",
        "/r:System.Drawing.dll",
        `/out:${out1}`,
        src,
      ],
      { stdio: "inherit" },
    );
    try {
      fs.copyFileSync(out1, out2);
    } catch (copyErr) {
      console.warn("[build-native-helper] Note: Could not copy to backend/ (file may be running):", copyErr.message);
    }
    console.log("[build-native-helper] Compiled successfully:", out1);
  } catch (err) {
    console.error("[build-native-helper] Compilation failed:", err.message);
  }
}

function buildLinuxHelper(projectRoot, binDir) {
  const src = path.join(projectRoot, "backend", "native-helper-linux", "rovyl-helper.c");
  const out1 = path.join(binDir, "rovyl-helper-linux");
  const out2 = path.join(projectRoot, "backend", "rovyl-helper-linux");

  console.log("[build-native-helper] Compiling rovyl-helper.c with gcc");
  try {
    execFileSync("gcc", ["-O2", "-Wall", "-o", out1, src, "-lX11", "-lXtst"], { stdio: "inherit" });
    try {
      fs.copyFileSync(out1, out2);
    } catch (copyErr) {
      console.warn("[build-native-helper] Note: Could not copy to backend/ (file may be running):", copyErr.message);
    }
    console.log("[build-native-helper] Compiled successfully:", out1);
  } catch (err) {
    console.error("[build-native-helper] Compilation failed:", err.message);
    process.exitCode = 1;
  }
}

function buildNativeHelper() {
  const projectRoot = path.join(__dirname, "..");
  const binDir = path.join(projectRoot, "resources", "bin");
  if (!fs.existsSync(binDir)) {
    fs.mkdirSync(binDir, { recursive: true });
  }
  if (process.platform === "win32") {
    buildWindowsHelper(projectRoot, binDir);
  } else if (process.platform === "linux") {
    buildLinuxHelper(projectRoot, binDir);
  } else {
    console.warn(`[build-native-helper] No native helper for ${process.platform}. Skipping.`);
  }
}

buildNativeHelper();
