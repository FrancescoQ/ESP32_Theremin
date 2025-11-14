import { h } from 'preact';
import { useWebSocket } from './WebSocketProvider';

/**
 * Button to send commands via WebSocket
 */
export function ControlButton({ label, command, payload, variant = 'primary' }) {
  const { send, connected } = useWebSocket();

  const handleClick = () => {
    if (payload) {
      send(payload);
    } else if (command) {
      send({ command });
    }
  };

  const variants = {
    primary: 'bg-blue-500 hover:bg-blue-600 text-white',
    success: 'bg-green-500 hover:bg-green-600 text-white',
    danger: 'bg-red-500 hover:bg-red-600 text-white',
    warning: 'bg-yellow-500 hover:bg-yellow-600 text-white'
  };

  return (
    <button
      onClick={handleClick}
      disabled={!connected}
      class={`
        px-4 py-2 rounded-lg font-medium
        transition-colors duration-200
        disabled:opacity-50 disabled:cursor-not-allowed
        ${variants[variant]}
      `}
    >
      {label}
    </button>
  );
}
