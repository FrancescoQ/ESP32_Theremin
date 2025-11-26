import { useState, useEffect } from "preact/hooks";
import { useWebSocket } from "../hooks/WebSocketProvider";
import { CommandSelect } from "./CommandSelect";
import { CommandSlider } from "./CommandSlider";

export function Oscillator({id}) {
  const { data } = useWebSocket();

  // Track current waveform selection (default to SINE)
  const [currentWaveform, setCurrentWaveform] = useState("SINE");

  // Track volume as percentage (0-100)
  const [volume, setVolume] = useState(100);

  // Track octave shift (-1, 0, +1)
  const [octave, setOctave] = useState("0");

  // Sync local state with WebSocket data when it updates
  useEffect(() => {
    const oscData = data.oscillators?.[id];
    if (oscData) {
      // Update waveform if available
      if (oscData.waveform) {
        setCurrentWaveform(oscData.waveform);
      }

      // Update volume (convert from 0.0-1.0 to 0-100)
      if (oscData.volume !== undefined) {
        setVolume(Math.round(oscData.volume * 100));
      }

      // Update octave (convert number to string)
      if (oscData.octave !== undefined) {
        setOctave(String(oscData.octave));
      }
    }
  }, [data.oscillators, id]);

  // Get status color based on waveform
  const statusColor = currentWaveform && currentWaveform !== 'OFF' ? 'bg-green-500' : 'bg-red-500';

  return (
    <div class="space-y-4">
      {/* Title with status indicator */}
      <h3 class="text-lg font-medium text-gray-900 dark:text-white flex items-center gap-2">
        <span class={`w-2 h-2 rounded-full ${statusColor}`}></span>
        Oscillator {id}
      </h3>
      <CommandSelect
        label="Waveform"
        options={["OFF", "SINE", "SQUARE", "TRIANGLE", "SAW"]}
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
