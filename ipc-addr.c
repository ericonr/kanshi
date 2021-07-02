#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>

#include "ipc.h"

int check_env(void) {
	char *wayland_socket = getenv("WAYLAND_DISPLAY");
	char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
	if (!wayland_socket || !wayland_socket[0]) {
		fprintf(stderr, "WAYLAND_DISPLAY is not set\n");
		return -1;
	}
	if (!xdg_runtime_dir || !xdg_runtime_dir[0]) {
		fprintf(stderr, "XDG_RUNTIME_DIR is not set\n");
		return -1;
	}

	return 0;
}

int get_ipc_address(char *address, size_t size) {
	return snprintf(address, size, "unix:%s/fr.emersion.kanshi.%s",
			getenv("XDG_RUNTIME_DIR"), getenv("WAYLAND_DISPLAY"));
}
