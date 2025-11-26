import { useWebSocket } from '../hooks/WebSocketProvider';
import { StatusCard } from '../components/StatusCard';

/**
 * Main Dashboard - Overview of Theremin Status
 */
export function Dashboard() {
  const { data } = useWebSocket();

  // Helper to format uptime
  const formatUptime = (ms) => {
    if (!ms) return '0s';
    const seconds = Math.floor(ms / 1000);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);
    const days = Math.floor(hours / 24);

    if (days > 0) return `${days}d ${hours % 24}h`;
    if (hours > 0) return `${hours}h ${minutes % 60}m`;
    if (minutes > 0) return `${minutes}m ${seconds % 60}s`;
    return `${seconds}s`;
  };

  // Helper to format RAM
  const formatRAM = (bytes) => {
    if (!bytes) return '0 KB';
    const kb = bytes / 1024;
    if (kb > 1024) {
      return `${(kb / 1024).toFixed(2)} MB`;
    }
    return `${kb.toFixed(2)} KB`;
  };

  return (
    <div class="max-w-7xl mx-auto px-4 py-8 sm:px-6 lg:px-8">

      {/* Oscillators Status */}
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 dark:text-white mb-4">Oscillators</h2>
        <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
          {[1, 2, 3].map(oscNum => {
            const osc = data.oscillators?.[oscNum] || {};
            const isActive = osc.waveform && osc.waveform !== 'OFF';

            return (
              <div key={oscNum} class="bg-white dark:bg-gray-800 rounded-lg shadow p-6">
                <div class="flex items-center justify-between mb-4">
                  <h3 class="text-lg font-medium text-gray-900 dark:text-white">Oscillator {oscNum}</h3>
                  <span class={`px-2 py-1 rounded text-xs font-semibold ${
                    isActive ? 'bg-green-100 text-green-800' : 'bg-gray-100 text-gray-800'
                  }`}>
                    {isActive ? 'ACTIVE' : 'OFF'}
                  </span>
                </div>

                <div class="space-y-2 text-sm">
                  <div class="flex justify-between">
                    <span class="text-gray-600 dark:text-gray-400">Waveform:</span>
                    <span class="font-medium text-gray-900 dark:text-white">{osc.waveform || 'OFF'}</span>
                  </div>
                  <div class="flex justify-between">
                    <span class="text-gray-600 dark:text-gray-400">Octave:</span>
                    <span class="font-medium text-gray-900 dark:text-white">{osc.octave !== undefined ? (osc.octave > 0 ? `+${osc.octave}` : osc.octave) : '0'}</span>
                  </div>
                  <div class="flex justify-between">
                    <span class="text-gray-600 dark:text-gray-400">Volume:</span>
                    <span class="font-medium text-gray-900 dark:text-white">{osc.volume !== undefined ? `${Math.round(osc.volume * 100)}%` : '0%'}</span>
                  </div>
                </div>
              </div>
            );
          })}
        </div>
      </section>

      {/* Effects Status */}
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 dark:text-white mb-4">Effects</h2>
        <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
          {/* Delay */}
          <div class="bg-white dark:bg-gray-800 rounded-lg shadow p-6">
            <div class="flex items-center justify-between mb-4">
              <h3 class="text-lg font-medium text-gray-900 dark:text-white">Delay</h3>
              <span class={`px-2 py-1 rounded text-xs font-semibold ${
                data.effects?.delay?.enabled ? 'bg-blue-100 text-blue-800' : 'bg-gray-100 text-gray-800'
              }`}>
                {data.effects?.delay?.enabled ? 'ON' : 'OFF'}
              </span>
            </div>

            {data.effects?.delay?.enabled && (
              <div class="space-y-2 text-sm">
                <div class="flex justify-between">
                  <span class="text-gray-600 dark:text-gray-400">Time:</span>
                  <span class="font-medium text-gray-900 dark:text-white">{data.effects.delay.time || 0} ms</span>
                </div>
                <div class="flex justify-between">
                  <span class="text-gray-600 dark:text-gray-400">Feedback:</span>
                  <span class="font-medium text-gray-900 dark:text-white">{((data.effects.delay.feedback || 0) * 100).toFixed(0)}%</span>
                </div>
                <div class="flex justify-between">
                  <span class="text-gray-600 dark:text-gray-400">Mix:</span>
                  <span class="font-medium text-gray-900 dark:text-white">{((data.effects.delay.mix || 0) * 100).toFixed(0)}%</span>
                </div>
              </div>
            )}
          </div>

          {/* Chorus */}
          <div class="bg-white dark:bg-gray-800 rounded-lg shadow p-6">
            <div class="flex items-center justify-between mb-4">
              <h3 class="text-lg font-medium text-gray-900 dark:text-white">Chorus</h3>
              <span class={`px-2 py-1 rounded text-xs font-semibold ${
                data.effects?.chorus?.enabled ? 'bg-purple-100 text-purple-800' : 'bg-gray-100 text-gray-800'
              }`}>
                {data.effects?.chorus?.enabled ? 'ON' : 'OFF'}
              </span>
            </div>

            {data.effects?.chorus?.enabled && (
              <div class="space-y-2 text-sm">
                <div class="flex justify-between">
                  <span class="text-gray-600 dark:text-gray-400">Rate:</span>
                  <span class="font-medium text-gray-900 dark:text-white">{(data.effects.chorus.rate || 0).toFixed(1)} Hz</span>
                </div>
                <div class="flex justify-between">
                  <span class="text-gray-600 dark:text-gray-400">Depth:</span>
                  <span class="font-medium text-gray-900 dark:text-white">{(data.effects.chorus.depth || 0).toFixed(1)}</span>
                </div>
                <div class="flex justify-between">
                  <span class="text-gray-600 dark:text-gray-400">Mix:</span>
                  <span class="font-medium text-gray-900 dark:text-white">{((data.effects.chorus.mix || 0) * 100).toFixed(0)}%</span>
                </div>
              </div>
            )}
          </div>

          {/* Reverb */}
          <div class="bg-white dark:bg-gray-800 rounded-lg shadow p-6">
            <div class="flex items-center justify-between mb-4">
              <h3 class="text-lg font-medium text-gray-900 dark:text-white">Reverb</h3>
              <span class={`px-2 py-1 rounded text-xs font-semibold ${
                data.effects?.reverb?.enabled ? 'bg-indigo-100 text-indigo-800' : 'bg-gray-100 text-gray-800'
              }`}>
                {data.effects?.reverb?.enabled ? 'ON' : 'OFF'}
              </span>
            </div>

            {data.effects?.reverb?.enabled && (
              <div class="space-y-2 text-sm">
                <div class="flex justify-between">
                  <span class="text-gray-600 dark:text-gray-400">Room Size:</span>
                  <span class="font-medium text-gray-900 dark:text-white">{((data.effects.reverb.roomSize || 0) * 100).toFixed(0)}%</span>
                </div>
                <div class="flex justify-between">
                  <span class="text-gray-600 dark:text-gray-400">Damping:</span>
                  <span class="font-medium text-gray-900 dark:text-white">{((data.effects.reverb.damping || 0) * 100).toFixed(0)}%</span>
                </div>
                <div class="flex justify-between">
                  <span class="text-gray-600 dark:text-gray-400">Mix:</span>
                  <span class="font-medium text-gray-900 dark:text-white">{((data.effects.reverb.mix || 0) * 100).toFixed(0)}%</span>
                </div>
              </div>
            )}
          </div>
        </div>
      </section>

      {/* Performance Metrics */}
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 dark:text-white mb-4">System Performance</h2>

        <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
          <StatusCard
            title="Free RAM"
            value={formatRAM(data.performance?.ram)}
            unit=""
          />

          <StatusCard
            title="Audio Task"
            value={
              data.performance?.audioTime !== undefined && data.performance?.maxAudioTime !== undefined
                ? `${data.performance.audioTime.toFixed(2)} / ${data.performance.maxAudioTime.toFixed(2)}`
                : '0.00 / 0.00'
            }
            unit="ms"
            description="This is the time that the audio task takes, compared to the maximum available time calculated on buffer and sample values, es. 256 / 22050 * 1000 = 11.61 ms"
          />

          <StatusCard
            title="Uptime"
            value={formatUptime(data.performance?.uptime)}
            unit=""
          />

          <StatusCard
            title="Connection"
            value={data.sensor?.pitch !== undefined ? 'Active' : 'Waiting'}
            unit=""
          />
        </div>
      </section>

      {/* System Settings/Presets */}
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 dark:text-white mb-4">System Settings</h2>

        <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
          <StatusCard
            title="Pitch Smoothing"
            value={
              data.system?.pitchSmoothing === 0 ? 'None' :
              data.system?.pitchSmoothing === 2 ? 'Extra' :
              'Normal'
            }
            unit=""
          />

          <StatusCard
            title="Volume Smoothing"
            value={
              data.system?.volumeSmoothing === 0 ? 'None' :
              data.system?.volumeSmoothing === 2 ? 'Extra' :
              'Normal'
            }
            unit=""
          />

          <StatusCard
            title="Frequency Range"
            value={
              data.system?.frequencyRange === 0 ? 'Narrow' :
              data.system?.frequencyRange === 2 ? 'Wide' :
              'Normal'
            }
            unit=""
          />
        </div>
      </section>

      {/* Sensor Data (Optional) */}
      {data.sensor?.pitch !== undefined && (
        <section class="mb-8">
          <h2 class="text-xl font-semibold text-gray-800 dark:text-white mb-4">Sensor Readings</h2>
          <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
            <StatusCard
              title="Pitch Distance"
              value={data.sensor.pitch?.toFixed(1) || '0'}
              unit="mm"
            />

            <StatusCard
              title="Volume Distance"
              value={data.sensor.volume?.toFixed(1) || '0'}
              unit="mm"
            />
          </div>
        </section>
      )}

      {/* Debug info */}
      <section class="mt-8">
        <details class="bg-white dark:bg-gray-800 rounded-lg shadow p-4">
          <summary class="cursor-pointer font-medium text-gray-700 dark:text-gray-300">
            Debug: Raw WebSocket Data
          </summary>
          <pre class="mt-4 p-4 bg-gray-100 dark:bg-gray-900 rounded overflow-auto text-xs text-gray-900 dark:text-gray-100">
            {JSON.stringify(data, null, 2)}
          </pre>
        </details>
      </section>

    </div>
  );
}
