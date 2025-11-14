import { h, createContext } from 'preact';
import { useState, useEffect, useContext, useCallback } from 'preact/hooks';

// Context per condividere WebSocket in tutta l'app
const WebSocketContext = createContext(null);

export function WebSocketProvider({ children, url }) {
  const [ws, setWs] = useState(null);
  const [connected, setConnected] = useState(false);
  const [data, setData] = useState({});
  const [error, setError] = useState(null);

  useEffect(() => {
    let websocket;
    let reconnectTimer;

    const connect = () => {
      try {
        // Determina URL WebSocket
        const wsUrl = url || `ws://${window.location.hostname}/ws`;
        websocket = new WebSocket(wsUrl);

        websocket.onopen = () => {
          console.log('WebSocket connesso');
          setConnected(true);
          setError(null);
        };

        websocket.onmessage = (event) => {
          try {
            const parsed = JSON.parse(event.data);
            setData(parsed);
          } catch (e) {
            console.error('Errore parsing JSON:', e);
          }
        };

        websocket.onerror = (err) => {
          console.error('WebSocket errore:', err);
          setError('Errore connessione WebSocket');
        };

        websocket.onclose = () => {
          console.log('WebSocket disconnesso');
          setConnected(false);
          setWs(null);
          
          // Auto-reconnect dopo 3 secondi
          reconnectTimer = setTimeout(() => {
            console.log('Tentativo riconnessione...');
            connect();
          }, 3000);
        };

        setWs(websocket);
      } catch (err) {
        console.error('Errore creazione WebSocket:', err);
        setError(err.message);
      }
    };

    connect();

    // Cleanup
    return () => {
      if (reconnectTimer) clearTimeout(reconnectTimer);
      if (websocket) {
        websocket.close();
      }
    };
  }, [url]);

  // Helper per inviare dati
  const send = useCallback((payload) => {
    if (ws && connected) {
      ws.send(typeof payload === 'string' ? payload : JSON.stringify(payload));
    } else {
      console.warn('WebSocket non connesso');
    }
  }, [ws, connected]);

  const value = {
    connected,
    data,
    error,
    send
  };

  return h(WebSocketContext.Provider, { value }, children);
}

// Custom hook per usare WebSocket nei componenti
export function useWebSocket() {
  const context = useContext(WebSocketContext);
  if (!context) {
    throw new Error('useWebSocket deve essere usato dentro WebSocketProvider');
  }
  return context;
}
