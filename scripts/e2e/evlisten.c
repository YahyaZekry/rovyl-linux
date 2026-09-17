/*
 * Prints EV_KEY / EV_REL events read from a device node.
 * Usage: evlisten /dev/input/eventNN
 */
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
	int fd = open(argv[1], O_RDONLY | O_NONBLOCK);
	if (fd < 0) { perror("open"); return 1; }
	struct input_event e;
	for (;;) {
		int n = (int)read(fd, &e, sizeof(e));
		if (n <= 0) { usleep(1000); continue; }
		if (e.type == EV_KEY)
			printf("KEY %u %d\n", e.code, e.value);
		else if (e.type == EV_REL)
			printf("REL %u %d\n", e.code, e.value);
		fflush(stdout);
	}
}
