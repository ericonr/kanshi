project(
	'kanshi',
	'c',
	version: '1.7.0',
	license: 'MIT',
	meson_version: '>=0.59.0',
	default_options: [
		'c_std=c11',
		'warning_level=3',
		'werror=true',
	],
)

cc = meson.get_compiler('c')

add_project_arguments(cc.get_supported_arguments([
	'-Wundef',
	'-Wlogical-op',
	'-Wmissing-include-dirs',
	'-Wold-style-definition',
	'-Wpointer-arith',
	'-Winit-self',
	'-Wfloat-equal',
	'-Wstrict-prototypes',
	'-Wredundant-decls',
	'-Wimplicit-fallthrough=2',
	'-Wendif-labels',
	'-Wstrict-aliasing=2',
	'-Woverflow',
	'-Wformat=2',

	'-Wno-missing-braces',
	'-Wno-missing-field-initializers',
	'-Wno-unused-parameter',
]), language: 'c')

wayland_client = dependency('wayland-client')
scfg = dependency('scfg', fallback: 'libscfg')
varlinkgen = dependency('varlinkgen', required: get_option('ipc'))

add_project_arguments([
	'-D_POSIX_C_SOURCE=200809L',
	'-DKANSHI_VERSION="@0@"'.format(meson.project_version()),
	'-DKANSHI_HAS_VARLINK=@0@'.format(varlinkgen.found().to_int()),
], language: 'c')

subdir('protocol')

kanshi_deps = [
	wayland_client,
	scfg,
]

kanshi_srcs = [
	'event-loop.c',
	'main.c',
	'config.c',
	'ipc-addr.c',
]

if varlinkgen.found()
	varlinkgen_prog = find_program('varlinkgen')
	kanshi_gen = custom_target(
		'kanshi_gen',
		command: [varlinkgen_prog, '--prefix=kanshi', '@INPUT@', '@OUTPUT0@', '@OUTPUT1@'],
		input: 'kanshi.varlink',
		output: ['kanshi-ipc.h', 'kanshi-ipc.c'],
	)

	kanshi_deps += varlinkgen
	kanshi_srcs += ['ipc.c', 'ipc-addr.c', kanshi_gen]
endif

executable(
	meson.project_name(),
	kanshi_srcs + protocols_src,
	include_directories: 'include',
	dependencies: kanshi_deps,
	install: true,
)

if varlinkgen.found()
	executable(
		meson.project_name() + 'ctl',
		[
			'ctl.c',
			'ipc-addr.c',
			kanshi_gen,
		],
		include_directories: 'include',
		dependencies: [varlinkgen],
		install: true,
	)
endif

subdir('doc')

summary({
	'Man pages': scdoc.found(),
	'IPC': varlinkgen.found(),
}, bool_yn: true)
