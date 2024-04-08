#include <ctype.h>
#include <errno.h>
#include <scfg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

#include <wayland-client.h>

#include "config.h"
#include "parser.h"

static bool parse_int(int *dst, const char *str) {
	char *end;
	errno = 0;
	int v = strtol(str, &end, 10);
	if (errno != 0 || end[0] != '\0' || str[0] == '\0') {
		return false;
	}
	*dst = v;
	return true;
}

static bool parse_mode(struct kanshi_profile_output *output, char *str) {
	const char *width = strtok(str, "x");
	const char *height = strtok(NULL, "@");
	const char *refresh = strtok(NULL, "");

	if (width == NULL || height == NULL) {
		fprintf(stderr, "invalid output mode: missing width/height\n");
		return false;
	}

	if (!parse_int(&output->mode.width, width)) {
		fprintf(stderr, "invalid output mode: invalid width\n");
		return false;
	}
	if (!parse_int(&output->mode.height, height)) {
		fprintf(stderr, "invalid output mode: invalid height\n");
		return false;
	}

	if (refresh != NULL) {
		char *end;
		errno = 0;
		float v = strtof(refresh, &end);
		if (errno != 0 || (end[0] != '\0' && strcmp(end, "Hz") != 0) ||
				str[0] == '\0') {
			fprintf(stderr, "invalid output mode: invalid refresh rate\n");
			return false;
		}
		output->mode.refresh = v * 1000;
	}

	return true;
}

static bool parse_position(struct kanshi_profile_output *output, char *str) {
	const char *x = strtok(str, ",");
	const char *y = strtok(NULL, "");

	if (x == NULL || y == NULL) {
		fprintf(stderr, "invalid output position: missing x/y\n");
		return false;
	}

	if (!parse_int(&output->position.x, x)) {
		fprintf(stderr, "invalid output position: invalid x\n");
		return false;
	}
	if (!parse_int(&output->position.y, y)) {
		fprintf(stderr, "invalid output position: invalid y\n");
		return false;
	}

	return true;
}

static bool parse_float(float *dst, const char *str) {
	char *end;
	errno = 0;
	float v = strtof(str, &end);
	if (errno != 0 || end[0] != '\0' || str[0] == '\0') {
		return false;
	}
	*dst = v;
	return true;
}

static bool parse_transform(enum wl_output_transform *dst, const char *str) {
	if (strcmp(str, "normal") == 0) {
		*dst = WL_OUTPUT_TRANSFORM_NORMAL;
	} else if (strcmp(str, "90") == 0) {
		*dst = WL_OUTPUT_TRANSFORM_90;
	} else if (strcmp(str, "180") == 0) {
		*dst = WL_OUTPUT_TRANSFORM_180;
	} else if (strcmp(str, "270") == 0) {
		*dst = WL_OUTPUT_TRANSFORM_270;
	} else if (strcmp(str, "flipped") == 0) {
		*dst = WL_OUTPUT_TRANSFORM_FLIPPED;
	} else if (strcmp(str, "flipped-90") == 0) {
		*dst = WL_OUTPUT_TRANSFORM_FLIPPED_90;
	} else if (strcmp(str, "flipped-180") == 0) {
		*dst = WL_OUTPUT_TRANSFORM_FLIPPED_180;
	} else if (strcmp(str, "flipped-270") == 0) {
		*dst = WL_OUTPUT_TRANSFORM_FLIPPED_270;
	} else {
		return false;
	}
	return true;
}

static bool parse_bool(bool *dst, const char *str) {
	if (strcmp(str, "on") == 0) {
		*dst = true;
	} else if (strcmp(str, "off") == 0) {
		*dst = false;
	} else {
		return false;
	}
	return true;
}

static ssize_t parse_profile_output_param(struct kanshi_profile_output *output,
		const char *name, char **params, size_t params_len) {
	if (strcmp(name, "enable") == 0) {
		output->fields |= KANSHI_OUTPUT_ENABLED;
		output->enabled = true;
		return 0;
	} else if (strcmp(name, "disable") == 0) {
		output->fields |= KANSHI_OUTPUT_ENABLED;
		output->enabled = false;
		return 0;
	}

	if (params_len == 0) {
		fprintf(stderr, "output directive '%s' requires at least one param\n",
			name);
		return -1;
	}

	char *value = params[0];
	enum kanshi_output_field key;
	size_t n = 1;
	if (strcmp(name, "mode") == 0) {
		key = KANSHI_OUTPUT_MODE;
		if (strcmp(value, "--custom") == 0) {
			output->mode.custom = true;
			if (params_len < 2) {
				fprintf(stderr, "output directive 'mode' is missing param\n");
				return -1;
			}
			value = params[1];
			n++;
		}
		if (!parse_mode(output, value)) {
			return -1;
		}
	} else if (strcmp(name, "position") == 0) {
		key = KANSHI_OUTPUT_POSITION;
		if (!parse_position(output, value)) {
			return -1;
		}
	} else if (strcmp(name, "scale") == 0) {
		key = KANSHI_OUTPUT_SCALE;
		if (!parse_float(&output->scale, value)) {
			fprintf(stderr, "invalid output scale\n");
			return -1;
		}
	} else if (strcmp(name, "transform") == 0) {
		key = KANSHI_OUTPUT_TRANSFORM;
		if (!parse_transform(&output->transform, value)) {
			fprintf(stderr, "invalid output transform\n");
			return -1;
		}
	} else if (strcmp(name, "adaptive_sync") == 0) {
		key = KANSHI_OUTPUT_ADAPTIVE_SYNC;
		if (!parse_bool(&output->adaptive_sync, value)) {
			fprintf(stderr, "invalid output adaptive_sync\n");
			return -1;
		}
	} else {
		fprintf(stderr,
			"unknown directive '%s' in profile output '%s'\n",
			name, output->name);
		return false;
	}

	output->fields |= key;
	return n;
}

static struct kanshi_profile_output *parse_profile_output(
		struct scfg_directive *dir) {
	if (dir->params_len == 0) {
		fprintf(stderr, "directive 'output': expected at least one param\n");
		fprintf(stderr, "(on line %d)\n", dir->lineno);
		return NULL;
	}

	struct kanshi_profile_output *output = calloc(1, sizeof(*output));

	output->name = strdup(dir->params[0]);

	size_t i = 1;
	while (i < dir->params_len) {
		const char *name = dir->params[i];
		ssize_t n = parse_profile_output_param(output, name,
			&dir->params[i + 1], dir->params_len - i - 1);
		if (n < 0) {
			fprintf(stderr, "(on line %d)\n", dir->lineno);
			return NULL;
		}
		i += 1 + n;
	}

	for (size_t i = 0; i < dir->children.directives_len; i++) {
		const struct scfg_directive *child = &dir->children.directives[i];

		ssize_t n = parse_profile_output_param(output, child->name,
			child->params, child->params_len);
		if (n < 0) {
			fprintf(stderr, "(on line %d)\n", child->lineno);
			return NULL;
		} else if ((size_t)n != child->params_len) {
			fprintf(stderr, "directive 'output': only one directive per line is allowed in output blocks\n");
			return NULL;
		}
	}

	return output;
}

static struct kanshi_profile_command *parse_profile_exec(
		struct scfg_directive *dir) {
	if (dir->params_len == 0) {
		fprintf(stderr, "directive 'exec': expected at least one param\n");
		fprintf(stderr, "(on line %d)\n", dir->lineno);
		return NULL;
	}

	// Unfortunately older versions of kanshi read the raw bytes from the
	// config file until the end of the line, bypassing the regular scfg
	// syntax. This makes it pretty painful to switch to a proper parser in a
	// backwards-compatible manner. Here's an attempt at maximizing backwards
	// compatibility by re-escaping the characters that libscfg has unescaped,
	// so that sh(1) can re-unescape these.
	char *str = NULL;
	size_t str_size = 0;
	FILE *f = open_memstream(&str, &str_size);
	for (size_t i = 0; i < dir->params_len; i++) {
		const char *param = dir->params[i];
		if (i > 0) {
			fprintf(f, " ");
		}
		for (size_t j = 0; param[j] != '\0'; j++) {
			char ch = param[j];
			if (ch == ' ' || ch == '\t' || ch == '\\' || ch == '\'' || ch == '"') {
				fprintf(f, "\\");
			}
			fprintf(f, "%c", ch);
		}
	}
	fclose(f);

	struct kanshi_profile_command *command = calloc(1, sizeof(*command));
	command->command = str;
	return command;
}

static struct kanshi_profile *parse_profile(struct scfg_directive *dir) {
	struct kanshi_profile *profile = calloc(1, sizeof(*profile));
	wl_list_init(&profile->outputs);
	wl_list_init(&profile->commands);

	if (dir->params_len > 1) {
		fprintf(stderr, "directive 'profile': expected zero or one param\n");
		fprintf(stderr, "(on line %d)\n", dir->lineno);
		return NULL;
	}
	if (dir->params_len > 0) {
		profile->name = strdup(dir->params[0]);
	}

	if (profile->name == NULL) {
		static int anon_profile_num = 1;
		char generated_name[100];
		snprintf(generated_name, sizeof(generated_name),
			"<anonymous profile %d>", anon_profile_num);
		anon_profile_num++;
		profile->name = strdup(generated_name);
	}

	for (size_t i = 0; i < dir->children.directives_len; i++) {
		struct scfg_directive *child = &dir->children.directives[i];

		if (strcmp(child->name, "output") == 0) {
			struct kanshi_profile_output *output = parse_profile_output(child);
			if (output == NULL) {
				return NULL;
			}
			// Store wildcard outputs at the end of the list
			if (strcmp(output->name, "*") == 0) {
				wl_list_insert(profile->outputs.prev, &output->link);
			} else {
				wl_list_insert(&profile->outputs, &output->link);
			}
		} else if (strcmp(child->name, "exec") == 0) {
			struct kanshi_profile_command *command = parse_profile_exec(child);
			if (command == NULL) {
				return NULL;
			}
			// Insert commands at the end to preserve order
			wl_list_insert(profile->commands.prev, &command->link);
		} else {
			fprintf(stderr, "profile '%s': unknown directive '%s'\n",
				profile->name, child->name);
			fprintf(stderr, "(on line %d)\n", child->lineno);
			return NULL;
		}
	}

	return profile;
}

static bool parse_config_file(const char *path, struct kanshi_config *config);

static bool parse_include_command(struct scfg_directive *dir, struct kanshi_config *config) {
	if (dir->params_len != 1) {
		fprintf(stderr, "directive 'include': expected exactly one parameter\n");
		fprintf(stderr, "(on line %d)\n", dir->lineno);
		return false;
	}

	wordexp_t p;
	if (wordexp(dir->params[0], &p, WRDE_SHOWERR | WRDE_UNDEF) != 0) {
		fprintf(stderr, "Could not expand include path: '%s'\n", dir->params[0]);
		return false;
	}

	char **w = p.we_wordv;
	for (size_t idx = 0; idx < p.we_wordc; idx++) {
		if (!parse_config_file(w[idx], config)) {
			fprintf(stderr, "Could not parse included config: '%s'\n", w[idx]);
			wordfree(&p);
			return false;
		}
	}
	wordfree(&p);
	return true;
}

static bool _parse_config(struct scfg_block *block, struct kanshi_config *config) {
	for (size_t i = 0; i < block->directives_len; i++) {
		struct scfg_directive *dir = &block->directives[i];

		if (strcmp(dir->name, "profile") == 0) {
			struct kanshi_profile *profile = parse_profile(dir);
			if (!profile) {
				return false;
			}
			wl_list_insert(config->profiles.prev, &profile->link);
		} else if (strcmp(dir->name, "output") == 0) {
			struct kanshi_profile_output *output_default = parse_profile_output(dir);
			if (!output_default) {
				return false;
			}
			wl_list_insert(config->output_defaults.prev, &output_default->link);
		} else if (strcmp(dir->name, "include") == 0) {
			if (!parse_include_command(dir, config)) {
				return false;
			}
		} else {
			fprintf(stderr, "unknown directive '%s'\n", dir->name);
			fprintf(stderr, "(on line %d)\n", dir->lineno);
			return false;
		}
	}

	return true;
}

static bool parse_config_file(const char *path, struct kanshi_config *config) {
	struct scfg_block block = {0};
	if (scfg_load_file(&block, path) != 0) {
		fprintf(stderr, "failed to parse config file\n");
		return false;
	}

	if (!_parse_config(&block, config)) {
		fprintf(stderr, "failed to parse config file\n");
		return false;
	}

	scfg_block_finish(&block);
	return true;
}

static void apply_output_defaults(struct kanshi_profile_output *profile_output,
		const struct kanshi_profile_output *output_default) {
	profile_output->fields |= output_default->fields;

	if (!(profile_output->fields & KANSHI_OUTPUT_ENABLED)) {
		profile_output->enabled = output_default->enabled;
	}
	if (!(profile_output->fields & KANSHI_OUTPUT_MODE)) {
		profile_output->mode = output_default->mode;
	}
	if (!(profile_output->fields & KANSHI_OUTPUT_POSITION)) {
		profile_output->position = output_default->position;
	}
	if (!(profile_output->fields & KANSHI_OUTPUT_SCALE)) {
		profile_output->scale = output_default->scale;
	}
	if (!(profile_output->fields & KANSHI_OUTPUT_TRANSFORM)) {
		profile_output->transform = output_default->transform;
	}
	if (!(profile_output->fields & KANSHI_OUTPUT_ADAPTIVE_SYNC)) {
		profile_output->adaptive_sync = output_default->adaptive_sync;
	}
}

static void resolve_output_defaults(struct kanshi_config *config) {
	struct kanshi_profile *profile;
	wl_list_for_each(profile, &config->profiles, link) {
		struct kanshi_profile_output *profile_output;
		wl_list_for_each(profile_output, &profile->outputs, link) {
			struct kanshi_profile_output *output_default;
			wl_list_for_each(output_default, &config->output_defaults, link) {
				if (strcmp(profile_output->name, output_default->name) == 0) {
					apply_output_defaults(profile_output, output_default);
					break;
				}
			}
		}
	}
}

struct kanshi_config *parse_config(const char *path) {
	struct kanshi_config *config = calloc(1, sizeof(*config));
	if (config == NULL) {
		return NULL;
	}
	wl_list_init(&config->output_defaults);
	wl_list_init(&config->profiles);

	if (!parse_config_file(path, config)) {
		free(config);
		return NULL;
	}

	resolve_output_defaults(config);

	return config;
}

static void destroy_output(struct kanshi_profile_output *output) {
	free(output->name);
	wl_list_remove(&output->link);
	free(output);
}

void destroy_config(struct kanshi_config *config) {
	struct kanshi_profile_output *output_default, *tmp_output_default;
	wl_list_for_each_safe(output_default, tmp_output_default, &config->output_defaults, link) {
		destroy_output(output_default);
	}

	struct kanshi_profile *profile, *profile_tmp;
	wl_list_for_each_safe(profile, profile_tmp, &config->profiles, link) {
		struct kanshi_profile_output *output, *output_tmp;
		wl_list_for_each_safe(output, output_tmp, &profile->outputs, link) {
			destroy_output(output);
		}

		struct kanshi_profile_command *cmd, *cmd_tmp;
		wl_list_for_each_safe(cmd, cmd_tmp, &profile->commands, link) {
			free(cmd->command);
			wl_list_remove(&cmd->link);
			free(cmd);
		}

		free(profile->name);
		wl_list_remove(&profile->link);
		free(profile);
	}

	free(config);
}
