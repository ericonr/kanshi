#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <varlinkgen.h>

#include "kanshi-ipc.h"
#include "ipc.h"

static void usage(void) {
	fprintf(stderr, "Usage: kanshictl [command]\n"
		"\n"
		"Commands:\n"
		"  reload            Reload the configuration file\n"
		"  switch <profile>  Switch to another profile\n");
}

static void print_error(const struct varlinkgen_error *err) {
	if (kanshi_error_ProfileNotFound_from(NULL, err)) {
		fprintf(stderr, "Profile not found\n");
	} else if (kanshi_error_ProfileNotMatched_from(NULL, err)) {
		fprintf(stderr, "Profile does not match the current output configuration\n");
	} else if (kanshi_error_ProfileNotApplied_from(NULL, err)) {
		fprintf(stderr, "Profile could not be applied by the compositor\n");
	} else {
		fprintf(stderr, "Error: %s\n", err->name);
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		usage();
		return EXIT_FAILURE;
	}
	if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
		usage();
		return EXIT_SUCCESS;
	}

	char address[PATH_MAX];
	if (get_ipc_address(address, sizeof(address)) < 0) {
		return EXIT_FAILURE;
	}

	struct varlinkgen_client *client = varlinkgen_client_connect_unix(address);
	if (client == NULL) {
		fprintf(stderr, "Couldn't connect to kanshi at %s.\n"
				"Is the kanshi daemon running?\n", address);
		return EXIT_FAILURE;
	}

	const char *command = argv[1];
	bool ret;
	if (strcmp(command, "reload") == 0) {
		struct kanshi_Reload_in in = {0};
		struct kanshi_Reload_out out = {0};

		ret = kanshi_Reload(client, &in, &out, NULL);
		kanshi_Reload_out_finish(&out);
	} else if (strcmp(command, "switch") == 0) {
		if (argc < 3) {
			usage();
			return EXIT_FAILURE;
		}
		char *profile = strdup(argv[2]);
		if (profile == NULL) {
			perror("strdup");
			return EXIT_FAILURE;
		}

		struct kanshi_Switch_in in = { .profile = profile };
		struct kanshi_Switch_out out = {0};

		struct varlinkgen_error err;
		ret = kanshi_Switch(client, &in, &out, &err);
		if (!ret) {
			print_error(&err);
		}
		kanshi_Switch_out_finish(&out);
	} else {
		fprintf(stderr, "invalid command: %s\n", argv[1]);
		usage();
		return EXIT_FAILURE;
	}

	if (!ret) {
		fprintf(stderr, "varlink command failed\n");
		return EXIT_FAILURE;
	}

	varlinkgen_client_destroy(client);

	return EXIT_SUCCESS;
}
