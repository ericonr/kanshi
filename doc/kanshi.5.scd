kanshi(5)

# NAME

kanshi - configuration file

# DESCRIPTION

A kanshi configuration file is a list of profiles. Each profile has an optional
name and contains profile directives delimited by brackets (*{* and *}*).

Example:

```
include /etc/kanshi/config.d/*

profile {
	output LVDS-1 disable
	output "Some Company ASDF 4242" {
		mode 1600x900
		position 0,0
	}
}

profile nomad {
	output LVDS-1 enable scale 2
}
```

# DIRECTIVES

*profile* [<name>] { <profile directives...> }
	Defines a new profile using the specified bracket-delimited profile
	directives. A name can be specified but is optional.

*output* <criteria> <output-directive...>
	Defines defaults for output directives inside profile definitions.

	These defaults only apply when the respective output is mentioned in a
	specific profile. For example, the following two profiles are equivalent:

	```
	output eDP-1 scale 2

	profile manual {
		output eDP-1 scale 2
	}

	profile uses-defaults {
		output eDP-1
	}
	```

	Output directives may be specified in a bracket-delimited block as well.

*include* <path>
	Include as another file from _path_. Expands shell syntax (see *wordexp*(3)
	for details).

# PROFILE DIRECTIVES

Profile directives are followed by space-separated arguments. Arguments can be
quoted (with *"*) if they contain spaces.

*output* <criteria> <output-directive...>
	An output directive adds an output to the profile.

	The criteria can be one of:

	- An output name (e.g. "DP-1"). Note, output names may not be stable: they
	  may change across reboots (depending on kernel driver probe order) or
	  creation order (typically for USB-C docks).
	- A space-separated string containing the output manufacturer, model and
	  serial number (e.g. "Foocorp ASDF 1234"). If one of these fields is
	  missing, it needs to be populated with the string "Unknown" (e.g.
	  "Foocorp ASDF Unknown").
	- An output alias (e.g. "$work-desk3") defined by an output alias directive.
	  Output aliases can only be used in profile scope.
	- A wildcard "\*", to match any output.
	  Wildcards can only be used in profile scope and will only match one output.

	Output directives may be specified in a bracket-delimited block as well.

	On *sway*(1), output names and identifiers can be obtained via
	"swaymsg -t get_outputs".

*exec* <command>
	An exec directive executes a command when the profile was successfully
	applied. This can be used to update the compositor state to the profile
	when not done automatically.

	Commands are executed asynchronously and their order may not be preserved.
	If you need to execute sequential commands, you should collect in one exec
	statement or in a separate script.

	On *sway*(1) for example, *exec* can be used to move workspaces to the
	right output:

	```
	profile multihead {
		output eDP-1 enable
		output DP-1 enable transform 270
		exec swaymsg workspace 1, move workspace to eDP-1
	}
	```

	Note that some extra care must be taken with outputs identified by an
	output description as the real name may change:

	```
	profile complex {
		output "Some Other Company GTBZ 2525" mode 1920x1200
		exec swaymsg workspace 1, move workspace to output '"Some Other Company GTBZ 2525"'
	}
	```

# OUTPUT DIRECTIVES

*enable*|*disable*
	Enables or disables the specified output.

*mode* [--custom] <width>x<height>[@<rate>[Hz]]
	Configures the specified output to use the specified mode. Modes are a
	combination of width and height (in pixels) and a refresh rate (in Hz) that
	your display can be configured to use.

	Examples:

	```
	output HDMI-A-1 mode 1920x1080
	output HDMI-A-1 mode 1920x1080@60Hz
	output HDMI-A-1 mode --custom 1280x720@60Hz
	```

*position* <x>,<y>
	Places the output at the specified position in the global coordinates space.

	Example:

	```
	output HDMI-A-1 position 1600,0
	```

*scale* <factor>
	Scales the output by the specified scale factor.

*transform* <transform>
	Sets the output transform. Can be one of "90", "180", "270" for a rotation;
	or "flipped", "flipped-90", "flipped-180", "flipped-270" for a rotation and
	a flip; or "normal" for no transform.

*adaptive_sync* on|off
	Enables or disables adaptive synchronization (aka. Variable Refresh Rate).

*alias* $<name>
	Defines an alias for this output. Output aliases can only be defined in
	global scope.

# AUTHORS

Maintained by Simon Ser <contact@emersion.fr>, who is assisted by other
open-source contributors. For more information about kanshi development, see
<https://git.sr.ht/~emersion/kanshi>.

# SEE ALSO

*kanshi*(1)
