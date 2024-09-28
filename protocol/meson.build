wl_mod = import('unstable-wayland')

wlr_output_management = wl_mod.find_protocol(
  'wlr-output-management',
  state: 'unstable',
  version: 1,
  vendored: true,
)

protocols_src = wl_mod.scan_xml(
  wlr_output_management,
  client: true,
  server: false,
)
