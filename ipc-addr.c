#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>

#include "ipc.h"

int get_ipc_address(char *address, size_t size) {
	const char *socket = getenv("WAYLAND_DISPLAY");
	// if WAYLAND_DISPLAY wasn't set, libwayland will have used "wayland-0"
	if (socket == NULL) {
		socket = "wayland-0";
	}
	return snprintf(address, size, "unix:@fr.emersion.kanshi.%s", socket);
}
