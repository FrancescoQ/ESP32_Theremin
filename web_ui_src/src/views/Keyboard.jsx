import { h } from 'preact';
import { useState, useEffect, useRef } from 'preact/hooks';
import { useWebSocket } from '../hooks/WebSocketProvider';

/**
 * Keyboard View - Virtual Piano Keyboard for playing the theremin as a synth
 *
 * Features:
 * - Configurable octave range (1.5, 2, 2.5, 3, 4 octaves)
 * - Piano-style keyboard with white and black keys
 * - Touch and mouse support
 * - Visual feedback on key press
 * - Monophonic playback (one note at a time)
 * - Starts from C4 (MIDI 60 - middle C)
 */

// Octave configuration options
const OCTAVE_CONFIGS = [
  { value: 1.5, label: '1.5 Octaves', notes: 19 },  // C to F# (1.5 octaves)
  { value: 2, label: '2 Octaves', notes: 25 },      // C to C (2 full octaves + 1)
  { value: 2.5, label: '2.5 Octaves', notes: 31 },  // C to F# (2.5 octaves)
  { value: 3, label: '3 Octaves', notes: 37 },      // C to C (3 full octaves + 1)
  { value: 4, label: '4 Octaves', notes: 49 }       // C to C (4 full octaves + 1)
];

// Note names in chromatic order (C to B)
const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

// Check if note is a black key
const isBlackKey = (noteIndex) => {
  const noteName = NOTE_NAMES[noteIndex % 12];
  return noteName.includes('#');
};

// Get note name from MIDI number
const getNoteName = (midiNote) => {
  const noteIndex = midiNote % 12;
  const octave = Math.floor(midiNote / 12) - 1;
  return `${NOTE_NAMES[noteIndex]}${octave}`;
};

// Keyboard shortcut mapping (1 octave: C4 to C5)
const KEY_MAP = {
  // White keys (naturals)
  'a': 60,  // C4
  's': 62,  // D4
  'd': 64,  // E4
  'f': 65,  // F4
  'g': 67,  // G4
  'h': 69,  // A4
  'j': 71,  // B4
  'k': 72,  // C5

  // Black keys (sharps)
  'w': 61,  // C#4
  'e': 63,  // D#4
  't': 66,  // F#4
  'y': 68,  // G#4
  'u': 70,  // A#4
};

export default function Keyboard() {
  const { send, state } = useWebSocket();
  const [octaveRange, setOctaveRange] = useState(2); // Default: 2 octaves
  const [activeNote, setActiveNote] = useState(null);
  const [pressedKeys, setPressedKeys] = useState(new Set());
  const [volume, setVolume] = useState(60); // Default: 60%
  const [keyboardShortcutsEnabled, setKeyboardShortcutsEnabled] = useState(true);
  const [originalFreqRange, setOriginalFreqRange] = useState(null);

  // Base MIDI note (C4 = 60, middle C)
  const BASE_MIDI_NOTE = 60;

  // Get configuration for current octave range
  const config = OCTAVE_CONFIGS.find(c => c.value === octaveRange) || OCTAVE_CONFIGS[1];

  // Generate array of MIDI notes for current range
  const notes = Array.from({ length: config.notes }, (_, i) => BASE_MIDI_NOTE + i);

  // Get keyboard shortcut label for a MIDI note
  const getKeyLabel = (midiNote) => {
    const keyEntry = Object.entries(KEY_MAP).find(([_, note]) => note === midiNote);
    return keyEntry ? keyEntry[0].toUpperCase() : '';
  };

  // Calculate frequency range for octave selection
  // MIDI to frequency: f = 440 * 2^((midiNote - 69) / 12)
  const calculateFrequencyRange = (octaveValue) => {
    const config = OCTAVE_CONFIGS.find(c => c.value === octaveValue) || OCTAVE_CONFIGS[1];
    const firstMidiNote = BASE_MIDI_NOTE;
    const lastMidiNote = BASE_MIDI_NOTE + config.notes - 1;

    const minFreq = Math.round(440 * Math.pow(2, (firstMidiNote - 69) / 12));
    const maxFreq = Math.round(440 * Math.pow(2, (lastMidiNote - 69) / 12));

    return { minFreq, maxFreq };
  };

  // Play note
  const playNote = (midiNote) => {
    if (activeNote === midiNote) return; // Already playing this note

    setActiveNote(midiNote);
    send({ cmd: 'playNote', note: midiNote });
    send({ cmd: 'setAmplitude', value: volume });
  };

  // Stop note
  const stopNote = () => {
    setActiveNote(null);
    send({ cmd: 'stopNote' });
    send({ cmd: 'setAmplitude', value: 0 });
  };

  // Handle mouse/touch start on key
  const handleKeyDown = (midiNote) => {
    playNote(midiNote);
  };

  // Handle mouse/touch end
  const handleKeyUp = () => {
    stopNote();
  };

  // Capture original frequency range from WebSocket state
  useEffect(() => {
    if (state?.system?.minFrequency && state?.system?.maxFrequency && !originalFreqRange) {
      setOriginalFreqRange({
        min: state.system.minFrequency,
        max: state.system.maxFrequency
      });
    }
  }, [state]);

  // Disable sensors and set frequency range when keyboard loads
  useEffect(() => {
    // Disable both sensors when entering keyboard mode
    send({ cmd: 'setSensorEnabled', sensor: 'both', enabled: false });

    // Set frequency range for current octave selection
    const { minFreq, maxFreq } = calculateFrequencyRange(octaveRange);
    send({ cmd: 'setFrequencyRange', minFreq, maxFreq });

    return () => {
      // Restore original frequency range
      if (originalFreqRange) {
        send({
          cmd: 'setFrequencyRange',
          minFreq: originalFreqRange.min,
          maxFreq: originalFreqRange.max
        });
      }

      // Re-enable sensors when leaving keyboard view
      send({ cmd: 'setSensorEnabled', sensor: 'both', enabled: true });
      stopNote();
    };
  }, [originalFreqRange]);

  // Update frequency range when octave selection changes
  useEffect(() => {
    if (originalFreqRange) {  // Only if we've captured the original range
      const { minFreq, maxFreq } = calculateFrequencyRange(octaveRange);
      send({ cmd: 'setFrequencyRange', minFreq, maxFreq });
    }
  }, [octaveRange, originalFreqRange]);

  // Keyboard shortcuts event listeners
  useEffect(() => {
    if (!keyboardShortcutsEnabled) return;

    const handleKeyDown = (e) => {
      // Ignore if typing in input field
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;

      const key = e.key.toLowerCase();
      const midiNote = KEY_MAP[key];

      if (midiNote && !pressedKeys.has(key)) {
        e.preventDefault();
        setPressedKeys(prev => new Set(prev).add(key));
        playNote(midiNote);
      }
    };

    const handleKeyUp = (e) => {
      const key = e.key.toLowerCase();
      const midiNote = KEY_MAP[key];

      if (midiNote && pressedKeys.has(key)) {
        e.preventDefault();
        setPressedKeys(prev => {
          const next = new Set(prev);
          next.delete(key);
          return next;
        });
        stopNote();
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);

    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, [keyboardShortcutsEnabled, pressedKeys, volume]);

  // Render white keys
  const renderWhiteKeys = () => {
    return notes
      .filter((note) => !isBlackKey(note - BASE_MIDI_NOTE))
      .map((midiNote) => {
        const isActive = activeNote === midiNote;
        const noteName = getNoteName(midiNote);

        return (
          <button
            key={midiNote}
            class={`white-key ${isActive ? 'active' : ''}`}
            onMouseDown={() => handleKeyDown(midiNote)}
            onMouseUp={handleKeyUp}
            onMouseLeave={handleKeyUp}
            onTouchStart={(e) => {
              e.preventDefault();
              handleKeyDown(midiNote);
            }}
            onTouchEnd={(e) => {
              e.preventDefault();
              handleKeyUp();
            }}
          >
            <span class="key-label">{noteName}</span>
            {keyboardShortcutsEnabled && getKeyLabel(midiNote) && (
              <span class="keyboard-shortcut">{getKeyLabel(midiNote)}</span>
            )}
          </button>
        );
      });
  };

  // Render black keys
  const renderBlackKeys = () => {
    return notes
      .filter((note) => isBlackKey(note - BASE_MIDI_NOTE))
      .map((midiNote) => {
        const isActive = activeNote === midiNote;
        const noteName = getNoteName(midiNote);

        // Calculate position relative to white keys
        const noteIndex = (midiNote - BASE_MIDI_NOTE) % 12;
        const whiteKeysBefore = notes
          .slice(0, midiNote - BASE_MIDI_NOTE)
          .filter((n) => !isBlackKey(n - BASE_MIDI_NOTE)).length;

        // Position black key between white keys
        const left = `calc(${whiteKeysBefore * 100 / (config.notes - Math.floor(config.notes / 12) * 5)}% + 3.5%)`;

        return (
          <button
            key={midiNote}
            class={`black-key ${isActive ? 'active' : ''}`}
            style={{ left }}
            onMouseDown={() => handleKeyDown(midiNote)}
            onMouseUp={handleKeyUp}
            onMouseLeave={handleKeyUp}
            onTouchStart={(e) => {
              e.preventDefault();
              handleKeyDown(midiNote);
            }}
            onTouchEnd={(e) => {
              e.preventDefault();
              handleKeyUp();
            }}
          >
            <span class="key-label">{noteName}</span>
            {keyboardShortcutsEnabled && getKeyLabel(midiNote) && (
              <span class="keyboard-shortcut">{getKeyLabel(midiNote)}</span>
            )}
          </button>
        );
      });
  };

  return (
    <div class="p-6 max-w-7xl mx-auto">
      {/* Header */}
      <div class="mb-6">
        <h2 class="text-2xl font-bold text-gray-800 dark:text-white mb-2">
          Keyboard
        </h2>
        <p class="text-gray-600 dark:text-gray-400">
          Play the theremin with a virtual piano keyboard
        </p>
      </div>

      {/* Controls */}
      <div class="mb-6 bg-white dark:bg-gray-800 rounded-lg p-4 shadow">
        <div class="flex flex-col gap-4">
          {/* Octave Range */}
          <div class="flex items-center gap-4">
            <label class="text-sm font-medium text-gray-700 dark:text-gray-300 whitespace-nowrap">
              Octave Range:
            </label>
            <select
              value={octaveRange}
              onChange={(e) => {
                stopNote(); // Stop any playing note when changing range
                setOctaveRange(parseFloat(e.target.value));
              }}
              class="px-3 py-2 bg-gray-100 dark:bg-gray-700 border border-gray-300 dark:border-gray-600 rounded-lg text-gray-800 dark:text-white focus:ring-2 focus:ring-blue-500"
            >
              {OCTAVE_CONFIGS.map((config) => (
                <option key={config.value} value={config.value}>
                  {config.label}
                </option>
              ))}
            </select>

            <div class="ml-auto text-sm text-gray-600 dark:text-gray-400">
              {activeNote !== null ? (
                <span class="font-medium text-blue-600 dark:text-blue-400">
                  Playing: {getNoteName(activeNote)} (MIDI {activeNote})
                </span>
              ) : (
                <span>Click or tap keys to play</span>
              )}
            </div>
          </div>

          {/* Volume Control */}
          <div class="flex items-center gap-4">
            <label class="text-sm font-medium text-gray-700 dark:text-gray-300 whitespace-nowrap">
              Volume: {volume}%
            </label>
            <input
              type="range"
              min="0"
              max="100"
              value={volume}
              onChange={(e) => setVolume(parseInt(e.target.value))}
              class="flex-1 h-2 bg-gray-200 dark:bg-gray-700 rounded-lg appearance-none cursor-pointer"
              style={{
                background: `linear-gradient(to right, #3b82f6 0%, #3b82f6 ${volume}%, #e5e7eb ${volume}%, #e5e7eb 100%)`
              }}
            />
          </div>

          {/* Keyboard Shortcuts Toggle */}
          <div class="flex items-center gap-4">
            <label class="text-sm font-medium text-gray-700 dark:text-gray-300 whitespace-nowrap">
              Keyboard Shortcuts
            </label>
            <label class="relative inline-flex items-center cursor-pointer">
              <input
                type="checkbox"
                checked={keyboardShortcutsEnabled}
                onChange={(e) => setKeyboardShortcutsEnabled(e.target.checked)}
                class="sr-only peer"
              />
              <div class="w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 dark:peer-focus:ring-blue-800 rounded-full peer dark:bg-gray-700 peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all dark:border-gray-600 peer-checked:bg-blue-600"></div>
            </label>
            <span class="text-xs text-gray-500 dark:text-gray-400">
              {keyboardShortcutsEnabled ? 'Enabled' : 'Disabled'}
            </span>
          </div>
        </div>
      </div>

      {/* Keyboard */}
      <div class="bg-white dark:bg-gray-800 rounded-lg p-6 shadow-lg">
        <div class="keyboard-container">
          {/* White keys */}
          <div class="white-keys">
            {renderWhiteKeys()}
          </div>

          {/* Black keys (positioned absolutely on top) */}
          <div class="black-keys">
            {renderBlackKeys()}
          </div>
        </div>
      </div>

      {/* Keyboard Layout Guide */}
      {keyboardShortcutsEnabled && (
        <div class="mt-4 bg-blue-50 dark:bg-blue-900/20 rounded-lg p-4 border border-blue-200 dark:border-blue-800">
          <h3 class="text-sm font-semibold text-blue-900 dark:text-blue-100 mb-2">
            ⌨️ Keyboard Shortcuts (1 Octave)
          </h3>
          <div class="grid grid-cols-2 gap-4 text-xs text-blue-800 dark:text-blue-200">
            <div>
              <p class="font-medium mb-1">White Keys (Naturals):</p>
              <div class="font-mono space-y-0.5">
                <div>A = C4 | S = D4 | D = E4 | F = F4</div>
                <div>G = G4 | H = A4 | J = B4 | K = C5</div>
              </div>
            </div>
            <div>
              <p class="font-medium mb-1">Black Keys (Sharps):</p>
              <div class="font-mono space-y-0.5">
                <div>W = C#4 | E = D#4</div>
                <div>T = F#4 | Y = G#4 | U = A#4</div>
              </div>
            </div>
          </div>
        </div>
      )}

      {/* Info */}
      <div class="mt-4 text-sm text-gray-600 dark:text-gray-400">
        <p>💡 <strong>Tip:</strong> Use this keyboard to test effects, try different waveforms, and experiment with the synthesizer. Sensors are automatically disabled while using the keyboard.</p>
      </div>

      <style jsx>{`
        .keyboard-container {
          position: relative;
          height: 200px;
          overflow-x: auto;
          overflow-y: hidden;
        }

        .white-keys {
          display: flex;
          gap: 2px;
          height: 100%;
        }

        .white-key {
          flex: 1;
          min-width: 40px;
          height: 100%;
          background: linear-gradient(to bottom, #ffffff 0%, #f0f0f0 100%);
          border: 2px solid #333;
          border-radius: 0 0 8px 8px;
          cursor: pointer;
          position: relative;
          transition: all 0.05s ease;
          display: flex;
          align-items: flex-end;
          justify-content: center;
          padding-bottom: 8px;
          user-select: none;
          -webkit-tap-highlight-color: transparent;
        }

        .white-key:hover {
          background: linear-gradient(to bottom, #f0f0f0 0%, #e0e0e0 100%);
          transform: translateY(2px);
        }

        .white-key.active {
          background: linear-gradient(to bottom, #4ade80 0%, #22c55e 100%);
          transform: translateY(4px);
          box-shadow: inset 0 2px 8px rgba(0, 0, 0, 0.3);
        }

        .black-keys {
          position: absolute;
          top: 0;
          left: 0;
          right: 0;
          height: 60%;
          pointer-events: none;
        }

        .black-key {
          position: absolute;
          width: 7%;
          height: 100%;
          background: linear-gradient(to bottom, #000 0%, #333 100%);
          border: 2px solid #000;
          border-radius: 0 0 6px 6px;
          cursor: pointer;
          pointer-events: auto;
          transition: all 0.05s ease;
          display: flex;
          align-items: flex-end;
          justify-content: center;
          padding-bottom: 6px;
          user-select: none;
          -webkit-tap-highlight-color: transparent;
          z-index: 10;
        }

        .black-key:hover {
          background: linear-gradient(to bottom, #333 0%, #555 100%);
          transform: translateY(2px);
        }

        .black-key.active {
          background: linear-gradient(to bottom, #22c55e 0%, #16a34a 100%);
          transform: translateY(3px);
          box-shadow: inset 0 2px 6px rgba(0, 0, 0, 0.5);
        }

        .key-label {
          font-size: 0.75rem;
          font-weight: 600;
          color: #666;
          pointer-events: none;
        }

        .keyboard-shortcut {
          position: absolute;
          top: 8px;
          right: 8px;
          font-size: 0.7rem;
          font-weight: 700;
          color: #3b82f6;
          background: rgba(255, 255, 255, 0.9);
          padding: 2px 6px;
          border-radius: 4px;
          pointer-events: none;
          box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
        }

        .black-key .key-label {
          color: #fff;
        }

        .black-key .keyboard-shortcut {
          background: rgba(59, 130, 246, 0.9);
          color: white;
        }

        .white-key.active .key-label,
        .black-key.active .key-label {
          color: white;
        }

        .white-key.active .keyboard-shortcut,
        .black-key.active .keyboard-shortcut {
          background: rgba(255, 255, 255, 0.95);
          color: #16a34a;
        }

        /* Responsive adjustments */
        @media (max-width: 768px) {
          .keyboard-container {
            height: 150px;
          }

          .white-key {
            min-width: 30px;
          }

          .key-label {
            font-size: 0.65rem;
          }

          .keyboard-shortcut {
            font-size: 0.6rem;
            padding: 1px 4px;
            top: 4px;
            right: 4px;
          }
        }
      `}</style>
    </div>
  );
}
