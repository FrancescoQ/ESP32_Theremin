import { h } from 'preact';
import { render } from 'preact';
import { WebSocketProvider, useWebSocket } from './hooks/WebSocketProvider';
import { useState, useEffect } from 'preact/hooks';
import { Dashboard } from './views/Dashboard';
import { Oscillators } from './views/Oscillators';
import { Effects } from './views/Effects';
import { Sensors } from './views/Sensors';
import { Header } from './components/Header';
import './styles.css';

/**
 * Views configuration - Single source of truth for all views
 */
const VIEWS = [
  { id: 'dashboard', label: 'Dashboard', component: Dashboard, default: true },
  { id: 'oscillators', label: 'Oscillators', component: Oscillators },
  { id: 'effects', label: 'Effects', component: Effects },
  { id: 'sensors', label: 'Sensors', component: Sensors }
];

/**
 * Default view - derived from VIEWS configuration
 */
const DEFAULT_VIEW = VIEWS.find(v => v.default)?.id || 'dashboard';

/**
 * Helper to resolve WebSocket URL for development
 * Priority: URL parameter > Environment variable > Default (window.location.hostname)
 *
 * Usage examples:
 * 1. URL parameter: http://localhost:5173/?ws=192.168.1.100
 * 2. Environment variable: Create .env.local with VITE_WS_URL=ws://192.168.1.100/ws
 * 3. Default: Uses window.location.hostname (for production on ESP32)
 */
function getWebSocketUrl() {
  // Priority 1: URL parameter (?ws=192.168.1.100)
  const params = new URLSearchParams(window.location.search);
  const urlParam = params.get('ws');
  if (urlParam) {
    const wsUrl = `ws://${urlParam}/ws`;
    console.log('[WebSocket] Using URL parameter:', wsUrl);
    return { url: wsUrl, isOverridden: true, source: urlParam };
  }

  // Priority 2: Environment variable (VITE_WS_URL in .env.local)
  if (import.meta.env.VITE_WS_URL) {
    console.log('[WebSocket] Using environment variable:', import.meta.env.VITE_WS_URL);
    return { url: import.meta.env.VITE_WS_URL, isOverridden: true, source: import.meta.env.VITE_WS_URL };
  }

  // Priority 3: Default (undefined - will use window.location.hostname in WebSocketProvider)
  console.log('[WebSocket] Using default (window.location.hostname)');
  return { url: undefined, isOverridden: false, source: null };
}

/**
 * Main App
 */
function App() {
  const [view, setView] = useState(DEFAULT_VIEW);

  // Dark mode state with localStorage persistence (default: dark mode)
  const [darkMode, setDarkMode] = useState(() => {
    const saved = localStorage.getItem('darkMode');
    return saved ? JSON.parse(saved) : true;
  });

  // Apply dark mode class to document root
  useEffect(() => {
    if (darkMode) {
      document.documentElement.classList.add('dark');
    } else {
      document.documentElement.classList.remove('dark');
    }
    localStorage.setItem('darkMode', JSON.stringify(darkMode));
  }, [darkMode]);

  // Find current view component dynamically
  const CurrentView = VIEWS.find(v => v.id === view)?.component || Dashboard;

  // Resolve WebSocket URL (supports development override)
  const wsConfig = getWebSocketUrl();

  return (
    <WebSocketProvider url={wsConfig.url}>
      <div class="min-h-screen bg-gray-100 dark:bg-gray-900">
        {/* Development mode warning banner */}
        {wsConfig.isOverridden && (
          <div class="bg-yellow-500 text-black px-4 py-2 text-center text-sm font-medium">
            ⚠️ Development Mode: Connected to {wsConfig.source}
          </div>
        )}

        <Header
          setView={setView}
          views={VIEWS.map(v => ({ id: v.id, label: v.label }))}
          defaultView={DEFAULT_VIEW}
          currentView={view}
          darkMode={darkMode}
          setDarkMode={setDarkMode}
        />
        <CurrentView />
      </div>
    </WebSocketProvider>
  );
}

// Render!
render(h(App), document.getElementById('app'));
