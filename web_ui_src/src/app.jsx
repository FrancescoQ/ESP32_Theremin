import { h } from 'preact';
import { render } from 'preact';
import { WebSocketProvider, useWebSocket } from './components/WebSocketProvider';
import { StatusCard } from './components/StatusCard';
import { ControlButton } from './components/ControlButton';
import { ToggleSwitch } from './components/ToggleSwitch';
import './styles.css';

/**
 * Header con status connessione
 */
function Header() {
  const { connected } = useWebSocket();

  return (
    <header class="bg-white shadow-md">
      <div class="max-w-7xl mx-auto px-4 py-4 sm:px-6 lg:px-8">
        <div class="flex items-center justify-between">
          <h1 class="text-2xl font-bold text-gray-900">
            TheremAIn Control Panel
          </h1>

          <div class="flex items-center gap-2">
            <div class={`w-3 h-3 rounded-full ${connected ? 'bg-green-500' : 'bg-red-500'}`} />
            <span class="text-sm text-gray-600">
              {connected ? 'Connesso' : 'Disconnesso'}
            </span>
          </div>
        </div>
      </div>
    </header>
  );
}

/**
 * Dashboard principale
 */
function Dashboard() {
  const { data } = useWebSocket();

  return (
    <div class="max-w-7xl mx-auto px-4 py-8 sm:px-6 lg:px-8">

      {/* Sezione metriche */}
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 mb-4">Metriche in tempo reale</h2>

        <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
          <StatusCard
            title="Temperatura"
            value={data.temperature}
            unit="°C"
            icon="🌡️"
            color="blue"
          />

          <StatusCard
            title="Umidità"
            value={data.humidity}
            unit="%"
            icon="💧"
            color="blue"
          />

          <StatusCard
            title="CPU Load"
            value={data.cpu_load}
            unit="%"
            icon="⚙️"
            color={data.cpu_load > 80 ? 'red' : 'green'}
          />

          <StatusCard
            title="Uptime"
            value={data.uptime}
            unit="min"
            icon="⏱️"
            color="gray"
          />
        </div>
      </section>

      {/* Sezione controlli */}
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 mb-4">Controlli</h2>

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
            <h3 class="text-lg font-medium text-gray-900 mb-4">Azioni rapide</h3>

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

/**
 * App principale
 */
function App() {
  return (
    <WebSocketProvider>
      <div class="min-h-screen bg-gray-100">
        <Header />
        <Dashboard />
      </div>
    </WebSocketProvider>
  );
}

// Render!
render(h(App), document.getElementById('app'));
