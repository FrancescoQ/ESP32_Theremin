import { useWebSocket } from '../hooks/WebSocketProvider';
import { Effect } from '../components/Effect';

/**
 * Effects View
 */
export function Effects() {
  const { data } = useWebSocket();

  return (
    <div class="max-w-7xl mx-auto px-4 py-8 sm:px-6 lg:px-8">
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 dark:text-white mb-4">Effects</h2>

        <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
          <div class="bg-white dark:bg-gray-800 rounded-lg shadow p-6">
            <h3 class="text-lg font-medium text-gray-900 dark:text-white mb-4">Delay</h3>
            <Effect effectName="delay" />
          </div>
          <div class="bg-white dark:bg-gray-800 rounded-lg shadow p-6">
            <h3 class="text-lg font-medium text-gray-900 dark:text-white mb-4">Chorus</h3>
            <Effect effectName="chorus" />
          </div>
          <div class="bg-white dark:bg-gray-800 rounded-lg shadow p-6">
            <h3 class="text-lg font-medium text-gray-900 dark:text-white mb-4">Reverb</h3>
            <Effect effectName="reverb" />
          </div>
        </div>
      </section>
    </div>
  );
}
