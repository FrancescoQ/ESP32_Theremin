import { h } from 'preact';
import { useState, useEffect } from 'preact/hooks';
import { useWebSocket } from '../hooks/WebSocketProvider';

/**
 * Reusable slider that sends WebSocket commands on change
 * Displays values as percentages (0-100%) but can convert to other ranges
 */
export function CommandSlider({
  label,
  value,
  onChange,
  commandGenerator,
  dataKey,
  min = 0,
  max = 100,
  step = 1,
  unit = '%'
}) {
  const { data, send } = useWebSocket();
  const [localValue, setLocalValue] = useState(value);

  // Synchronize with WebSocket data if dataKey is provided
  useEffect(() => {
    if (data && dataKey && data[dataKey] !== undefined) {
      setLocalValue(data[dataKey]);
      if (onChange) {
        onChange(data[dataKey]);
      }
    }
  }, [data, dataKey, onChange]);

  // Synchronize with prop value
  useEffect(() => {
    setLocalValue(value);
  }, [value]);

  const handleChange = (e) => {
    const newValue = parseFloat(e.target.value);
    setLocalValue(newValue);

    // Update parent state
    if (onChange) {
      onChange(newValue);
    }

    // Send WebSocket command if commandGenerator is provided
    if (commandGenerator) {
      const command = commandGenerator(newValue);
      send(command);
    }
  };

  return (
    <div class="flex flex-col p-4 bg-white rounded-lg shadow space-y-2">
      <div class="flex items-center justify-between">
        <span class="text-gray-700 font-medium">{label}</span>
        <span class="text-gray-600 text-sm font-mono">
          {Math.round(localValue)}{unit}
        </span>
      </div>

      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={localValue}
        onChange={handleChange}
        class="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer slider"
        style={{
          background: `linear-gradient(to right, #3B82F6 0%, #3B82F6 ${localValue}%, #E5E7EB ${localValue}%, #E5E7EB 100%)`
        }}
      />
    </div>
  );
}
