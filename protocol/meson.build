wayland_scanner = find_program('wayland-scanner')

wayland_scanner = dependency('wayland-scanner', version: '>=1.14.91', native: true)
wayland_scanner_path = wayland_scanner.get_variable(pkgconfig: 'wayland_scanner')
wayland_scanner_prog = find_program(wayland_scanner_path, native: true)

wayland_scanner_code = generator(
	wayland_scanner_prog,
	output: '@BASENAME@-protocol.c',
	arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(
	wayland_scanner_prog,
	output: '@BASENAME@-client-protocol.h',
	arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

protocols = [
	'wlr-output-management-unstable-v1.xml',
]

protocols_src = []
foreach xml : protocols
	protocols_src += wayland_scanner_code.process(xml)
	protocols_src += wayland_scanner_client.process(xml)
endforeach
