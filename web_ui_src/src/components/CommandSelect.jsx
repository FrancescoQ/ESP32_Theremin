import { h } from 'preact';
import { useState, useEffect } from 'preact/hooks';
import { useWebSocket } from '../hooks/WebSocketProvider';

/**
 * Reusable select dropdown that sends WebSocket commands on change
 */
export function CommandSelect({ label, options, value, onChange, commandGenerator, dataKey }) {
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
    const newValue = e.target.value;
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
    <div class="flex items-center justify-between p-4 bg-white rounded-lg shadow">
      <span class="text-gray-700 font-medium">{label}</span>

      <select
        value={localValue}
        onChange={handleChange}
        class="px-3 py-2 bg-gray-50 border border-gray-300 rounded-md text-gray-700 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors"
      >
        {options.map((option) => (
          <option key={option} value={option}>
            {option}
          </option>
        ))}
      </select>
    </div>
  );
}
