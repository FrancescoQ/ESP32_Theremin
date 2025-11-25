import { h } from 'preact';
import { render } from 'preact';
import { WebSocketProvider, useWebSocket } from './hooks/WebSocketProvider';
import { useState } from 'preact/hooks';
import { Dashboard } from './views/Dashboard';
import { Oscillators } from './views/Oscillators';
import { Header } from './components/Header';
import './styles.css';

/**
 * Main App
 */
function App() {
  const [view, setView] = useState('oscillators');
  return (
    <WebSocketProvider>
      <div class="min-h-screen bg-gray-100">
        <Header setView={setView} />
        {view === 'dashboard' && <Dashboard />}
        {view === 'oscillators' && <Oscillators />}
        {view === 'effects' && <Effects />}
        {view === 'settings' && <Settings />}
      </div>
    </WebSocketProvider>
  );
}

// Render!
render(h(App), document.getElementById('app'));
