/*
 * Virtual test keyboard for E2E testing — creates a uinput keyboard named "e2e-test-kbd"
 * and replays keystrokes from stdin. Used by scripts/e2e/run-gesture-e2e.sh; the gesture
 * helper opens keyboards read-only (never grabbed), so the compositor sees every key too.
 *
 * stdin commands:
 *   k <code>       press and release a key
 *   d <code>       press (down)
 *   u <code>       release (up)
 */
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

static int fd;

static void ev(int type, int code, int value) {
	struct input_event e = {0};
	gettimeofday(&e.time, NULL);
	e.type = (unsigned short)type;
	e.code = (unsigned short)code;
	e.value = value;
	(void)!write(fd, &e, sizeof(e));
}

static void syn(void) { ev(EV_SYN, SYN_REPORT, 0); }

int main(void) {
	fd = open("/dev/uinput", O_WRONLY);
	if (fd < 0) { perror("open /dev/uinput"); return 1; }
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_EVBIT, EV_SYN);
	/* all of KEY_A..KEY_Z (30..38, 44..50), digits (2..11), F-keys (59..68,87,88), modifiers */
	for (int c = 1; c <= 120; c++) ioctl(fd, UI_SET_KEYBIT, c);
	struct uinput_user_dev u = {0};
	strcpy(u.name, "e2e-test-kbd");
	u.id.bustype = BUS_USB;
	u.id.vendor = 0x1234;
	u.id.product = 0x5679;
	(void)!write(fd, &u, sizeof(u));
	ioctl(fd, UI_DEV_CREATE);

	char line[64];
	while (fgets(line, sizeof(line), stdin)) {
		char op;
		int code = 0;
		if (sscanf(line, " %c %d", &op, &code) < 2) continue;
		if (op == 'k') { ev(EV_KEY, code, 1); syn(); ev(EV_KEY, code, 0); syn(); }
		else { ev(EV_KEY, code, op == 'd' ? 1 : 0); syn(); }
		fflush(stdout);
		usleep(20000);
	}
	ioctl(fd, UI_DEV_DESTROY);
	return 0;
}
