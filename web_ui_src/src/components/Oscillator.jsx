import { useState } from "preact/hooks";
import { ToggleSwitch } from "./ToggleSwitch";
import { CommandSelect } from "./CommandSelect";
import { CommandSlider } from "./CommandSlider";

export function Oscillator({id}) {
  // Track current waveform selection (default to SINE)
  const [currentWaveform, setCurrentWaveform] = useState("SINE");

  // Track volume as percentage (0-100)
  const [volume, setVolume] = useState(100);

  // Track octave shift (-1, 0, +1)
  const [octave, setOctave] = useState("0");

  return (
    <div class="space-y-4">
      <ToggleSwitch
        label="Status"
        onCommand={{
          cmd: "setWaveform",
          osc: id,
          value: currentWaveform
        }}
        offCommand={{
          cmd: "setWaveform",
          osc: id,
          value: "OFF"
        }}
      />

      <CommandSelect
        label="Waveform"
        options={["SINE", "SQUARE", "TRIANGLE", "SAW"]}
        value={currentWaveform}
        onChange={setCurrentWaveform}
        commandGenerator={(value) => ({
          cmd: "setWaveform",
          osc: id,
          value: value
        })}
      />

      <CommandSelect
        label="Octave"
        options={["-1", "0", "1"]}
        value={octave}
        onChange={setOctave}
        commandGenerator={(value) => ({
          cmd: "setOctave",
          osc: id,
          value: parseInt(value, 10)
        })}
      />

      <CommandSlider
        label="Volume"
        value={volume}
        onChange={setVolume}
        min={0}
        max={100}
        step={1}
        unit="%"
        commandGenerator={(percentage) => ({
          cmd: "setVolume",
          osc: id,
          value: percentage / 100.0
        })}
      />
    </div>
  );
}
