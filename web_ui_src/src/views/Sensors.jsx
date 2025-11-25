import { useState } from 'preact/hooks';
import { useWebSocket } from '../hooks/WebSocketProvider';
import { StatusCard } from '../components/StatusCard';
import { CommandSelect } from '../components/CommandSelect';

/**
 * Sensors View - Monitor sensor readings and configure smoothing/range presets
 */
export function Sensors() {
  const { data } = useWebSocket();

  // Track current preset selections (default to Normal = 1)
  const [pitchSmoothing, setPitchSmoothing] = useState('1');
  const [volumeSmoothing, setVolumeSmoothing] = useState('1');
  const [frequencyRange, setFrequencyRange] = useState('1');

  // Smoothing preset options
  const smoothingOptions = [
    { value: '0', label: 'None (Instant)' },
    { value: '1', label: 'Normal (Balanced)' },
    { value: '2', label: 'Extra (Smooth)' }
  ];

  // Frequency range preset options
  const rangeOptions = [
    { value: '0', label: 'Narrow (1 Octave)' },
    { value: '1', label: 'Normal (2 Octaves)' },
    { value: '2', label: 'Wide (3 Octaves)' }
  ];

  return (
    <div class="max-w-7xl mx-auto px-4 py-8 sm:px-6 lg:px-8">

      {/* Smoothing Configuration */}
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 mb-4">Smoothing Configuration</h2>

        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
          {/* Pitch Smoothing */}
          <div class="bg-white rounded-lg shadow p-6">
            <h3 class="text-lg font-medium text-gray-900 mb-4">Pitch Smoothing</h3>
            <p class="text-sm text-gray-600 mb-4">
              Controls how smoothly the pitch responds to hand movement.
            </p>

            <CommandSelect
              label="Smoothing Level"
              options={smoothingOptions.map(opt => opt.label)}
              value={smoothingOptions.find(opt => opt.value === pitchSmoothing)?.label || 'Normal (Balanced)'}
              onChange={(label) => {
                const option = smoothingOptions.find(opt => opt.label === label);
                if (option) setPitchSmoothing(option.value);
              }}
              commandGenerator={(label) => {
                const option = smoothingOptions.find(opt => opt.label === label);
                return {
                  cmd: 'setSmoothing',
                  target: 'pitch',
                  value: parseInt(option?.value || '1', 10)
                };
              }}
            />
          </div>

          {/* Volume Smoothing */}
          <div class="bg-white rounded-lg shadow p-6">
            <h3 class="text-lg font-medium text-gray-900 mb-4">Volume Smoothing</h3>
            <p class="text-sm text-gray-600 mb-4">
              Controls how smoothly the volume responds to hand movement.
            </p>

            <CommandSelect
              label="Smoothing Level"
              options={smoothingOptions.map(opt => opt.label)}
              value={smoothingOptions.find(opt => opt.value === volumeSmoothing)?.label || 'Normal (Balanced)'}
              onChange={(label) => {
                const option = smoothingOptions.find(opt => opt.label === label);
                if (option) setVolumeSmoothing(option.value);
              }}
              commandGenerator={(label) => {
                const option = smoothingOptions.find(opt => opt.label === label);
                return {
                  cmd: 'setSmoothing',
                  target: 'volume',
                  value: parseInt(option?.value || '1', 10)
                };
              }}
            />
          </div>
        </div>
      </section>

      {/* Frequency Range Configuration */}
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 mb-4">Frequency Range</h2>

        <div class="bg-white rounded-lg shadow p-6">
          <h3 class="text-lg font-medium text-gray-900 mb-4">Playing Range</h3>
          <p class="text-sm text-gray-600 mb-4">
            Sets the frequency range (musical pitch range) of the theremin.
          </p>

          <CommandSelect
            label="Range Preset"
            options={rangeOptions.map(opt => opt.label)}
            value={rangeOptions.find(opt => opt.value === frequencyRange)?.label || 'Normal (2 Octaves)'}
            onChange={(label) => {
              const option = rangeOptions.find(opt => opt.label === label);
              if (option) setFrequencyRange(option.value);
            }}
            commandGenerator={(label) => {
              const option = rangeOptions.find(opt => opt.label === label);
              return {
                cmd: 'setRange',
                value: parseInt(option?.value || '1', 10)
              };
            }}
          />

          <div class="mt-4 p-4 bg-blue-50 rounded-md">
            <p class="text-sm text-blue-800">
              <strong>Narrow:</strong> 1 octave range - tight playing area for precise control<br/>
              <strong>Normal:</strong> 2 octaves range - balanced (default)<br/>
              <strong>Wide:</strong> 3 octaves range - extended range for expressive playing
            </p>
          </div>
        </div>
      </section>

      {/* Sensor Readings */}
      <section class="mb-8">
        <h2 class="text-xl font-semibold text-gray-800 mb-4">Sensor Readings</h2>
        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
          <StatusCard
            title="Pitch Distance"
            value={data.sensor?.pitch?.toFixed(1) || '0'}
            unit="cm"
            color="blue"
          />

          <StatusCard
            title="Volume Distance"
            value={data.sensor?.volume?.toFixed(1) || '0'}
            unit="cm"
            color="purple"
          />
        </div>
      </section>

    </div>
  );
}
