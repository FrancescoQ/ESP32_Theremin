import { h, createContext } from 'preact';
import { useState, useEffect, useContext, useCallback } from 'preact/hooks';

// Context to share WebSocket across the app
const WebSocketContext = createContext(null);

export function WebSocketProvider({ children, url }) {
  const [ws, setWs] = useState(null);
  const [connected, setConnected] = useState(false);
  const [data, setData] = useState({
    oscillators: {},
    effects: {},
    sensor: {},
    performance: {},
    system: {},
    tuner: {}
  });
  const [error, setError] = useState(null);

  useEffect(() => {
    let websocket;
    let reconnectTimer;

    const connect = () => {
      try {
        // Determine WebSocket URL
        const wsUrl = url || `ws://${window.location.hostname}/ws`;
        websocket = new WebSocket(wsUrl);

        websocket.onopen = () => {
          console.log('WebSocket connected');
          setConnected(true);
          setError(null);
        };

        websocket.onmessage = (event) => {
          try {
            const parsed = JSON.parse(event.data);

            // Handle different message types
            if (parsed.type === 'complete') {
              // New batched message format - update all state at once
              setData({
                oscillators: parsed.oscillators || {},
                effects: parsed.effects || {},
                sensor: parsed.sensor || {},
                performance: parsed.performance || {},
                system: parsed.system || {},
                tuner: parsed.tuner || {}
              });
            } else if (parsed.type === 'oscillator') {
              // Individual oscillator update (backward compatibility)
              setData(prev => ({
                ...prev,
                oscillators: {
                  ...prev.oscillators,
                  [parsed.osc]: parsed
                }
              }));
            } else if (parsed.type === 'effect') {
              // Individual effect update (backward compatibility)
              setData(prev => ({
                ...prev,
                effects: {
                  ...prev.effects,
                  [parsed.effect]: parsed
                }
              }));
            } else if (parsed.type === 'sensor') {
              setData(prev => ({
                ...prev,
                sensor: parsed
              }));
            } else if (parsed.type === 'performance') {
              setData(prev => ({
                ...prev,
                performance: parsed
              }));
            } else if (parsed.type === 'system') {
              setData(prev => ({
                ...prev,
                system: parsed
              }));
            }
          } catch (e) {
            console.error('JSON parsing error:', e);
          }
        };

        websocket.onerror = (err) => {
          console.error('WebSocket error:', err);
          setError('WebSocket connection error');
        };

        websocket.onclose = () => {
          console.log('WebSocket disconnected');
          setConnected(false);
          setWs(null);

          // Auto-reconnect after 3 seconds
          reconnectTimer = setTimeout(() => {
            console.log('Attempting reconnection...');
            connect();
          }, 3000);
        };

        setWs(websocket);
      } catch (err) {
        console.error('Error creating WebSocket:', err);
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

  // Helper to send data
  const send = useCallback((payload) => {
    if (ws && connected) {
      ws.send(typeof payload === 'string' ? payload : JSON.stringify(payload));
    } else {
      console.warn('WebSocket not connected');
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

// Custom hook to use WebSocket in components
export function useWebSocket() {
  const context = useContext(WebSocketContext);
  if (!context) {
    throw new Error('useWebSocket must be used inside WebSocketProvider');
  }
  return context;
}
