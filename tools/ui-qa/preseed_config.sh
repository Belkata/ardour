#!/bin/bash
# usage: preseed_config.sh <XDG_CONFIG_HOME>
# Fresh Ardour config that skips the first-run wizard and auto-starts the
# Dummy audio backend (no sound hardware in CI/containers).
mkdir -p "$1/ardour9"
touch "$1/ardour9/.a9"
# no "locked memory limit" warning dialog
echo '<?xml version="1.0" encoding="UTF-8"?><instant><no-memory-warning/></instant>' > "$1/ardour9/instant.xml"
cat > "$1/ardour9/config" <<'XML'
<?xml version="1.0" encoding="UTF-8"?>
<Ardour>
  <Config>
    <Option name="try-autostart-engine" value="1"/>
  </Config>
  <Extra>
    <AudioMIDISetup>
      <EngineStates>
        <State backend="None (Dummy)" driver="Normal Speed" device="Silence" input-device="" output-device="" sample-rate="48000" buffer-size="256" n-periods="2" input-latency="0" output-latency="0" lm-input="" lm-output="" active="1" use-buffered-io="0" midi-option="2 in, 2 out, Silence" lru="1">
          <MIDIDevices/>
        </State>
      </EngineStates>
    </AudioMIDISetup>
  </Extra>
</Ardour>
XML
