/*
 * WebUIManager.cpp
 *
 * Implementation of WebSocket-based real-time control interface.
 */

#include "system/WebUIManager.h"

#ifdef ENABLE_NETWORK

#include "system/Debug.h"
#include "audio/Oscillator.h"
#include "system/PerformanceMonitor.h"
#include "controls/SensorManager.h"

// Static pointer for event handler callback, we use it as we cannot pass [this]
// to the ws lambda.
static WebUIManager* g_webUIInstance = nullptr;

WebUIManager::WebUIManager(AsyncWebServer* srv, Theremin* thmn)
    : server(srv), ws("/ws"), theremin(thmn), lastUpdate(0) {
  g_webUIInstance = this;
}

void WebUIManager::begin() {
  if (!server || !theremin) {
    DEBUG_PRINTLN("[WebUI] ERROR: Invalid server or theremin pointer");
    return;
  }

  // Configure WebSocket
  // NOTE: we cannot pass this to [this] as onEvent expects a C-style function
  // pointer, hence we create a static pointer g_webUIInstance in global scope,
  // so we can use it in the lambda.
  ws.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (g_webUIInstance) {
      g_webUIInstance->onWebSocketEvent(server, client, type, arg, data, len);
    }
  });

  // Register WebSocket endpoint
  server->addHandler(&ws);

  DEBUG_PRINTLN("[WebUI] WebSocket endpoint registered at /ws");
}

void WebUIManager::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (!g_webUIInstance) return;

  switch (type) {
    case WS_EVT_CONNECT:
      DEBUG_PRINTF("[WebUI] Client #%u connected from %s\n", client->id(),
                   client->remoteIP().toString().c_str());
      g_webUIInstance->sendFullState(client);
      break;

    case WS_EVT_DISCONNECT:
      DEBUG_PRINTF("[WebUI] Client #%u disconnected\n", client->id());
      break;

    case WS_EVT_DATA: {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        g_webUIInstance->handleWebSocketMessage(client, data, len);
      }
      break;
    }

    case WS_EVT_ERROR:
      DEBUG_PRINTF("[WebUI] Client #%u error\n", client->id());
      break;

    case WS_EVT_PONG:
      // Pong received - connection alive
      break;
  }
}

void WebUIManager::handleWebSocketMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len) {
  // Null-terminate the data for JSON parsing
  data[len] = 0;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, (const char*)data);

  if (error) {
    DEBUG_PRINTF("[WebUI] JSON parse error: %s\n", error.c_str());
    return;
  }

  const char* cmd = doc["cmd"];
  if (!cmd) {
    DEBUG_PRINTLN("[WebUI] Missing 'cmd' field");
    return;
  }

  // Route command to appropriate handler
  if (strncmp(cmd, "setWaveform", 11) == 0 || strncmp(cmd, "setOctave", 9) == 0 ||
      strncmp(cmd, "setVolume", 9) == 0) {
    handleOscillatorCommand(doc);
  } else if (strncmp(cmd, "setEffectParam", 14) == 0 || strncmp(cmd, "enableEffect", 12) == 0) {
    handleEffectCommand(doc);
  } else if (strncmp(cmd, "setSmoothing", 12) == 0 || strncmp(cmd, "setRange", 8) == 0) {
    handleSystemCommand(doc);
  } else {
    DEBUG_PRINTF("[WebUI] Unknown command: %s\n", cmd);
  }
}

void WebUIManager::handleOscillatorCommand(JsonDocument& doc) {
  const char* cmd = doc["cmd"];
  int oscNum = doc["osc"] | 1;  // Default to oscillator 1

  if (oscNum < 1 || oscNum > 3) {
    DEBUG_PRINTF("[WebUI] Invalid oscillator number: %d\n", oscNum);
    return;
  }

  AudioEngine* audio = theremin->getAudioEngine();

  if (strcmp(cmd, "setWaveform") == 0) {
    const char* waveformStr = doc["value"];
    Oscillator::Waveform wf = Oscillator::OFF;

    if (strcmp(waveformStr, "OFF") == 0)
      wf = Oscillator::OFF;
    else if (strcmp(waveformStr, "SINE") == 0)
      wf = Oscillator::SINE;
    else if (strcmp(waveformStr, "SQUARE") == 0)
      wf = Oscillator::SQUARE;
    else if (strcmp(waveformStr, "TRIANGLE") == 0)
      wf = Oscillator::TRIANGLE;
    else if (strcmp(waveformStr, "SAW") == 0)
      wf = Oscillator::SAW;

    audio->setOscillatorWaveform(oscNum, wf);
    DEBUG_PRINTF("[WebUI] Osc %d waveform -> %s\n", oscNum, waveformStr);
    sendOscillatorState(oscNum);

  } else if (strcmp(cmd, "setOctave") == 0) {
    int octave = doc["value"] | 0;
    audio->setOscillatorOctave(oscNum, octave);
    DEBUG_PRINTF("[WebUI] Osc %d octave -> %d\n", oscNum, octave);
    sendOscillatorState(oscNum);

  } else if (strcmp(cmd, "setVolume") == 0) {
    float volume = doc["value"] | 1.0f;
    audio->setOscillatorVolume(oscNum, volume);
    DEBUG_PRINTF("[WebUI] Osc %d volume -> %.2f\n", oscNum, volume);
    sendOscillatorState(oscNum);
  }
}

void WebUIManager::handleEffectCommand(JsonDocument& doc) {
  const char* cmd = doc["cmd"];
  const char* effectName = doc["effect"];

  if (!effectName) {
    DEBUG_PRINTLN("[WebUI] Missing 'effect' field");
    return;
  }

  EffectsChain* effects = theremin->getAudioEngine()->getEffectsChain();

  if (strcmp(cmd, "enableEffect") == 0) {
    bool enabled = doc["value"] | false;

    if (strcmp(effectName, "delay") == 0) {
      effects->setDelayEnabled(enabled);
      DEBUG_PRINTF("[WebUI] Delay %s\n", enabled ? "enabled" : "disabled");
    } else if (strcmp(effectName, "chorus") == 0) {
      effects->setChorusEnabled(enabled);
      DEBUG_PRINTF("[WebUI] Chorus %s\n", enabled ? "enabled" : "disabled");
    } else if (strcmp(effectName, "reverb") == 0) {
      effects->setReverbEnabled(enabled);
      DEBUG_PRINTF("[WebUI] Reverb %s\n", enabled ? "enabled" : "disabled");
    }

    sendEffectState(effectName);

  } else if (strcmp(cmd, "setEffectParam") == 0) {
    const char* param = doc["param"];
    if (!param) {
      DEBUG_PRINTLN("[WebUI] Missing 'param' field");
      return;
    }

    if (strcmp(effectName, "delay") == 0) {
      DelayEffect* delay = effects->getDelay();
      if (strcmp(param, "time") == 0) {
        uint32_t time = doc["value"] | 300;
        delay->setDelayTime(time);
        DEBUG_PRINTF("[WebUI] Delay time -> %u ms\n", time);
      } else if (strcmp(param, "feedback") == 0) {
        float feedback = doc["value"] | 0.5f;
        delay->setFeedback(feedback);
        DEBUG_PRINTF("[WebUI] Delay feedback -> %.2f\n", feedback);
      } else if (strcmp(param, "mix") == 0) {
        float mix = doc["value"] | 0.3f;
        delay->setMix(mix);
        DEBUG_PRINTF("[WebUI] Delay mix -> %.2f\n", mix);
      }
      sendEffectState("delay");

    } else if (strcmp(effectName, "chorus") == 0) {
      ChorusEffect* chorus = effects->getChorus();
      if (strcmp(param, "rate") == 0) {
        float rate = doc["value"] | 1.0f;
        chorus->setRate(rate);
        DEBUG_PRINTF("[WebUI] Chorus rate -> %.2f Hz\n", rate);
      } else if (strcmp(param, "depth") == 0) {
        float depth = doc["value"] | 0.5f;
        chorus->setDepth(depth);
        DEBUG_PRINTF("[WebUI] Chorus depth -> %.2f\n", depth);
      } else if (strcmp(param, "mix") == 0) {
        float mix = doc["value"] | 0.5f;
        chorus->setMix(mix);
        DEBUG_PRINTF("[WebUI] Chorus mix -> %.2f\n", mix);
      }
      sendEffectState("chorus");

    } else if (strcmp(effectName, "reverb") == 0) {
      ReverbEffect* reverb = effects->getReverb();
      if (strcmp(param, "roomSize") == 0) {
        float size = doc["value"] | 0.5f;
        reverb->setRoomSize(size);
        DEBUG_PRINTF("[WebUI] Reverb room size -> %.2f\n", size);
      } else if (strcmp(param, "damping") == 0) {
        float damping = doc["value"] | 0.5f;
        reverb->setDamping(damping);
        DEBUG_PRINTF("[WebUI] Reverb damping -> %.2f\n", damping);
      } else if (strcmp(param, "mix") == 0) {
        float mix = doc["value"] | 0.3f;
        reverb->setMix(mix);
        DEBUG_PRINTF("[WebUI] Reverb mix -> %.2f\n", mix);
      }
      sendEffectState("reverb");
    }
  }
}

void WebUIManager::handleSystemCommand(JsonDocument& doc) {
  const char* cmd = doc["cmd"];

  if (strcmp(cmd, "setSmoothing") == 0) {
    const char* target = doc["target"];
    int preset = doc["value"] | 1;

    if (strcmp(target, "pitch") == 0) {
      theremin->setPitchSmoothingPreset((Theremin::SmoothingPreset)preset);
      DEBUG_PRINTF("[WebUI] Pitch smoothing preset -> %d\n", preset);
    } else if (strcmp(target, "volume") == 0) {
      theremin->setVolumeSmoothingPreset((Theremin::SmoothingPreset)preset);
      DEBUG_PRINTF("[WebUI] Volume smoothing preset -> %d\n", preset);
    }

  } else if (strcmp(cmd, "setRange") == 0) {
    int preset = doc["value"] | 1;
    theremin->setFrequencyRangePreset((Theremin::FrequencyRangePreset)preset);
    DEBUG_PRINTF("[WebUI] Frequency range preset -> %d\n", preset);
  }
}

void WebUIManager::sendFullState(AsyncWebSocketClient* client) {
  DEBUG_PRINTF("[WebUI] Sending full state to client #%u\n", client->id());

  // Send all oscillator states
  for (int i = 1; i <= 3; i++) {
    sendOscillatorState(i, client);
  }

  // Send all effect states
  sendEffectState("delay", client);
  sendEffectState("chorus", client);
  sendEffectState("reverb", client);

  // Send initial sensor and performance data
  sendSensorState(client);
  sendPerformanceState(client);
}

void WebUIManager::sendOscillatorState(int oscNum, AsyncWebSocketClient* client) {
  AudioEngine* audio = theremin->getAudioEngine();

  JsonDocument doc;
  doc["type"] = "oscillator";
  doc["osc"] = oscNum;

  // Get waveform as string
  Oscillator::Waveform wf = audio->getOscillatorWaveform(oscNum);
  const char* wfStr = "OFF";
  switch (wf) {
    case Oscillator::SINE:
      wfStr = "SINE";
      break;
    case Oscillator::SQUARE:
      wfStr = "SQUARE";
      break;
    case Oscillator::TRIANGLE:
      wfStr = "TRIANGLE";
      break;
    case Oscillator::SAW:
      wfStr = "SAW";
      break;
    default:
      wfStr = "OFF";
      break;
  }
  doc["waveform"] = wfStr;
  doc["octave"] = audio->getOscillatorOctave(oscNum);
  doc["volume"] = audio->getOscillatorVolume(oscNum);

  broadcastUpdate("oscillator", doc);
}

void WebUIManager::sendEffectState(const char* effectName, AsyncWebSocketClient* client) {
  EffectsChain* effects = theremin->getAudioEngine()->getEffectsChain();

  JsonDocument doc;
  doc["type"] = "effect";
  doc["effect"] = effectName;

  if (strcmp(effectName, "delay") == 0) {
    DelayEffect* delay = effects->getDelay();
    doc["enabled"] = delay->isEnabled();
    doc["time"] = delay->getDelayTime();
    doc["feedback"] = delay->getFeedback();
    doc["mix"] = delay->getMix();

  } else if (strcmp(effectName, "chorus") == 0) {
    ChorusEffect* chorus = effects->getChorus();
    doc["enabled"] = chorus->isEnabled();
    doc["rate"] = chorus->getRate();
    doc["depth"] = chorus->getDepth();
    doc["mix"] = chorus->getMix();

  } else if (strcmp(effectName, "reverb") == 0) {
    ReverbEffect* reverb = effects->getReverb();
    doc["enabled"] = reverb->isEnabled();
    doc["roomSize"] = reverb->getRoomSize();
    doc["damping"] = reverb->getDamping();
    doc["mix"] = reverb->getMix();
  }

  broadcastUpdate("effect", doc);
}

void WebUIManager::sendSensorState(AsyncWebSocketClient* client) {
  SensorManager* sensors = theremin->getSensorManager();

  JsonDocument doc;
  doc["type"] = "sensor";
  doc["pitch"] = sensors->getPitchDistance();
  doc["volume"] = sensors->getVolumeDistance();

  String output;
  serializeJson(doc, output);

  if (client) {
    client->text(output);
  } else {
    ws.textAll(output);
  }
}

void WebUIManager::sendPerformanceState(AsyncWebSocketClient* client) {
  JsonDocument doc;
  doc["type"] = "performance";
  doc["cpu"] = 0.0;  // CPU usage calculation would require additional monitoring
  doc["ram"] = ESP.getFreeHeap();
  doc["uptime"] = millis();

  String output;
  serializeJson(doc, output);

  if (client) {
    client->text(output);
  } else {
    ws.textAll(output);
  }
}

void WebUIManager::broadcastUpdate(const char* type, JsonDocument& doc) {
  String output;
  serializeJson(doc, output);
  ws.textAll(output);
}

void WebUIManager::update() {
  // Clean up dead connections
  ws.cleanupClients();

  // Periodic state broadcasts
  unsigned long now = millis();
  if (now - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = now;

    // Only broadcast if there are connected clients
    if (ws.count() > 0) {
      // Broadcast oscillator states so hardware changes are reflected in UI
      for (int i = 1; i <= 3; i++) {
        sendOscillatorState(i);
      }

      sendSensorState();
      sendPerformanceState();
    }
  }
}

#endif  // ENABLE_NETWORK
