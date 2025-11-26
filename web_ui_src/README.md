# ESP32 Preact UI

Modern and lightweight web UI for ESP32 with Preact + Tailwind CSS.

## 🚀 Quick Start

### 1. Install dependencies
```bash
npm install
```

### 2. Development
```bash
npm run dev
```
Open http://localhost:5173

### 3. Build for production
```bash
npm run build
```
Optimized files will be in `dist/`

## 📁 Structure

```
esp32-preact-ui/
├── src/
│   ├── components/
│   │   ├── WebSocketProvider.js  # Global WebSocket context
│   │   ├── StatusCard.js         # Card for displaying metrics
│   │   ├── ControlButton.js      # Button for commands
│   │   └── ToggleSwitch.js       # On/off switch
│   ├── app.js                    # Entry point
│   └── styles.css                # Tailwind imports
├── dist/                         # Build output (to upload to ESP32)
└── vite.config.js                # Build configuration
```

## 🔌 WebSocket

The app automatically connects to `ws://[hostname]/ws`.

### Local Development (Development Override)

When developing the UI locally with `npm run dev`, you can connect to your ESP32's WebSocket without having to reload files on the device. There are two methods:

**Method 1: URL Parameter** (quick, temporary)
```bash
# Start the dev server
npm run dev

# Open in browser with ?ws= parameter
http://localhost:5173/?ws=192.168.1.100
```

**Method 2: .env.local File** (persistent, recommended)
```bash
# Copy the example file
cp .env.local.example .env.local

# Edit .env.local and set your ESP32's IP
VITE_WS_URL=ws://192.168.1.100/ws

# Start the dev server
npm run dev

# Open normally
http://localhost:5173
```

The `.env.local` file is already in `.gitignore` and won't be committed.

**Priority order:**
1. URL parameter `?ws=192.168.1.100`
2. Environment variable `VITE_WS_URL`
3. Default: `window.location.hostname` (for production on ESP32)

### Expected data format (example):
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

### Sending commands:
```javascript
// From a component:
import { useWebSocket } from './components/WebSocketProvider';

function MyComponent() {
  const { send } = useWebSocket();

  send({ command: "LED_ON" });  // JSON object
  send("RESTART");               // Simple string
}
```

## 🎨 Available components

### StatusCard
Display a single metric:
```jsx
<StatusCard
  title="Temperature"
  value={25.5}
  unit="°C"
  icon="🌡️"
  color="blue"
/>
```

### ToggleSwitch
On/off switch that syncs with WebSocket:
```jsx
<ToggleSwitch
  label="LED 1"
  dataKey="led1"           // Key in WebSocket data
  onCommand="LED1_ON"      // Command to send when on
  offCommand="LED1_OFF"    // Command to send when off
/>
```

### ControlButton
Button that sends a command:
```jsx
<ControlButton
  label="Restart"
  command="RESTART"        // Command string
  variant="warning"        // primary|success|danger|warning
/>
```

## 📦 Deploy to ESP32

### Method 1: SPIFFS/LittleFS
1. `npm run build`
2. Upload `dist/` contents to SPIFFS
3. Serve with AsyncWebServer:
```cpp
server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
```

### Method 2: Embedded in code
1. `npm run build`
2. Convert `dist/index.html` to C++ string
3. Use `server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ ... })`

### Optimization tips:
- Build is already minified and optimized
- Enable GZIP on ESP32 (`response->addHeader("Content-Encoding", "gzip")`)
- Final bundle: ~30KB (gzipped ~10KB!)

## 🛠️ Customization

### Change WebSocket endpoint
In `vite.config.js` modify the proxy:
```javascript
proxy: {
  '/ws': {
    target: 'ws://192.168.1.100',  // <-- Your ESP32 IP
    ws: true
  }
}
```

### Add new components
Create file in `src/components/` and import in `app.js`:
```javascript
import { MyComponent } from './components/MyComponent';
```

### Modify Tailwind theme
Edit `tailwind.config.js`:
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

### WebSocket doesn't connect in dev
- Check ESP32 IP in `vite.config.js`
- Verify ESP32 is reachable
- Use browser console (F12) to see errors

### Build too large
- Remove unused components
- In `vite.config.js` add `drop_console: true`
- Use only necessary Tailwind utilities

### Style doesn't load
- Verify `styles.css` is imported in `app.js`
- Regenerate Tailwind: `npm run build`

## 📚 Resources

- [Preact Docs](https://preactjs.com)
- [Tailwind CSS](https://tailwindcss.com)
- [Vite](https://vitejs.dev)

---

**Happy hacking! 😄**
