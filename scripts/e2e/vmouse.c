/*
 * Virtual test mouse for E2E testing — creates a uinput device named "e2e-test-mouse"
 * and replays scripted input from stdin. Used by scripts/e2e/run-gesture-e2e.sh; the
 * gesture helper's name-filter scopes grabbing to this device only, so the tester's real
 * mouse is never touched.
 *
 * stdin commands:
 *   m <dx> <dy>    relative move
 *   p <btn>        press   (1 left, 2 middle, 3 right)
 *   r <btn>        release
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
	ioctl(fd, UI_SET_EVBIT, EV_REL);
	ioctl(fd, UI_SET_EVBIT, EV_SYN);
	ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
	ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
	ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
	ioctl(fd, UI_SET_RELBIT, REL_X);
	ioctl(fd, UI_SET_RELBIT, REL_Y);
	ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
	struct uinput_user_dev u = {0};
	strcpy(u.name, "e2e-test-mouse");
	u.id.bustype = BUS_USB;
	u.id.vendor = 0x1234;
	u.id.product = 0x5678;
	(void)!write(fd, &u, sizeof(u));
	ioctl(fd, UI_DEV_CREATE);

	char line[64];
	while (fgets(line, sizeof(line), stdin)) {
		char op;
		int a = 0, b = 0;
		if (sscanf(line, " %c %d %d", &op, &a, &b) < 2) continue;
		if (op == 'm') { ev(EV_REL, REL_X, a); ev(EV_REL, REL_Y, b); syn(); }
		else {
			int code = a == 1 ? BTN_LEFT : a == 2 ? BTN_MIDDLE : BTN_RIGHT;
			ev(EV_KEY, code, op == 'p' ? 1 : 0);
			syn();
		}
		fflush(stdout);
		usleep(20000);
	}
	ioctl(fd, UI_DEV_DESTROY);
	return 0;
}
