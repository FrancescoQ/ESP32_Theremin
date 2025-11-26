import { useWebSocket } from '../hooks/WebSocketProvider';

export default function Tuner() {
  const { data } = useWebSocket();
  const tuner = data.tuner;

  // Check if we have valid tuner data
  const hasData = tuner && tuner.frequency && tuner.frequency >= 20;

  return (
    <div className="space-y-6">
      <h2 className="text-2xl font-bold">Tuner</h2>

      {!hasData ? (
        <div className="bg-gray-800 rounded-lg p-8 text-center">
          <p className="text-gray-400 text-lg">No signal detected</p>
          <p className="text-gray-500 text-sm mt-2">Play a note to see tuning information</p>
        </div>
      ) : (
        <>
          {/* Note Display */}
          <div className="bg-gray-800 rounded-lg p-8 text-center">
            <div className="text-6xl font-bold mb-2">{tuner.note}</div>
            <div className="text-xl text-gray-400">
              {tuner.frequency.toFixed(1)} Hz
            </div>
          </div>

          {/* Tuning Bar */}
          <div className="bg-gray-800 rounded-lg p-6">
            <div className="mb-4 text-center">
              <span className="text-lg font-semibold">
                {tuner.cents > 0 ? '+' : ''}{tuner.cents} cents
              </span>
              <span className={`ml-3 px-3 py-1 rounded text-sm font-medium ${
                tuner.inTune
                  ? 'bg-green-500/20 text-green-400'
                  : Math.abs(tuner.cents) > 25
                  ? 'bg-red-500/20 text-red-400'
                  : 'bg-yellow-500/20 text-yellow-400'
              }`}>
                {tuner.inTune ? 'IN TUNE' : tuner.cents < 0 ? 'FLAT' : 'SHARP'}
              </span>
            </div>

            {/* Horizontal Tuning Bar */}
            <div className="relative h-12 bg-gray-700 rounded-lg overflow-hidden">
              {/* Center line */}
              <div className="absolute left-1/2 top-0 bottom-0 w-0.5 bg-gray-500 z-10" />

              {/* In-tune zone (±10 cents) */}
              <div
                className="absolute top-0 bottom-0 bg-green-500/20"
                style={{
                  left: '45%',
                  width: '10%'
                }}
              />

              {/* Tuning indicator */}
              <div
                className={`absolute top-0 bottom-0 w-1 transition-all duration-100 ${
                  tuner.inTune ? 'bg-green-400' : 'bg-blue-400'
                }`}
                style={{
                  left: `${50 + (tuner.cents * 0.4)}%`,
                  transform: 'translateX(-50%)'
                }}
              >
                <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2">
                  <div className="w-0 h-0 border-l-[6px] border-l-transparent border-r-[6px] border-r-transparent border-t-[8px] border-t-current" />
                </div>
              </div>

              {/* Scale markers */}
              <div className="absolute inset-x-0 bottom-1 flex justify-between px-2 text-xs text-gray-500">
                <span>-50</span>
                <span>-25</span>
                <span className="text-gray-400">0</span>
                <span>+25</span>
                <span>+50</span>
              </div>
            </div>
          </div>

          {/* Additional Info */}
          <div className="bg-gray-800 rounded-lg p-4">
            <div className="grid grid-cols-2 gap-4 text-sm">
              <div>
                <span className="text-gray-400">Note Name:</span>
                <span className="ml-2 font-medium">{tuner.noteName}</span>
              </div>
              <div>
                <span className="text-gray-400">Octave:</span>
                <span className="ml-2 font-medium">{tuner.octave}</span>
              </div>
            </div>
          </div>
        </>
      )}

      {/* Info Section */}
      <div className="bg-gray-800/50 rounded-lg p-4 text-sm text-gray-400">
        <p className="mb-2">
          <strong className="text-gray-300">About the Tuner:</strong>
        </p>
        <ul className="list-disc list-inside space-y-1 ml-2">
          <li>The tuner calculates note information directly from the theremin's frequency</li>
          <li>No microphone needed - data comes directly from the ESP32</li>
          <li>"In tune" means within ±10 cents of perfect pitch</li>
          <li>100 cents = 1 semitone (musical half-step)</li>
        </ul>
      </div>
    </div>
  );
}
