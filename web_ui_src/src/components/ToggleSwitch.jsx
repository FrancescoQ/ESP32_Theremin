import { h } from 'preact';
import { useState, useEffect } from 'preact/hooks';
import { useWebSocket } from './WebSocketProvider';

/**
 * iOS-style toggle switch for on/off controls
 */
export function ToggleSwitch({ label, dataKey, onCommand, offCommand }) {
  const { data, send } = useWebSocket();
  const [isOn, setIsOn] = useState(false);

  // Synchronize state with WebSocket data
  useEffect(() => {
    if (data && dataKey && data[dataKey] !== undefined) {
      setIsOn(!!data[dataKey]);
    }
  }, [data, dataKey]);

  const handleToggle = () => {
    const newState = !isOn;
    setIsOn(newState);

    // Send appropriate command
    if (newState && onCommand) {
      send({ command: onCommand });
    } else if (!newState && offCommand) {
      send({ command: offCommand });
    }
  };

  return (
    <div class="flex items-center justify-between p-4 bg-white rounded-lg shadow">
      <span class="text-gray-700 font-medium">{label}</span>

      <button
        onClick={handleToggle}
        class={`
          relative inline-flex h-8 w-14 items-center rounded-full
          transition-colors duration-200 ease-in-out
          focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2
          ${isOn ? 'bg-blue-500' : 'bg-gray-300'}
        `}
      >
        <span
          class={`
            inline-block h-6 w-6 transform rounded-full bg-white
            transition-transform duration-200 ease-in-out
            ${isOn ? 'translate-x-7' : 'translate-x-1'}
          `}
        />
      </button>
    </div>
  );
}
