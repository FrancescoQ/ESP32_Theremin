import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [preact()],

  build: {
    outDir: '../data',
    emptyOutDir: true,
    minify: 'esbuild',
    cssCodeSplit: false,
    rollupOptions: {
      output: {
        manualChunks: undefined,
        entryFileNames: 'app.js',
        assetFileNames: 'app.[ext]'
      }
    },
    target: 'es2015',
  },

  // Dev configs
  server: {
    port: 3000,
    // Proxy for ESP32 testing web ui locally while running esp32
    proxy: {
      '/ws': {
        target: 'ws://192.168.1.100', // Set ESP32 IP.
        ws: true
      },
      '/api': {
        target: 'http://192.168.1.100', // Set ESP32 IP.
        changeOrigin: true
      }
    }
  }
});
