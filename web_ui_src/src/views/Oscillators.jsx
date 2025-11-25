import { useWebSocket } from '../hooks/WebSocketProvider';
import { ControlButton } from '../components/ControlButton';
import { ToggleSwitch } from '../components/ToggleSwitch';
import { Oscillator } from '../components/Oscillator';

/**
 * Main Dashboard
 */
export function Oscillators() {
  const { data } = useWebSocket();

  return (
    <div class="max-w-7xl mx-auto px-4 py-8 sm:px-6 lg:px-8">
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 mb-4">Oscillators</h2>

        <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
          <div class="bg-white rounded-lg shadow p-6">
            <h3 class="text-lg font-medium text-gray-900 mb-4">Oscillator 1</h3>
            <Oscillator id="1" />
          </div>
          <div class="bg-white rounded-lg shadow p-6">
            <h3 class="text-lg font-medium text-gray-900 mb-4">Oscillator 2</h3>
            <Oscillator id="2" />
          </div>
          <div class="bg-white rounded-lg shadow p-6">
            <h3 class="text-lg font-medium text-gray-900 mb-4">Oscillator 3</h3>
            <Oscillator id="3" />
          </div>
        </div>
      </section>
    </div>
  );
}
