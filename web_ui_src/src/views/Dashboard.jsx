import { useWebSocket } from '../hooks/WebSocketProvider';
import { StatusCard } from '../components/StatusCard';
import { ControlButton } from '../components/ControlButton';
import { ToggleSwitch } from '../components/ToggleSwitch';

/**
 * Main Dashboard
 */
export function Dashboard() {
  const { data } = useWebSocket();

  return (
    <div class="max-w-7xl mx-auto px-4 py-8 sm:px-6 lg:px-8">

      {/* Metrics section */}
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 mb-4">Real-time Metrics</h2>

        <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
          <StatusCard
            title="Temperature"
            value={data.temperature}
            unit="°C"
            color="blue"
          />

          <StatusCard
            title="Humidity"
            value={data.humidity}
            unit="%"
            color="blue"
          />

          <StatusCard
            title="CPU Load"
            value={data.cpu_load}
            unit="%"
            color={data.cpu_load > 80 ? 'red' : 'green'}
          />

          <StatusCard
            title="Uptime"
            value={data.uptime}
            unit="min"
            color="gray"
          />
        </div>
      </section>

      {/* Controls section */}
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 mb-4">Controls</h2>

        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div class="bg-white rounded-lg shadow p-6">
            <h3 class="text-lg font-medium text-gray-900 mb-4">LED Control</h3>

            <div class="space-y-4">
              <ToggleSwitch
                label="LED 1"
                dataKey="led1"
                onCommand="LED1_ON"
                offCommand="LED1_OFF"
              />

              <ToggleSwitch
                label="LED 2"
                dataKey="led2"
                onCommand="LED2_ON"
                offCommand="LED2_OFF"
              />
            </div>
          </div>

          <div class="bg-white rounded-lg shadow p-6">
            <h3 class="text-lg font-medium text-gray-900 mb-4">Quick Actions</h3>

            <div class="flex flex-wrap gap-3">
              <ControlButton
                label="Restart"
                command="RESTART"
                variant="warning"
              />

              <ControlButton
                label="Reset"
                command="RESET"
                variant="danger"
              />

              <ControlButton
                label="Calibrate"
                command="CALIBRATE"
                variant="primary"
              />
            </div>
          </div>
        </div>
      </section>

      {/* Debug info */}
      <section class="mt-8">
        <details class="bg-white rounded-lg shadow p-4">
          <summary class="cursor-pointer font-medium text-gray-700">
            Debug: Raw WebSocket Data
          </summary>
          <pre class="mt-4 p-4 bg-gray-100 rounded overflow-auto text-xs">
            {JSON.stringify(data, null, 2)}
          </pre>
        </details>
      </section>

    </div>
  );
}
