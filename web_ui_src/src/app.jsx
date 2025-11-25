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
 * Main App
 */
function App() {
  const [view, setView] = useState(DEFAULT_VIEW);

  // Dark mode state with localStorage persistence
  const [darkMode, setDarkMode] = useState(() => {
    const saved = localStorage.getItem('darkMode');
    return saved ? JSON.parse(saved) : false;
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

  return (
    <WebSocketProvider>
      <div class="min-h-screen bg-gray-100 dark:bg-gray-900">
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
