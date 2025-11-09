// WebSocket connection
let ws;
let reconnectInterval;

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
  connectWebSocket();
  setupEventListeners();
});

// Connect to WebSocket
function connectWebSocket() {
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const wsUrl = `${protocol}//${window.location.host}/ws`;

  console.log('Connecting to:', wsUrl);
  ws = new WebSocket(wsUrl);

  ws.onopen = () => {
    console.log('✅ Connected to Theremin WebSocket');
    updateConnectionStatus(true);
    clearInterval(reconnectInterval);
  };

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      handleUpdate(data);
    } catch (e) {
      console.error('Failed to parse message:', e);
    }
  };

  ws.onclose = () => {
    console.log('❌ Disconnected from Theremin');
    updateConnectionStatus(false);

    // Auto-reconnect every 2 seconds
    reconnectInterval = setInterval(() => {
      console.log('🔄 Attempting to reconnect...');
      connectWebSocket();
    }, 2000);
  };

  ws.onerror = (error) => {
    console.error('WebSocket error:', error);
  };
}

// Handle incoming WebSocket messages
function handleUpdate(data) {
  //console.log('Received:', data.type, data);

  switch (data.type) {
    case 'oscillator':
      if (data.osc === 1) {
        updateOscillator1UI(data);
      }
      break;

    case 'sensor':
      updateSensorUI(data);
      break;

    case 'performance':
      updatePerformanceUI(data);
      break;
  }
}

// Update oscillator 1 UI
function updateOscillator1UI(data) {
  document.getElementById('osc1-waveform').textContent = data.waveform;
  document.getElementById('osc1-octave').textContent = data.octave;
  document.getElementById('osc1-volume').textContent = (data.volume * 100).toFixed(0) + '%';

  // Update checkbox state (enabled if waveform is not OFF)
  const checkbox = document.getElementById('osc1-enable');
  const isEnabled = data.waveform !== 'OFF';

  // Only update if different to avoid triggering change event
  if (checkbox.checked !== isEnabled) {
    checkbox.checked = isEnabled;
  }
}

// Update sensor UI
function updateSensorUI(data) {
  document.getElementById('pitch-value').textContent = data.pitch + ' mm';
  document.getElementById('volume-value').textContent = data.volume + ' mm';
}

// Update performance UI
function updatePerformanceUI(data) {
  const ramKB = (data.ram / 1024).toFixed(0);
  document.getElementById('ram-value').textContent = ramKB + ' KB';

  // Format uptime
  const seconds = Math.floor(data.uptime / 1000);
  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;
  document.getElementById('uptime-value').textContent =
    `${hours}h ${minutes}m ${secs}s`;
}

// Update connection status indicator
function updateConnectionStatus(connected) {
  const statusEl = document.getElementById('connection-status');
  statusEl.textContent = connected ? '✅ Connected' : '❌ Disconnected';
  statusEl.className = 'status ' + (connected ? 'connected' : 'disconnected');
}

// Send command to ESP32
function sendCommand(cmd, params) {
  if (ws.readyState === WebSocket.OPEN) {
    const message = JSON.stringify({ cmd, ...params });
    //console.log('Sending:', message);
    ws.send(message);
  } else {
    console.error('WebSocket not connected - cannot send command');
  }
}

// Setup event listeners
function setupEventListeners() {
  const checkbox = document.getElementById('osc1-enable');

  checkbox.addEventListener('change', (e) => {
    const enabled = e.target.checked;
    const waveform = enabled ? 'SINE' : 'OFF';

    console.log('Oscillator 1:', enabled ? 'Enabling' : 'Disabling');
    sendCommand('setWaveform', { osc: 1, value: waveform });
  });
}
