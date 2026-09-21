# Rovyl — container packaging (published to ghcr.io/yahyazekry/rovyl-linux)
#
# The image bundles the unpacked Electron app; the desktop sockets come from mounts.
# The gesture helper needs direct input access, so /dev/input is passed through.
#
# X11:
#   docker run -it --rm \
#     -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix \
#     -v /dev/input:/dev/input \
#     --device /dev/dri \
#     ghcr.io/yahyazekry/rovyl-linux
#
# Wayland (KDE/sway/…):
#   docker run -it --rm \
#     -e XDG_RUNTIME_DIR=/wr -e WAYLAND_DISPLAY=$WAYLAND_DISPLAY \
#     -v $XDG_RUNTIME_DIR/$WAYLAND_DISPLAY:/wr/$WAYLAND_DISPLAY \
#     -v /dev/input:/dev/input \
#     --device /dev/dri \
#     ghcr.io/yahyazekry/rovyl-linux
#
# Persist settings by mounting a volume at /config (XDG_CONFIG_HOME points there).

FROM debian:12-slim

# Electron's runtime libraries plus the basics a desktop app expects (icons, MIME,
# a secret store for the licensing flow). No GUI toolkit of our own — Electron brings it.
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        libgtk-3-0 libnss3 libasound2 libgbm1 libxss1 libxtst6 \
        libxkbcommon0 libx11-6 libx11-xcb1 libxcb1 libxcomposite1 \
        libxdamage1 libxrandr2 libxcursor1 libxi6 \
        libatk-bridge2.0-0 libatk1.0-0 libcups2 libdrm2 \
        libpango-1.0-0 libcairo2 libnotify4 libsecret-1-0 \
        xdg-utils shared-mime-info \
    && rm -rf /var/lib/apt/lists/*

# Settings live outside the image so a `--rm` container keeps its configuration.
ENV XDG_CONFIG_HOME=/config \
    ELECTRON_DISABLE_SANDBOX=1 \
    ELECTRON_OZONE_PLATFORM_HINT=auto
RUN mkdir -p /config
VOLUME ["/config"]

WORKDIR /opt/rovyl
COPY linux-unpacked/ /opt/rovyl/
RUN chmod +x /opt/rovyl/rovyl \
             /opt/rovyl/resources/app.asar.unpacked/backend/rovyl-helper-linux \
             /opt/rovyl/resources/bin/rovyl-helper-linux 2>/dev/null || true

ENTRYPOINT ["/opt/rovyl/rovyl"]
