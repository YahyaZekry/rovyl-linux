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
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Mouse VKs the JS side sends (same numbers the Windows helper takes). */
#define VK_RIGHT 2
#define VK_MIDDLE 4
#define VK_X1 5
#define VK_X2 6

/* Button1..5 are left/right/middle/scroll-up/scroll-down in X11; X1/X2 are 8/9. */
#define BTN_LEFT 1
#define BTN_MIDDLE 2
#define BTN_RIGHT 3
#define BTN_SCROLL_UP 4
#define BTN_SCROLL_DOWN 5
#define BTN_X1 8
#define BTN_X2 9

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
    case VK_RIGHT: return BTN_RIGHT;
    case VK_MIDDLE: return BTN_MIDDLE;
    case VK_X1: return BTN_X1;
    case VK_X2: return BTN_X2;
    default: return BTN_MIDDLE;
  }
}

static int grabs_installed;

static void uninstall_grabs(void) {
  if (!grabs_installed) return;
  static const int all_buttons[] = {
    BTN_LEFT, BTN_MIDDLE, BTN_RIGHT, BTN_SCROLL_UP, BTN_SCROLL_DOWN, BTN_X1, BTN_X2,
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
    case BTN_MIDDLE: return "Middle";
    case BTN_X1: return "Mouse4";
    case BTN_X2: return "Mouse5";
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
    int is_recordable = button == BTN_MIDDLE || button == BTN_X1 ||
                        button == BTN_X2 || button == BTN_RIGHT;
    if (is_recordable) {
      int mods = query_modifier_mask();
      if (button == BTN_RIGHT && mods == 0) {
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
    grab_button(BTN_MIDDLE);
    grab_button(BTN_RIGHT);
    grab_button(BTN_X1);
    grab_button(BTN_X2);
  }
  if (blocking) {
    grab_button(BTN_LEFT);
    grab_button(BTN_MIDDLE);
    grab_button(BTN_RIGHT);
    grab_button(BTN_SCROLL_UP);
    grab_button(BTN_SCROLL_DOWN);
    grab_button(BTN_X1);
    grab_button(BTN_X2);
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
  char *parts[8] = {0};
  int n = 0;
  for (char *p = strtok_r(line, " ", &save); p && n < 8; p = strtok_r(NULL, " ", &save)) {
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

  fprintf(stderr, "Usage: rovyl-helper-linux [mouse-blocker <parentPid> | foreground-focus]\n");
  return 1;
}
