import { WebSocketProvider, useWebSocket } from '../hooks/WebSocketProvider';

/**
 * Header with connection status
 */
export function Header({ setView }) {
  const { connected } = useWebSocket();

  return (
    <header class="bg-white shadow-md">
      <div class="max-w-7xl mx-auto px-4 py-4 sm:px-6 lg:px-8">
        <div class="flex items-center justify-between">
          <h1 class="text-2xl font-bold text-gray-900">
            Therem[AI]n Control Panel
          </h1>

          <div class="flex items-center gap-2">
            <div class={`w-3 h-3 rounded-full ${connected ? 'bg-green-500' : 'bg-red-500'}`} />
            <span class="text-sm text-gray-600">
              {connected ? 'Connected' : 'Disconnected'}
            </span>
          </div>
        </div>
      </div>
    </header>
  );
}
