# ESP32 Preact UI

UI web moderna e leggera per ESP32 con Preact + Tailwind CSS.

## 🚀 Quick Start

### 1. Installa dipendenze
```bash
npm install
```

### 2. Development
```bash
npm run dev
```
Apri http://localhost:3000

### 3. Build per produzione
```bash
npm run build
```
I file ottimizzati saranno in `dist/`

## 📁 Struttura

```
esp32-preact-ui/
├── src/
│   ├── components/
│   │   ├── WebSocketProvider.js  # Context WebSocket globale
│   │   ├── StatusCard.js         # Card per visualizzare metriche
│   │   ├── ControlButton.js      # Bottone per comandi
│   │   └── ToggleSwitch.js       # Switch on/off
│   ├── app.js                    # Entry point
│   └── styles.css                # Tailwind imports
├── dist/                         # Build output (da caricare su ESP32)
└── vite.config.js                # Config build
```

## 🔌 WebSocket

L'app si connette automaticamente a `ws://[hostname]/ws`.

### Formato dati atteso (esempio):
```json
{
  "temperature": 25.5,
  "humidity": 60,
  "cpu_load": 45,
  "uptime": 12345,
  "led1": true,
  "led2": false
}
```

### Inviare comandi:
```javascript
// Da un componente:
import { useWebSocket } from './components/WebSocketProvider';

function MyComponent() {
  const { send } = useWebSocket();
  
  send({ command: "LED_ON" });  // Oggetto JSON
  send("RESTART");               // Stringa semplice
}
```

## 🎨 Componenti disponibili

### StatusCard
Visualizza una metrica singola:
```jsx
<StatusCard
  title="Temperatura"
  value={25.5}
  unit="°C"
  icon="🌡️"
  color="blue"
/>
```

### ToggleSwitch
Switch on/off che si sincronizza con WebSocket:
```jsx
<ToggleSwitch
  label="LED 1"
  dataKey="led1"           // Chiave nei dati WebSocket
  onCommand="LED1_ON"      // Comando da inviare quando acceso
  offCommand="LED1_OFF"    // Comando da inviare quando spento
/>
```

### ControlButton
Bottone che invia un comando:
```jsx
<ControlButton
  label="Restart"
  command="RESTART"        // Stringa comando
  variant="warning"        // primary|success|danger|warning
/>
```

## 📦 Deploy su ESP32

### Metodo 1: SPIFFS/LittleFS
1. `npm run build`
2. Carica il contenuto di `dist/` su SPIFFS
3. Servi con AsyncWebServer:
```cpp
server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
```

### Metodo 2: Embedded in codice
1. `npm run build`
2. Converti `dist/index.html` in string C++
3. Usa `server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ ... })`

### Tips per ottimizzare:
- Il build è già minificato e ottimizzato
- Abilita GZIP su ESP32 (`response->addHeader("Content-Encoding", "gzip")`)
- Bundle finale: ~30KB (gzipped ~10KB!)

## 🛠️ Personalizzazione

### Cambia endpoint WebSocket
In `vite.config.js` modifica il proxy:
```javascript
proxy: {
  '/ws': {
    target: 'ws://192.168.1.100',  // <-- Tuo IP ESP32
    ws: true
  }
}
```

### Aggiungi nuovi componenti
Crea file in `src/components/` e importa in `app.js`:
```javascript
import { MyComponent } from './components/MyComponent';
```

### Modifica tema Tailwind
Edita `tailwind.config.js`:
```javascript
theme: {
  extend: {
    colors: {
      'esp-blue': '#00A8FF',
    }
  }
}
```

## 🐛 Troubleshooting

### WebSocket non si connette in dev
- Verifica IP ESP32 in `vite.config.js`
- Check che ESP32 sia raggiungibile
- Usa console browser (F12) per vedere errori

### Build troppo grande
- Rimuovi componenti inutilizzati
- In `vite.config.js` aggiungi `drop_console: true`
- Usa solo utility Tailwind necessarie

### Stile non carica
- Verifica che `styles.css` sia importato in `app.js`
- Rigenera Tailwind: `npm run build`

## 📚 Risorse

- [Preact Docs](https://preactjs.com)
- [Tailwind CSS](https://tailwindcss.com)
- [Vite](https://vitejs.dev)

---

**Buon divertimento con i "disegnetti"! 😄**
