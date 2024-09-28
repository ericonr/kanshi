#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <varlinkgen.h>

#include "kanshi-ipc.h"
#include "config.h"
#include "kanshi.h"
#include "ipc.h"

struct apply_profile_done_data {
	struct varlinkgen_service_call *call;
	bool success;
};

static struct kanshi_state *get_state_from_call(struct varlinkgen_service_call *call) {
	struct varlinkgen_service *service = varlinkgen_service_call_get_service(call);
	return varlinkgen_service_get_user_data(service);
}

static void apply_profile_done(void *data, bool success) {
	struct apply_profile_done_data *pdata = data;
	if (!success) {
		kanshi_fail_ProfileNotApplied(pdata->call, NULL);
	}
	pdata->success = success;
}

static bool handle_reload(struct varlinkgen_service_call *call, const struct kanshi_Reload_in *in, struct kanshi_Reload_out *out) {
	struct kanshi_state *state = get_state_from_call(call);

	struct apply_profile_done_data data = {
		.call = call,
	};

	if (!kanshi_reload_config(state, apply_profile_done, &data)) {
		kanshi_fail_ProfileNotMatched(call, NULL);
		return false;
	}
	return data.success;
}

static bool handle_switch(struct varlinkgen_service_call *call, const struct kanshi_Switch_in *in, struct kanshi_Switch_out *out) {
	struct kanshi_state *state = get_state_from_call(call);

	struct apply_profile_done_data data = {
		.call = call,
	};

	struct kanshi_profile *profile;
	bool found = false;
	bool matched = false;
	wl_list_for_each(profile, &state->config->profiles, link) {
		if (strcmp(profile->name, in->profile) != 0) {
			continue;
		}

		found = true;
		if (kanshi_switch(state, profile, apply_profile_done, &data)) {
			matched = true;
			break;
		}
	}
	if (!found) {
		kanshi_fail_ProfileNotFound(call, NULL);
		return false;
	}
	if (!matched) {
		kanshi_fail_ProfileNotMatched(call, NULL);
		return false;
	}

	return data.success;
}

static const struct kanshi_handler kanshi_handler = {
	.Reload = handle_reload,
	.Switch = handle_switch,
};

int kanshi_init_ipc(struct kanshi_state *state, int listen_fd) {
	struct varlinkgen_service *service = varlinkgen_service_create();
	varlinkgen_service_set_user_data(service, state);

	char address[PATH_MAX];
	if (get_ipc_address(address, sizeof(address)) < 0) {
		return -1;
	}

	const struct varlinkgen_registry_options registry_options = {
		.vendor = "emersion",
		.product = "kanshi",
		.version = KANSHI_VERSION,
		.url = "https://wayland.emersion.fr/kanshi/",
	};
	struct varlinkgen_registry *registry = varlinkgen_registry_create(&registry_options);
	varlinkgen_registry_add(registry, &kanshi_interface, kanshi_get_call_handler(&kanshi_handler));
	varlinkgen_service_set_call_handler(service, varlinkgen_registry_get_call_handler(registry));

	/* XXX: remove +5 ("unix:" prefix) when kanshi starts using varlinkgen */
	if (listen_fd < 0) {
		unlink(address+5);
		if (!varlinkgen_service_listen_unix(service, address+5)) {
			return -1;
		}
	} else {
		if (!varlinkgen_service_listen_fd(service, listen_fd)) {
			return -1;
		}
	}

	state->service = service;

	return 0;
}

void kanshi_finish_ipc(struct kanshi_state *state) {
	if (state->service) {
		varlinkgen_service_destroy(state->service);
		state->service = NULL;
	}
}
