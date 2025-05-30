-- /usr/share/wireplumber/bluetooth.lua.d/50-bluez-config.lua

bluez_monitor.enabled = true

bluez_monitor.properties = {
  ["bluez5.enable-hw-volume"] = false,
  ["with-logind"] = true,
}

bluez_monitor.rules = {
  {
    matches = {
      {
        { "device.name", "matches", "bluez_card.*" },
      },
    },
    apply_properties = {
      ["bluez5.auto-connect"]  = "[ hfp_hf hsp_hs a2dp_sink ]",
    },
  },
  {
    matches = {
      {
        { "node.name", "matches", "bluez_input.*" },
      },
      {
        { "node.name", "matches", "bluez_output.*" },
      },
    },
    apply_properties = {
    },
  },
}
