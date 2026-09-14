/*
 * rovyl-helper-linux — the Linux twin of backend/native-helper/rovyl-helper.cs.
 *
 * Speaks the same line protocol over stdin/stdout so electron-main.js can talk to either:
 *   commands in:  BLOCK x y w h mx my mw mh | UNBLOCK | TRIGGER vk mode slop [holdMs dragPx]
 *                 TRIGGER OFF | RECORD ON/OFF | SHORTCUT_TRIGGER vk mask | WARP x y | EXIT
 *   events out:   READY | TRIGGER_DOWN | TRIGGER_UP | TRIGGER_HOLD | TRIGGER_OFF |
 *                 SHORTCUT_DOWN/UP | RECORD_MOUSE <name> <mask> | TRIM|OK|0
 *   foreground-focus mode:  FG -> FG|l,t,w,h|exe|title   FOCUS <id> -> OK|MISS|HIDDEN|ALREADY|BADHWND
 *
 * Windows hook (WH_MOUSE_LL) maps to X11 passive grabs on the root window, installed with
 * GrabModeSync: the frozen press is delivered to us, and one XAllowEvents decides its fate —
 * AsyncPointer swallows it, ReplayPointer reprocesses it as if we never grabbed, which is the
 * "return 1" / "CallNextHookEx" pair of the C# hook. XTest re-synthesises the clicks that the
 * gesture hands back (the C# SendInput + dwExtraInfo signature trick becomes a flag: the only
 * synthetic events that can reach us are the ones we just injected).
 *
 * Everything needs only DISPLAY access — no evdev, no uinput, no special permissions.
 * Wayland (without XWayland focus) cannot support any of this; there the app falls back to
 * hotkey-only mode and this helper is never spawned.
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>

/* Mouse VKs the JS side sends (same numbers the Windows helper takes). */
#define VK_RIGHT 2
#define VK_MIDDLE 4
#define VK_X1 5
#define VK_X2 6

/* Button1..5 are left/right/middle/scroll-up/scroll-down in X11; X1/X2 are 8/9. */
#define XBTN_LEFT 1
#define XBTN_MIDDLE 2
#define XBTN_RIGHT 3
#define XBTN_SCROLL_UP 4
#define XBTN_SCROLL_DOWN 5
#define XBTN_X1 8
#define XBTN_X2 9

/* Modifier mask bits, matching the Windows helper's GetCurrentModifierMask. */
#define MOD_CTRL 1
#define MOD_ALT 2
#define MOD_SHIFT 4
#define MOD_SUPER 8

#define PASSTHROUGH_MAX_MS 250
#define DEFAULT_CLICK_HOLD_MS 400
#define DEFAULT_CLICK_DRAG_PX 30

static Display *dpy;
static Window root;
static int xfd;

static void emit(const char *line) {
  fputs(line, stdout);
  fputc('\n', stdout);
  fflush(stdout);
}

static long long now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* ---------------------------------------------------------------- modifier keys */

#define MAX_KEYCODE 255
static KeySym keycode_syms[MAX_KEYCODE + 1][2];

static void build_keycode_table(void) {
  int min_kc, max_kc, per_row;
  XDisplayKeycodes(dpy, &min_kc, &max_kc);
  if (min_kc < 0) min_kc = 8;
  if (max_kc > MAX_KEYCODE) max_kc = MAX_KEYCODE;
  for (int kc = min_kc; kc <= max_kc; kc++) {
    KeySym *syms = XGetKeyboardMapping(dpy, kc, 1, &per_row);
    if (syms) {
      keycode_syms[kc][0] = syms[0];
      keycode_syms[kc][1] = per_row > 1 ? syms[1] : NoSymbol;
      XFree(syms);
    }
  }
}

static int mod_bit_for(KeySym sym) {
  switch (sym) {
    case XK_Control_L: case XK_Control_R: return MOD_CTRL;
    case XK_Alt_L: case XK_Alt_R: return MOD_ALT;
    case XK_Shift_L: case XK_Shift_R: return MOD_SHIFT;
    case XK_Super_L: case XK_Super_R: case XK_Hyper_L: case XK_Hyper_R: return MOD_SUPER;
    default: return 0;
  }
}

static int query_modifier_mask(void) {
  unsigned char keys[32];
  XQueryKeymap(dpy, (char *)keys);
  int mask = 0;
  for (int kc = 0; kc <= MAX_KEYCODE; kc++) {
    if (!(keys[kc / 8] & (1 << (kc % 8)))) continue;
    int bit = mod_bit_for(keycode_syms[kc][0]) | mod_bit_for(keycode_syms[kc][1]);
    if (bit) mask |= bit;
  }
  return mask;
}

/* ---------------------------------------------------------------- grab management */

static int vk_to_button(int vk) {
  switch (vk) {
    case VK_RIGHT: return XBTN_RIGHT;
    case VK_MIDDLE: return XBTN_MIDDLE;
    case VK_X1: return XBTN_X1;
    case VK_X2: return XBTN_X2;
    default: return XBTN_MIDDLE;
  }
}

static int grabs_installed;

static void uninstall_grabs(void) {
  if (!grabs_installed) return;
  static const int all_buttons[] = {
    XBTN_LEFT, XBTN_MIDDLE, XBTN_RIGHT, XBTN_SCROLL_UP, XBTN_SCROLL_DOWN, XBTN_X1, XBTN_X2,
  };
  for (unsigned i = 0; i < sizeof(all_buttons) / sizeof(all_buttons[0]); i++) {
    XUngrabButton(dpy, all_buttons[i], AnyModifier, root);
  }
  grabs_installed = 0;
}

static void grab_button(int button) {
  XGrabButton(dpy, button, AnyModifier, root, False,
              ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeSync, None, None);
}

/* ---------------------------------------------------------------- gesture state */

/* Feature toggles driven by commands. */
static volatile int blocking;
static int block_l, block_t, block_r, block_b;
static int mon_l, mon_t, mon_r, mon_b;

static volatile int trigger_button_vk;   /* 0 = off */
static int trigger_hold_mode;
static int trigger_threshold;
static int click_hold_ms = DEFAULT_CLICK_HOLD_MS;
static int click_drag_px = DEFAULT_CLICK_DRAG_PX;

static volatile int record_mode;

static int shortcut_button;             /* 0 = off */
static int shortcut_mod_mask;
static int shortcut_active;

/* Per-press trigger state. */
static int trigger_held, click_press_armed, click_injected_button;
static int down_x, down_y;
static long long down_at;

/*
 * Synthetic events we are expecting: the passive grabs fire on our own XTest presses too
 * (the server cannot tell them from real ones). Each pending synthetic event is replayed to
 * the window underneath instead of being interpreted; any REAL press clears the counter, so
 * a lost synthetic event can never eat a later gesture.
 */
static int expect_synthetic;

static int point_in(int x, int y, int l, int t, int r, int b) {
  return x >= l && x < r && y >= t && y < b;
}

static void passthrough_click(int button) {
  /* Grab is inactive by the time we inject: the release just processed ended it. */
  expect_synthetic += 2;
  XTestFakeButtonEvent(dpy, button, True, CurrentTime);
  XTestFakeButtonEvent(dpy, button, False, CurrentTime);
  XFlush(dpy);
}

/* The click-mode mid-hold injection: too long or too drags, the app gets its button back. */
static void inject_button_down(int button) {
  expect_synthetic += 1;
  XTestFakeButtonEvent(dpy, button, True, CurrentTime);
  XFlush(dpy);
}

static void inject_button_up(int button) {
  expect_synthetic += 1;
  XTestFakeButtonEvent(dpy, button, False, CurrentTime);
  XFlush(dpy);
}

static const char *record_name_for(int button) {
  switch (button) {
    case XBTN_MIDDLE: return "Middle";
    case XBTN_X1: return "Mouse4";
    case XBTN_X2: return "Mouse5";
    default: return "RightClick";
  }
}

static void handle_button_press(int button, int x, int y) {
  if (expect_synthetic > 0) {
    expect_synthetic--;
    XAllowEvents(dpy, ReplayPointer, CurrentTime);
    return;
  }

  if (shortcut_button && button == shortcut_button) {
    int mods = query_modifier_mask();
    if (mods == shortcut_mod_mask && !shortcut_active) {
      shortcut_active = 1;
      emit("SHORTCUT_DOWN");
      XAllowEvents(dpy, AsyncPointer, CurrentTime);
    } else {
      XAllowEvents(dpy, ReplayPointer, CurrentTime);
    }
    return;
  }

  if (record_mode) {
    int is_recordable = button == XBTN_MIDDLE || button == XBTN_X1 ||
                        button == XBTN_X2 || button == XBTN_RIGHT;
    if (is_recordable) {
      int mods = query_modifier_mask();
      if (button == XBTN_RIGHT && mods == 0) {
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
        return;
      }
      char line[64];
      snprintf(line, sizeof(line), "RECORD_MOUSE %s %d", record_name_for(button), mods);
      emit(line);
      XAllowEvents(dpy, AsyncPointer, CurrentTime);
      return;
    }
  }

  if (trigger_button_vk && button == vk_to_button(trigger_button_vk) && !trigger_held) {
    trigger_held = 1;
    down_x = x;
    down_y = y;
    down_at = now_ms();
    click_press_armed = !trigger_hold_mode;
    click_injected_button = 0;
    emit("TRIGGER_DOWN");
    XAllowEvents(dpy, AsyncPointer, CurrentTime);
    return;
  }

  if (blocking) {
    int inside_allowed = point_in(x, y, block_l, block_t, block_r, block_b);
    int inside_monitor = point_in(x, y, mon_l, mon_t, mon_r, mon_b);
    if (inside_monitor && !inside_allowed) emit("BLOCK_CLICK");
    XAllowEvents(dpy, inside_monitor && !inside_allowed ? AsyncPointer : ReplayPointer, CurrentTime);
    return;
  }

  XAllowEvents(dpy, ReplayPointer, CurrentTime);
}

static void handle_button_release(int button, int x, int y) {
  if (expect_synthetic > 0 && !(trigger_held && button == vk_to_button(trigger_button_vk))) {
    expect_synthetic--;
    XAllowEvents(dpy, ReplayPointer, CurrentTime);
    return;
  }

  if (trigger_held && button == vk_to_button(trigger_button_vk)) {
    trigger_held = 0;
    int dx = x - down_x;
    int dy = y - down_y;
    long long held = now_ms() - down_at;
    int threshold = trigger_threshold;
    if (trigger_hold_mode) {
      emit("TRIGGER_UP");
      if (held <= PASSTHROUGH_MAX_MS && (dx * dx + dy * dy) <= threshold * threshold) {
        passthrough_click(button);
      }
    } else {
      int armed = click_press_armed;
      click_press_armed = 0;
      int injected = click_injected_button;
      click_injected_button = 0;
      if (injected != 0) {
        inject_button_up(injected);
        emit("TRIGGER_HOLD");
      } else if (!armed || held >= click_hold_ms ||
                 (dx * dx + dy * dy) >= click_drag_px * click_drag_px) {
        emit("TRIGGER_HOLD");
      } else {
        emit("TRIGGER_UP");
        passthrough_click(button);
      }
    }
    XAllowEvents(dpy, AsyncPointer, CurrentTime);
    return;
  }

  if (shortcut_button && button == shortcut_button && shortcut_active) {
    shortcut_active = 0;
    emit("SHORTCUT_UP");
    XAllowEvents(dpy, AsyncPointer, CurrentTime);
    return;
  }

  if (blocking) {
    int inside_allowed = point_in(x, y, block_l, block_t, block_r, block_b);
    int inside_monitor = point_in(x, y, mon_l, mon_t, mon_r, mon_b);
    XAllowEvents(dpy, inside_monitor && !inside_allowed ? AsyncPointer : ReplayPointer, CurrentTime);
    return;
  }

  XAllowEvents(dpy, ReplayPointer, CurrentTime);
}

static void reinstall_grabs(void) {
  uninstall_grabs();
  if (trigger_button_vk) grab_button(vk_to_button(trigger_button_vk));
  if (shortcut_button) grab_button(vk_to_button(shortcut_button));
  if (record_mode) {
    grab_button(XBTN_MIDDLE);
    grab_button(XBTN_RIGHT);
    grab_button(XBTN_X1);
    grab_button(XBTN_X2);
  }
  if (blocking) {
    grab_button(XBTN_LEFT);
    grab_button(XBTN_MIDDLE);
    grab_button(XBTN_RIGHT);
    grab_button(XBTN_SCROLL_UP);
    grab_button(XBTN_SCROLL_DOWN);
    grab_button(XBTN_X1);
    grab_button(XBTN_X2);
  }
  XSync(dpy, False);
  grabs_installed = 1;
}

/* Click mode hands the button over mid-hold, so the held press needs the 15 ms poll. */
static void poll_click_hold(void) {
  static long long next_check;
  int armed_button = trigger_button_vk;
  if (!armed_button || trigger_hold_mode || !click_press_armed || click_injected_button != 0) return;
  long long now = now_ms();
  if (now < next_check) return;
  next_check = now + 15;
  long long pressed = now - down_at;
  int overdue = pressed < 0 || pressed >= click_hold_ms;
  if (!overdue) {
    Window root_ret, child;
    int rx, ry, wx, wy;
    unsigned int mask;
    if (XQueryPointer(dpy, root, &root_ret, &child, &rx, &ry, &wx, &wy, &mask)) {
      long long ddx = rx - down_x;
      long long ddy = ry - down_y;
      long long drag = click_drag_px;
      overdue = (ddx * ddx + ddy * ddy) >= drag * drag;
    }
  }
  if (overdue) {
    click_injected_button = vk_to_button(armed_button);
    inject_button_down(click_injected_button);
  }
}

static void apply_command(char *line) {
  char *save = NULL;
  char *parts[10] = {0};
  int n = 0;
  for (char *p = strtok_r(line, " ", &save); p && n < 10; p = strtok_r(NULL, " ", &save)) {
    parts[n++] = p;
  }
  if (n == 0) return;

  if (strcmp(parts[0], "BLOCK") == 0 && n == 9) {
    int x = atoi(parts[1]), y = atoi(parts[2]), w = atoi(parts[3]), h = atoi(parts[4]);
    int mx = atoi(parts[5]), my = atoi(parts[6]), mw = atoi(parts[7]), mh = atoi(parts[8]);
    block_l = x; block_t = y; block_r = x + w; block_b = y + h;
    mon_l = mx; mon_t = my; mon_r = mx + mw; mon_b = my + mh;
    blocking = 1;
    reinstall_grabs();
  } else if (strcmp(parts[0], "UNBLOCK") == 0) {
    blocking = 0;
    reinstall_grabs();
  } else if (strcmp(parts[0], "TRIGGER") == 0) {
    if (click_injected_button != 0) {
      inject_button_up(click_injected_button);
      click_injected_button = 0;
    }
    click_press_armed = 0;
    if (n >= 2 && strcmp(parts[1], "OFF") == 0) {
      trigger_button_vk = 0;
      reinstall_grabs();
      emit("TRIGGER_OFF");
      return;
    }
    if (n >= 4 && n <= 6) {
      int vk = atoi(parts[1]);
      int threshold = atoi(parts[3]);
      if (vk != VK_MIDDLE && vk != VK_X1 && vk != VK_X2) vk = VK_MIDDLE;
      trigger_hold_mode = strcmp(parts[2], "click") != 0;
      trigger_threshold = threshold > 0 ? threshold : 0;
      click_hold_ms = n >= 5 && atoi(parts[4]) > 0 ? atoi(parts[4]) : DEFAULT_CLICK_HOLD_MS;
      click_drag_px = n >= 6 && atoi(parts[5]) > 0 ? atoi(parts[5]) : DEFAULT_CLICK_DRAG_PX;
      trigger_button_vk = vk;
      reinstall_grabs();
      emit("TRIGGER_READY");
    }
  } else if (strcmp(parts[0], "RECORD") == 0) {
    if (n >= 2 && strcmp(parts[1], "ON") == 0) {
      record_mode = 1;
      emit("RECORD_READY");
    } else {
      record_mode = 0;
      emit("RECORD_OFF");
    }
    reinstall_grabs();
  } else if (strcmp(parts[0], "SHORTCUT_TRIGGER") == 0) {
    if (n >= 2 && strcmp(parts[1], "OFF") == 0) {
      shortcut_button = 0;
      shortcut_mod_mask = 0;
      shortcut_active = 0;
      reinstall_grabs();
      emit("SHORTCUT_TRIGGER_OFF");
      return;
    }
    if (n >= 3) {
      shortcut_button = atoi(parts[1]);
      shortcut_mod_mask = atoi(parts[2]);
      shortcut_active = 0;
      reinstall_grabs();
      emit("SHORTCUT_TRIGGER_READY");
    }
  } else if (n == 3 && strcmp(parts[0], "WARP") == 0) {
    XWarpPointer(dpy, None, root, 0, 0, 0, 0, atoi(parts[1]), atoi(parts[2]));
    XFlush(dpy);
  } else if (strcmp(parts[0], "EXIT") == 0) {
    exit(0);
  }
}

static void run_mouse_blocker(void) {
  build_keycode_table();
  reinstall_grabs();
  emit("READY");

  char buf[4096];
  size_t len = 0;
  for (;;) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    FD_SET(xfd, &fds);
    int maxfd = xfd > 0 ? xfd : 0;
    /* 15 ms tick while a click-mode press is held, otherwise sleep on the sockets. */
    struct timeval tv = {0, 15000}, *tvp = trigger_held && !trigger_hold_mode ? &tv : NULL;
    int ready = select(maxfd + 1, &fds, NULL, NULL, tvp);
    if (ready < 0 && errno != EINTR) break;
    if (ready > 0 && FD_ISSET(0, &fds)) {
      ssize_t got = read(0, buf + len, sizeof(buf) - len - 1);
      if (got <= 0) break; /* parent died or closed the pipe */
      len += (size_t)got;
      buf[len] = '\0';
      char *start = buf;
      char *nl;
      while ((nl = memchr(start, '\n', len - (size_t)(start - buf))) != NULL) {
        *nl = '\0';
        char *s = start;
        while (*s == ' ' || *s == '\t' || *s == '\r') s++;
        if (*s) apply_command(s);
        start = nl + 1;
      }
      size_t rem = len - (size_t)(start - buf);
      memmove(buf, start, rem);
      len = rem;
      buf[len] = '\0';
    }
    if (ready > 0 && FD_ISSET(xfd, &fds)) {
      while (XPending(dpy)) {
        XEvent ev;
        XNextEvent(dpy, &ev);
        if (ev.type == ButtonPress) {
          handle_button_press(ev.xbutton.button, ev.xbutton.x_root, ev.xbutton.y_root);
        } else if (ev.type == ButtonRelease) {
          handle_button_release(ev.xbutton.button, ev.xbutton.x_root, ev.xbutton.y_root);
        } else if (ev.type == MotionNotify) {
          /* Frozen by the active grab; unfreeze and swallow — aiming reads the real cursor. */
          XAllowEvents(dpy, AsyncPointer, CurrentTime);
        }
      }
      XFlush(dpy);
    }
    poll_click_hold();
  }
  uninstall_grabs();
  XCloseDisplay(dpy);
}

/* ---------------------------------------------------------------- foreground-focus mode */

static Window active_window(void) {
  Atom type;
  int fmt;
  unsigned long n, left;
  unsigned char *data = NULL;
  Window win = None;
  if (XGetWindowProperty(dpy, root, XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False), 0, 1, False,
                         XA_WINDOW, &type, &fmt, &n, &left, &data) == Success &&
      type == XA_WINDOW && n == 1 && data) {
    win = *(Window *)data;
  }
  if (data) XFree(data);
  return win;
}

static void sanitize(char *s) {
  for (; *s; s++) {
    if (*s == '\r' || *s == '\n' || *s == '|') *s = ' ';
  }
}

static void snapshot_fg(void) {
  Window win = active_window();
  if (win == None) {
    emit("FG||");
    return;
  }

  XWindowAttributes attrs;
  char bounds[64] = "";
  if (XGetWindowAttributes(dpy, win, &attrs)) {
    int rx = attrs.x, ry = attrs.y;
    Window child;
    XTranslateCoordinates(dpy, win, root, 0, 0, &rx, &ry, &child);
    snprintf(bounds, sizeof(bounds), "%d,%d,%d,%d", rx, ry, attrs.width, attrs.height);
  }

  char exe[PATH_MAX] = "";
  Atom type;
  int fmt;
  unsigned long n, left;
  unsigned char *data = NULL;
  if (XGetWindowProperty(dpy, win, XInternAtom(dpy, "_NET_WM_PID", False), 0, 1, False, XA_CARDINAL,
                         &type, &fmt, &n, &left, &data) == Success &&
      type == XA_CARDINAL && n == 1 && data) {
    pid_t pid = *(pid_t *)data;
    char link[64];
    snprintf(link, sizeof(link), "/proc/%d/exe", pid);
    ssize_t r = readlink(link, exe, sizeof(exe) - 1);
    if (r > 0) exe[r] = '\0';
  }
  if (data) XFree(data);

  char title[1024] = "";
  XTextProperty tp;
  if (XGetTextProperty(dpy, win, &tp, XInternAtom(dpy, "_NET_WM_NAME", False)) && tp.value) {
    snprintf(title, sizeof(title), "%s", tp.value);
  } else {
    char *name = NULL;
    if (XFetchName(dpy, win, &name) && name) {
      snprintf(title, sizeof(title), "%s", name);
      XFree(name);
    }
  }
  sanitize(title);

  printf("FG|%s|%s|%s\n", bounds, exe, title);
  fflush(stdout);
}

static void focus_window(Window target) {
  XWindowAttributes attrs;
  if (!XGetWindowAttributes(dpy, target, &attrs)) {
    emit("BADHWND");
    return;
  }
  if (attrs.map_state != IsViewable) {
    emit("HIDDEN");
    return;
  }
  if (active_window() == target) {
    emit("ALREADY");
    return;
  }

  XEvent ev = {0};
  ev.xclient.type = ClientMessage;
  ev.xclient.window = target;
  ev.xclient.message_type = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = 2; /* source: pager-style activation */
  ev.xclient.data.l[1] = 0; /* timestamp: CurrentTime */
  ev.xclient.data.l[2] = 0;
  XSendEvent(dpy, root, False,
             SubstructureRedirectMask | SubstructureNotifyMask, &ev);
  XFlush(dpy);

  for (int i = 0; i < 10; i++) {
    struct timespec ts = {0, 10 * 1000 * 1000};
    nanosleep(&ts, NULL);
    if (active_window() == target) {
      emit("OK");
      return;
    }
  }
  emit("MISS");
}

static void run_foreground_focus(void) {
  emit("READY");
  char buf[4096];
  size_t len = 0;
  for (;;) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    if (select(1, &fds, NULL, NULL, NULL) < 0) {
      if (errno == EINTR) continue;
      break;
    }
    ssize_t got = read(0, buf + len, sizeof(buf) - len - 1);
    if (got <= 0) break;
    len += (size_t)got;
    buf[len] = '\0';
    char *start = buf;
    char *nl;
    while ((nl = memchr(start, '\n', len - (size_t)(start - buf))) != NULL) {
      *nl = '\0';
      char *s = start;
      while (*s == ' ' || *s == '\t' || *s == '\r') s++;
      if (strncmp(s, "FG", 2) == 0) {
        snapshot_fg();
      } else if (strncmp(s, "FOCUS ", 6) == 0) {
        char *end = NULL;
        unsigned long long id = strtoull(s + 6, &end, 10);
        if (id == 0) {
          emit("BADHWND");
        } else {
          focus_window((Window)id);
        }
      } else if (strncmp(s, "TRIM", 4) == 0) {
        emit("TRIM|OK|0");
      } else if (strcmp(s, "EXIT") == 0) {
        XCloseDisplay(dpy);
        exit(0);
      }
      start = nl + 1;
    }
    size_t rem = len - (size_t)(start - buf);
    memmove(buf, start, rem);
    len = rem;
    buf[len] = '\0';
  }
  XCloseDisplay(dpy);
}

/* ---------------------------------------------------------------- evdev/uinput mode (native Wayland)
 *
 * The compositor owns the pointer on Wayland, so there is nothing to grab protocol-wise. This mode
 * works one level lower, the way input-remapper/keyd/ydotool do: EVIOCGRAB every mouse-class
 * /dev/input node (the events stop reaching the compositor) and re-emit them through a paired
 * uinput device (the compositor reads that instead). "Forward" = write the event to the paired
 * device; "swallow" = don't. The gesture/blocking state machine is the same one the Windows hook
 * and the X11 mode implement; `POS x y` feeds the helper the cursor position from the main process
 * (evdev events carry only deltas, and BLOCK judges clicks by position). Injected events come out
 * of our own virtual devices, which are never grabbed and always skip the grab scan, so no
 * signature scheme is needed — there is no feedback path back into this process.
 */

#define MAX_DEVICES 64
#define NAME_PREFIX "rovyl-fwd-"

struct ev_source {
  int ev_fd;   /* grabbed source, -1 when unused */
  int ui_fd;   /* paired uinput device */
  char name[96];
  char devnode[32];
};

static struct ev_source sources[MAX_DEVICES];
static int source_count;
static int kbd_fds[MAX_DEVICES];
static int kbd_count;

/* Capability probe: a mouse has EV_REL axes and at least BTN_LEFT. */
static int is_mouse_device(int fd) {
  unsigned char rel[(REL_MAX / 8) + 1] = {0};
  unsigned char key[(KEY_MAX / 8) + 1] = {0};
  if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel)), rel) < 0) return 0;
  if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key)), key) < 0) return 0;
  if (!(rel[REL_X / 8] & (1 << (REL_X % 8)))) return 0;
  return (key[BTN_LEFT / 8] & (1 << (BTN_LEFT % 8))) != 0;
}

static int is_keyboard_device(int fd) {
  unsigned char key[(KEY_MAX / 8) + 1] = {0};
  if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key)), key) < 0) return 0;
  return (key[KEY_A / 8] & (1 << (KEY_A % 8))) != 0;
}

static int mod_key_bit(unsigned int code) {
  switch (code) {
    case KEY_LEFTCTRL: case KEY_RIGHTCTRL: return MOD_CTRL;
    case KEY_LEFTALT: case KEY_RIGHTALT: return MOD_ALT;
    case KEY_LEFTSHIFT: case KEY_RIGHTSHIFT: return MOD_SHIFT;
    case KEY_LEFTMETA: case KEY_RIGHTMETA: return MOD_SUPER;
    default: return 0;
  }
}

static int btn_code_for_vk(int vk) {
  switch (vk) {
    case VK_RIGHT: return BTN_RIGHT;
    case VK_MIDDLE: return BTN_MIDDLE;
    case VK_X1: return BTN_SIDE;
    case VK_X2: return BTN_EXTRA;
    default: return BTN_MIDDLE;
  }
}

static int vk_for_btn_code(unsigned int code) {
  switch (code) {
    case BTN_RIGHT: return VK_RIGHT;
    case BTN_MIDDLE: return VK_MIDDLE;
    case BTN_SIDE: return VK_X1;
    case BTN_EXTRA: return VK_X2;
    default: return 0;
  }
}

static const char *record_name_for_vk(int vk) {
  switch (vk) {
    case VK_MIDDLE: return "Middle";
    case VK_X1: return "Mouse4";
    case VK_X2: return "Mouse5";
    default: return "RightClick";
  }
}

static long long evdev_now_ms(void) { return now_ms(); }

static void write_event(int fd, unsigned short type, unsigned short code, int value) {
  struct input_event ev;
  memset(&ev, 0, sizeof(ev));
  gettimeofday(&ev.time, NULL);
  ev.type = type;
  ev.code = code;
  ev.value = value;
  if (write(fd, &ev, sizeof(ev)) < 0) { /* device gone; hotplug cleanup will handle it */ }
}

static void emit_uinput(struct ev_source *src, unsigned short type, unsigned short code, int value) {
  if (!src || src->ui_fd < 0) return;
  write_event(src->ui_fd, type, code, value);
}

static void emit_syn(struct ev_source *src) {
  emit_uinput(src, EV_SYN, SYN_REPORT, 0);
}

static void inject_button(struct ev_source *src, unsigned int code, int value) {
  emit_uinput(src, EV_KEY, code, value);
  emit_syn(src);
}

/* Copy the full capability set and identity so libinput applies the same hwdb/DPI rules —
 * skipping this makes forwarded pointers "feel wrong" (input-remapper PR #1290). */
static int create_forwarding_device(int src_fd, const char *src_name) {
  unsigned char ev_bits[(EV_MAX / 8) + 1] = {0};
  unsigned char key_bits[(KEY_MAX / 8) + 1] = {0};
  unsigned char rel_bits[(REL_MAX / 8) + 1] = {0};
  unsigned char msc_bits[(MSC_MAX / 8) + 1] = {0};
  unsigned char prop_bits[(INPUT_PROP_MAX / 8) + 1] = {0};
  struct uinput_user_dev ui;
  int fd;

  if (ioctl(src_fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) return -1;
  ioctl(src_fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
  ioctl(src_fd, EVIOCGBIT(EV_REL, sizeof(rel_bits)), rel_bits);
  ioctl(src_fd, EVIOCGBIT(EV_MSC, sizeof(msc_bits)), msc_bits);
  ioctl(src_fd, EVIOCGPROP(sizeof(prop_bits)), prop_bits);

  fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (fd < 0) {
    fprintf(stderr, "rovyl-helper-linux: cannot open /dev/uinput (%s); "
            "add the user to the `input` group or install the packaged udev rule\n", strerror(errno));
    return -1;
  }

  ioctl(fd, UI_SET_EVBIT, EV_SYN);
  for (int bit = 0; bit <= EV_MAX; bit++) {
    if (!(ev_bits[bit / 8] & (1 << (bit % 8))) || bit == EV_SYN) continue;
    ioctl(fd, UI_SET_EVBIT, bit);
  }
  for (int bit = 0; bit <= KEY_MAX; bit++) {
    if (key_bits[bit / 8] & (1 << (bit % 8))) ioctl(fd, UI_SET_KEYBIT, bit);
  }
  for (int bit = 0; bit <= REL_MAX; bit++) {
    if (rel_bits[bit / 8] & (1 << (bit % 8))) ioctl(fd, UI_SET_RELBIT, bit);
  }
  for (int bit = 0; bit <= MSC_MAX; bit++) {
    if (msc_bits[bit / 8] & (1 << (bit % 8))) ioctl(fd, UI_SET_MSCBIT, bit);
  }
  for (int bit = 0; bit <= INPUT_PROP_MAX; bit++) {
    if (prop_bits[bit / 8] & (1 << (bit % 8))) ioctl(fd, UI_SET_PROPBIT, bit);
  }

  memset(&ui, 0, sizeof(ui));
  snprintf(ui.name, UINPUT_MAX_NAME_SIZE, NAME_PREFIX "%s", src_name);
  ioctl(src_fd, EVIOCGID, &ui.id);
  if (write(fd, &ui, sizeof(ui)) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
    fprintf(stderr, "rovyl-helper-linux: UI_DEV_CREATE failed: %s\n", strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}

static void drop_source_at(int i) {
  /* close, then compact — nothing may be left with a negative fd for select()/FD_SET */
  if (sources[i].ev_fd >= 0) {
    ioctl(sources[i].ev_fd, EVIOCGRAB, 0);
    close(sources[i].ev_fd);
  }
  if (sources[i].ui_fd >= 0) {
    ioctl(sources[i].ui_fd, UI_DEV_DESTROY);
    close(sources[i].ui_fd);
  }
  for (int j = i; j < source_count - 1; j++) sources[j] = sources[j + 1];
  source_count--;
  sources[source_count] = (struct ev_source){-1, -1, "", ""};
}

/* `name_filter` scopes grabbing for tests and CI: only mice whose name contains it. */
static void scan_input_devices(const char *name_filter) {
  DIR *dir = opendir("/dev/input");
  if (!dir) return;
  struct dirent *de;
  while ((de = readdir(dir)) != NULL) {
    if (strncmp(de->d_name, "event", 5) != 0) continue;
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/dev/input/%s", de->d_name);
    int fd = open(path, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
      fd = open(path, O_RDONLY | O_NONBLOCK);
      if (fd < 0) continue;
    }
    char name[96] = "";
    ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name);
    if (strncmp(name, "rovyl-", 6) == 0) { close(fd); continue; }

    if (is_mouse_device(fd)) {
      if (source_count >= MAX_DEVICES) { close(fd); continue; }
      if (name_filter && (!name[0] || !strstr(name, name_filter))) { close(fd); continue; }
      /* already ours from an earlier scan: the node is still held, EVIOCGRAB would just EBUSY */
      int held = 0;
      for (int k = 0; k < source_count; k++) {
        if (strcmp(sources[k].devnode, de->d_name) == 0) { held = 1; break; }
      }
      if (held) { close(fd); continue; }
      if (ioctl(fd, EVIOCGRAB, 1) < 0) {
        fprintf(stderr, "rovyl-helper-linux: EVIOCGRAB %s failed: %s (another grabber owns it?)\n",
                name, strerror(errno));
        close(fd);
        continue;
      }
      int ui = create_forwarding_device(fd, name);
      if (ui < 0) {
        ioctl(fd, EVIOCGRAB, 0);
        close(fd);
        continue;
      }
      sources[source_count].ev_fd = fd;
      sources[source_count].ui_fd = ui;
      snprintf(sources[source_count].name, sizeof(sources[source_count].name), "%s", name);
      snprintf(sources[source_count].devnode, sizeof(sources[source_count].devnode), "%s", de->d_name);
      source_count++;
    } else if (is_keyboard_device(fd)) {
      if (kbd_count >= MAX_DEVICES) { close(fd); continue; }
      /* observers, not grabbers: keyboards keep working normally, we just watch modifiers */
      kbd_fds[kbd_count++] = fd;
    } else {
      close(fd);
    }
  }
  closedir(dir);
}

static void rescan_input_devices(const char *name_filter) {
  /* poll tells us which nodes are really gone: POLLERR/POLLHUP on removal, and it never
   * false-positives the way a zero-length read probe would */
  for (int i = source_count - 1; i >= 0; i--) {
    struct pollfd p = {sources[i].ev_fd, POLLERR | POLLHUP, 0};
    if (poll(&p, 1, 0) > 0 && (p.revents & (POLLERR | POLLHUP))) drop_source_at(i);
  }
  for (int i = kbd_count - 1; i >= 0; i--) {
    struct pollfd p = {kbd_fds[i], POLLERR | POLLHUP, 0};
    if (poll(&p, 1, 0) > 0 && (p.revents & (POLLERR | POLLHUP))) {
      close(kbd_fds[i]);
      kbd_fds[i] = kbd_fds[--kbd_count];
    }
  }
  scan_input_devices(name_filter);
}

/* ---------------------------------------------------------------- evdev gesture state */

static int ev_trigger_vk;                 /* 0 = off */
static int ev_trigger_hold_mode;
static int ev_trigger_threshold;
static int ev_click_hold_ms = DEFAULT_CLICK_HOLD_MS;
static int ev_click_drag_px = DEFAULT_CLICK_DRAG_PX;

static int ev_trigger_held;
static struct ev_source *ev_trigger_src;
static int ev_click_press_armed, ev_click_injected;
static long long ev_down_at;
static long long ev_gesture_dx, ev_gesture_dy;

static volatile int ev_blocking;
static int ev_block_l, ev_block_t, ev_block_r, ev_block_b;
static int ev_mon_l, ev_mon_t, ev_mon_r, ev_mon_b;
static int ev_last_x = -1, ev_last_y = -1;

static volatile int ev_record_mode;
static int ev_shortcut_vk, ev_shortcut_mod_mask, ev_shortcut_active;
static int ev_mod_mask;

static int ev_point_in(int x, int y, int l, int t, int r, int b) {
  return x >= l && x < r && y >= t && y < b;
}

static void ev_handle_key(struct ev_source *src, unsigned int code, int value) {
  int vk = vk_for_btn_code(code);
  int press = value != 0; /* 2 = repeat: treat as held */

  if (ev_shortcut_vk && vk == ev_shortcut_vk) {
    if (press) {
      if (ev_mod_mask == ev_shortcut_mod_mask && !ev_shortcut_active) {
        ev_shortcut_active = 1;
        emit("SHORTCUT_DOWN");
        return; /* swallow */
      }
    } else if (ev_shortcut_active) {
      ev_shortcut_active = 0;
      emit("SHORTCUT_UP");
      return; /* swallow */
    }
    inject_button(src, code, value);
    return;
  }

  if (ev_record_mode && (vk == VK_MIDDLE || vk == VK_X1 || vk == VK_X2 || vk == VK_RIGHT)) {
    if (vk == VK_RIGHT && ev_mod_mask == 0) {
      /* not a recording candidate without modifiers: the whole click passes through */
      inject_button(src, code, value);
      return;
    }
    if (press) {
      char line[64];
      snprintf(line, sizeof(line), "RECORD_MOUSE %s %d", record_name_for_vk(vk), ev_mod_mask);
      emit(line);
    }
    return; /* recording candidate: swallow press and release */
  }

  if (ev_trigger_vk && vk == ev_trigger_vk) {
    if (press && !ev_trigger_held) {
      ev_trigger_held = 1;
      ev_trigger_src = src;
      ev_down_at = evdev_now_ms();
      ev_gesture_dx = ev_gesture_dy = 0;
      ev_click_press_armed = !ev_trigger_hold_mode;
      ev_click_injected = 0;
      emit("TRIGGER_DOWN");
      return; /* swallow the physical press */
    }
    if (!press && ev_trigger_held && src == ev_trigger_src) {
      ev_trigger_held = 0;
      ev_trigger_src = NULL;
      long long held = evdev_now_ms() - ev_down_at;
      long long dist2 = ev_gesture_dx * ev_gesture_dx + ev_gesture_dy * ev_gesture_dy;
      if (ev_trigger_hold_mode) {
        emit("TRIGGER_UP");
        if (held <= PASSTHROUGH_MAX_MS &&
            (long long)ev_trigger_threshold * ev_trigger_threshold >= dist2) {
          inject_button(src, code, 1);
          inject_button(src, code, 0);
        }
        return;
      }
      if (ev_click_injected) {
        inject_button(src, code, 0);
        emit("TRIGGER_HOLD");
      } else if (!ev_click_press_armed || held >= ev_click_hold_ms ||
                 dist2 >= (long long)ev_click_drag_px * ev_click_drag_px) {
        emit("TRIGGER_HOLD");
      } else {
        emit("TRIGGER_UP");
        inject_button(src, code, 1);
        inject_button(src, code, 0);
      }
      return;
    }
    /* release for a gesture we didn't start, or a repeat: forward */
    inject_button(src, code, value);
    return;
  }

  if (ev_blocking) {
    int inside_allowed = ev_point_in(ev_last_x, ev_last_y, ev_block_l, ev_block_t, ev_block_r, ev_block_b);
    int inside_monitor = ev_point_in(ev_last_x, ev_last_y, ev_mon_l, ev_mon_t, ev_mon_r, ev_mon_b);
    if (inside_monitor && !inside_allowed) {
      /* swallow — and tell main, so a click away from the wheel/panel can close it */
      if (press) emit("BLOCK_CLICK");
      return;
    }
  }

  inject_button(src, code, value);
}

static void ev_handle_rel(struct ev_source *src, unsigned int code, int value) {
  if (ev_trigger_held) {
    if (code == REL_X) ev_gesture_dx += value;
    else if (code == REL_Y) ev_gesture_dy += value;
  }
  if (ev_blocking && (code == REL_WHEEL || code == REL_HWHEEL || code == REL_WHEEL_HI_RES ||
                      code == REL_HWHEEL_HI_RES)) {
    int inside_allowed = ev_point_in(ev_last_x, ev_last_y, ev_block_l, ev_block_t, ev_block_r, ev_block_b);
    int inside_monitor = ev_point_in(ev_last_x, ev_last_y, ev_mon_l, ev_mon_t, ev_mon_r, ev_mon_b);
    if (inside_monitor && !inside_allowed) return; /* swallow scroll outside the wheel */
  }
  /* EV_REL — inject_button is EV_KEY-only; relaying motion through it writes garbage keys
   * instead of movement and freezes the compositor's cursor */
  emit_uinput(src, EV_REL, code, value);
  emit_syn(src);
}

/* Click mode hands the button over mid-hold: too long or too drags, the app gets its press. */
static void ev_poll_click_hold(void) {
  static long long next_check;
  if (!ev_trigger_vk || ev_trigger_hold_mode || !ev_click_press_armed || ev_click_injected) return;
  long long now = evdev_now_ms();
  if (now < next_check) return;
  next_check = now + 15;
  long long pressed = now - ev_down_at;
  long long dist2 = ev_gesture_dx * ev_gesture_dx + ev_gesture_dy * ev_gesture_dy;
  if (pressed < 0 || pressed >= ev_click_hold_ms ||
      dist2 >= (long long)ev_click_drag_px * ev_click_drag_px) {
    ev_click_injected = 1;
    inject_button(ev_trigger_src, btn_code_for_vk(ev_trigger_vk), 1);
  }
}

static void ev_apply_command(char *line) {
  char *save = NULL;
  char *parts[10] = {0};
  int n = 0;
  for (char *p = strtok_r(line, " ", &save); p && n < 10; p = strtok_r(NULL, " ", &save)) {
    parts[n++] = p;
  }
  if (n == 0) return;

  if (strcmp(parts[0], "BLOCK") == 0 && n == 9) {
    int x = atoi(parts[1]), y = atoi(parts[2]), w = atoi(parts[3]), h = atoi(parts[4]);
    int mx = atoi(parts[5]), my = atoi(parts[6]), mw = atoi(parts[7]), mh = atoi(parts[8]);
    ev_block_l = x; ev_block_t = y; ev_block_r = x + w; ev_block_b = y + h;
    ev_mon_l = mx; ev_mon_t = my; ev_mon_r = mx + mw; ev_mon_b = my + mh;
    ev_blocking = 1;
  } else if (strcmp(parts[0], "UNBLOCK") == 0) {
    ev_blocking = 0;
  } else if (strcmp(parts[0], "POS") == 0 && n == 3) {
    ev_last_x = atoi(parts[1]);
    ev_last_y = atoi(parts[2]);

  } else if (strcmp(parts[0], "TRIGGER") == 0) {
    if (ev_click_injected && ev_trigger_src) {
      inject_button(ev_trigger_src, btn_code_for_vk(ev_trigger_vk), 0);
    }
    ev_click_injected = 0;
    ev_click_press_armed = 0;
    ev_trigger_held = 0;
    ev_trigger_src = NULL;
    if (n >= 2 && strcmp(parts[1], "OFF") == 0) {
      ev_trigger_vk = 0;
      emit("TRIGGER_OFF");
      return;
    }
    if (n >= 4 && n <= 6) {
      int vk = atoi(parts[1]);
      int threshold = atoi(parts[3]);
      if (vk != VK_MIDDLE && vk != VK_X1 && vk != VK_X2) vk = VK_MIDDLE;
      ev_trigger_hold_mode = strcmp(parts[2], "click") != 0;
      ev_trigger_threshold = threshold > 0 ? threshold : 0;
      ev_click_hold_ms = n >= 5 && atoi(parts[4]) > 0 ? atoi(parts[4]) : DEFAULT_CLICK_HOLD_MS;
      ev_click_drag_px = n >= 6 && atoi(parts[5]) > 0 ? atoi(parts[5]) : DEFAULT_CLICK_DRAG_PX;
      ev_trigger_vk = vk;
      emit(source_count > 0 ? "TRIGGER_READY" : "TRIGGER_FAILED");
    }
  } else if (strcmp(parts[0], "RECORD") == 0) {
    ev_record_mode = (n >= 2 && strcmp(parts[1], "ON") == 0);
    emit(ev_record_mode ? "RECORD_READY" : "RECORD_OFF");
  } else if (strcmp(parts[0], "SHORTCUT_TRIGGER") == 0) {
    if (n >= 2 && strcmp(parts[1], "OFF") == 0) {
      ev_shortcut_vk = 0;
      ev_shortcut_mod_mask = 0;
      ev_shortcut_active = 0;
      emit("SHORTCUT_TRIGGER_OFF");
      return;
    }
    if (n >= 3) {
      ev_shortcut_vk = atoi(parts[1]);
      ev_shortcut_mod_mask = atoi(parts[2]);
      ev_shortcut_active = 0;
      emit("SHORTCUT_TRIGGER_READY");
    }
  } else if (strcmp(parts[0], "WARP") == 0) {
    fprintf(stderr, "rovyl-helper-linux: WARP unsupported on native Wayland (no warp protocol)\n");
  } else if (strcmp(parts[0], "EXIT") == 0) {
    exit(0);
  }
}

static int ev_stdin_line(char *buf, size_t cap) {
  size_t len = 0;
  for (;;) {
    char c;
    ssize_t got = read(0, &c, 1);
    if (got <= 0) return -1;
    if (c == '\n') break;
    if (len + 1 < cap) buf[len++] = c;
  }
  buf[len] = '\0';
  char *s = buf;
  while (*s == ' ' || *s == '\t' || *s == '\r') s++;
  if (*s) ev_apply_command(s);
  return 0;
}

static void run_mouse_blocker_evdev(const char *name_filter) {
  for (int i = 0; i < MAX_DEVICES; i++) sources[i] = (struct ev_source){-1, -1, "", ""};
  scan_input_devices(name_filter);
  emit("READY");
  if (source_count == 0) {
    if (name_filter) {
      fprintf(stderr, "rovyl-helper-linux: no grabbable mouse devices found matching '%s'; "
              "gesture capture will not fire (input group membership required)\n", name_filter);
    } else {
      fprintf(stderr, "rovyl-helper-linux: no grabbable mouse devices found; "
              "gesture capture will not fire (input group membership required)\n");
    }
  }

  char buf[256];
  for (;;) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    int maxfd = 0;
    for (int i = 0; i < source_count; i++) {
      FD_SET(sources[i].ev_fd, &fds);
      if (sources[i].ev_fd > maxfd) maxfd = sources[i].ev_fd;
    }
    for (int i = 0; i < kbd_count; i++) {
      FD_SET(kbd_fds[i], &fds);
      if (kbd_fds[i] > maxfd) maxfd = kbd_fds[i];
    }
    /*
     * Hotplug = a rescan every second. inotify on /dev/input looked like the right tool, but
     * uinput node creation does not raise IN_CREATE on every kernel, and the devices also
     * appear before they are readable — a periodic poll is simple and never misses.
     */
    struct timeval tv = {1, 0}, *tvp = &tv;
    if (ev_trigger_held && !ev_trigger_hold_mode) { tv.tv_sec = 0; tv.tv_usec = 15000; }
    int ready = select(maxfd + 1, &fds, NULL, NULL, tvp);
    if (ready < 0 && errno != EINTR) break;

    if (ready > 0 && FD_ISSET(0, &fds)) {
      if (ev_stdin_line(buf, sizeof(buf)) < 0) break; /* parent died */
    }
    for (int i = source_count - 1; i >= 0; i--) {
      if (ready > 0 && FD_ISSET(sources[i].ev_fd, &fds)) {
        struct input_event evs[64];
        ssize_t got;
        int dead = 0;
        while ((got = read(sources[i].ev_fd, evs, sizeof(evs))) > (ssize_t)0) {
          for (size_t k = 0; k < (size_t)got / sizeof(evs[0]); k++) {
            unsigned short type = evs[k].type;
            if (type == EV_KEY) {
              ev_handle_key(&sources[i], evs[k].code, evs[k].value);
            } else if (type == EV_REL) {
              ev_handle_rel(&sources[i], evs[k].code, evs[k].value);
            } else if (type == EV_SYN) {
              emit_syn(&sources[i]);
            } else {
              emit_uinput(&sources[i], type, evs[k].code, evs[k].value);
            }
          }
        }
        if (got == 0 || (got < 0 && errno != EAGAIN && errno != EINTR)) dead = 1;
        if (dead) drop_source_at(i);
      }
    }
    for (int i = 0; i < kbd_count; i++) {
      if (ready > 0 && FD_ISSET(kbd_fds[i], &fds)) {
        struct input_event evs[64];
        ssize_t got;
        while ((got = read(kbd_fds[i], evs, sizeof(evs))) > (ssize_t)0) {
          for (size_t k = 0; k < (size_t)got / sizeof(evs[0]); k++) {
            if (evs[k].type == EV_KEY) {
              int bit = mod_key_bit(evs[k].code);
              if (bit) {
                if (evs[k].value != 0) ev_mod_mask |= bit;
                else ev_mod_mask &= ~bit;
              }
            }
          }
        }
        if (got == 0 || (got < 0 && errno != EAGAIN)) {
          close(kbd_fds[i]);
          kbd_fds[i] = kbd_fds[--kbd_count];
        }
      }
    }
    if (ready == 0) {
      rescan_input_devices(name_filter);
    }
    ev_poll_click_hold();
  }

  for (int i = source_count - 1; i >= 0; i--) drop_source_at(i);
  for (int i = 0; i < kbd_count; i++) close(kbd_fds[i]);
}

/* ---------------------------------------------------------------- entry */

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);

  if (argc > 1 && strcmp(argv[1], "foreground-focus") == 0) {
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
      fprintf(stderr, "rovyl-helper-linux: cannot open DISPLAY\n");
      return 1;
    }
    root = DefaultRootWindow(dpy);
    run_foreground_focus();
    return 0;
  }

  if (argc > 1 && strcmp(argv[1], "mouse-blocker") == 0) {
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
      fprintf(stderr, "rovyl-helper-linux: cannot open DISPLAY\n");
      return 1;
    }
    root = DefaultRootWindow(dpy);
    xfd = ConnectionNumber(dpy);
    /* stdin EOF doubles as the parent-death watch; the pid argument is kept for parity. */
    run_mouse_blocker();
    return 0;
  }

  if (argc > 1 && strcmp(argv[1], "mouse-blocker-evdev") == 0) {
    /* argv[3] is a dev-only device-name filter: grab only matching mice (tests, CI). */
    run_mouse_blocker_evdev(argc > 3 ? argv[3] : NULL);
    return 0;
  }

  fprintf(stderr, "Usage: rovyl-helper-linux [mouse-blocker <parentPid> | foreground-focus]\n");
  return 1;
}
