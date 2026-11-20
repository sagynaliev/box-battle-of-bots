#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <atomic>
#include <mbedtls/base64.h>
#include "ArenaPage.h"
#include <driver/dac_continuous.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "VoiceAssets.h"

namespace {
constexpr uint8_t lightPins[] = {32, 26, 27, 14};  // R, G, B, W
constexpr uint8_t buttonPins[] = {21, 18, 22, 19};  // A, B, C, D in remote position order.
constexpr uint8_t inputActiveLevel[] = {HIGH, HIGH, HIGH, HIGH};
constexpr bool lightsActiveHigh = true;
constexpr bool inputDiagnostics = false;
constexpr uint32_t pressDebounceMs = 10;
constexpr uint32_t releaseDebounceMs = 15;
constexpr uint32_t repeatMs = 800;
constexpr uint32_t pairMs = 900;
constexpr uint32_t holdMs = 650;
constexpr uint32_t sampleRate = 16000;
constexpr int gainPercent = 80;
constexpr uint8_t defaultBrightness = 128;
constexpr char wifiName[] = "BoxBot-Arena";
constexpr char wifiPassword[] = "REPLACE-BEFORE-USE";
WebServer webServer(80);
QueueHandle_t webCommands = nullptr;
std::atomic<bool> webWinnerReady{false};

enum class WebCommand : uint8_t {
  Start, Stop, Finish, Winner, BlueIntro, RedIntro, Toggle, Full,
  Blue, BluePulse, BlueBlink, Red, RedPulse, RedBlink, Alternate, AllOff
};
constexpr const char *webNames[] = {
  "start", "stop", "finish", "winner", "blue-intro", "red-intro", "toggle", "full",
  "blue", "blue-pulse", "blue-blink", "red", "red-pulse", "red-blink", "alternate", "all-off"
};

enum Button : uint8_t { A, B, C, D, NoButton };
enum class Light { Off, Full, Blue, Red, BluePulse, RedPulse,
                   BlueBlink, RedBlink, Alternate, Finale };
enum class Voice : uint8_t { Fight, Stop, Round, Blue, Red, Final, BlueWins, RedWins };
constexpr Voice finaleVoice = Voice::Final;  // Or BlueWins / RedWins.

struct Clip {
  const uint8_t *data;
  size_t size;
};

constexpr Clip clips[] = {
  {voiceFight, sizeof voiceFight}, {voiceStop, sizeof voiceStop},
  {voiceRound, sizeof voiceRound}, {voiceBlue, sizeof voiceBlue},
  {voiceRed, sizeof voiceRed},
  finaleVoice == Voice::BlueWins ? Clip{voiceBlueWins, sizeof voiceBlueWins}
      : finaleVoice == Voice::RedWins ? Clip{voiceRedWins, sizeof voiceRedWins}
                                     : Clip{voiceFinal, sizeof voiceFinal},
};

QueueHandle_t audioRequests = nullptr;
dac_continuous_handle_t dac = nullptr;
Light light = Light::Off;
uint32_t effectStart = 0;
bool lightsReady = false;
uint8_t outputLevels[4]{};
uint8_t whiteLevel = 0;
bool roundFinished = false;

struct Input {
  bool raw;
  bool stable;
  uint32_t changedAt;
};

Input inputs[4]{};
Button pending = NoButton;
uint8_t clicks = 0;
uint32_t lastClick = 0;

constexpr size_t phoneAudioCapacity = 24576;
constexpr size_t phoneAudioChunk = 4096;
constexpr size_t phoneAudioLimit = sampleRate * 60;
portMUX_TYPE phoneAudioMux = portMUX_INITIALIZER_UNLOCKED;
struct PhoneAudio {
  uint8_t data[phoneAudioCapacity];
  uint32_t token, lastPacket;
  size_t head, bytes, received;
  bool ended, started, live;
};
PhoneAudio phoneAudio{};

void stopPhoneAudio() {
  portENTER_CRITICAL(&phoneAudioMux);
  phoneAudio.token = 0;
  portEXIT_CRITICAL(&phoneAudioMux);
}

size_t readPhoneAudio(uint8_t *output, size_t capacity) {
  const uint32_t now = millis();
  portENTER_CRITICAL(&phoneAudioMux);
  auto &stream = phoneAudio;
  size_t read = 0;
  const uint32_t expiry = stream.live ? 600 : 4000;
  if (stream.token && !stream.ended && now - stream.lastPacket > expiry) {
    stream.token = 0;
  }
  const size_t prebuffer = stream.live ? 2048 : 8192;
  if (stream.token && (stream.started || stream.ended || stream.bytes >= prebuffer)) {
    stream.started = true;
    read = min(capacity, stream.bytes);
    const size_t first = min(read, phoneAudioCapacity - stream.head);
    memcpy(output, stream.data + stream.head, first);
    memcpy(output + first, stream.data, read - first);
    stream.head = (stream.head + read) % phoneAudioCapacity;
    stream.bytes -= read;
    if (stream.bytes == 0) {
      stream.started = false;
      if (stream.ended) stream.token = 0;
    }
  }
  portEXIT_CRITICAL(&phoneAudioMux);
  return read;
}

void audioTask(void *) {
  Clip clip{};
  size_t position = 0;
  Voice playing = Voice::Fight;
  Voice requested = Voice::Fight;
  bool requestPending = false;
  while (true) {
    Voice voice;
    if (xQueueReceive(audioRequests, &voice, 0) == pdTRUE) {
      requested = voice;
      requestPending = true;
    }
    const bool finishingRound = playing == Voice::Round && requested == Voice::Final &&
                                position < clip.size;
    if (requestPending && !finishingRound) {
      stopPhoneAudio();
      playing = requested;
      clip = clips[static_cast<size_t>(playing)];
      position = 0;
      requestPending = false;
    }

    uint8_t samples[128];
    const size_t phoneSamples = position >= clip.size ? readPhoneAudio(samples, sizeof samples) : 0;
    for (size_t i = 0; i < sizeof samples; ++i) {
      int signal = 0;
      if (i < phoneSamples) {
        signal = int(samples[i]) - 128;
      } else if (position < clip.size) {
        signal = int(clip.data[position++]) - 128;
      }
      samples[i] = uint8_t(128 + signal * gainPercent / 100);
    }

#if !CONFIG_DAC_DMA_AUTO_16BIT_ALIGN
    uint16_t aligned[sizeof samples];
    for (size_t i = 0; i < sizeof samples; ++i) aligned[i] = uint16_t(samples[i]) << 8;
    const auto *data = reinterpret_cast<const uint8_t *>(aligned);
    const size_t size = sizeof aligned;
#else
    const auto *data = samples;
    const size_t size = sizeof samples;
#endif
    size_t offset = 0;
    while (offset < size) {
      size_t written = 0;
      const auto result = dac_continuous_write(dac, data + offset, size - offset,
                                               &written, 40);
      offset += written;
      if (result != ESP_OK && result != ESP_ERR_TIMEOUT) {
        Serial.printf("DAC write failed (%s)\n", esp_err_to_name(result));
        dac_continuous_disable(dac);
        vTaskDelete(nullptr);
      }
      if (written == 0) vTaskDelay(1);
    }
  }
}

void startAudio() {
  dac_continuous_config_t config{};
  config.chan_mask = DAC_CHANNEL_MASK_CH0;  // GPIO25 on classic ESP32.
  config.desc_num = 4;
  config.buf_size = 256;
  config.freq_hz = sampleRate;
  config.clk_src = DAC_DIGI_CLK_SRC_APLL;  // The default clock cannot reach 16 kHz.
  config.chan_mode = DAC_CHANNEL_MODE_SIMUL;

  auto result = dac_continuous_new_channels(&config, &dac);
  if (result == ESP_OK) result = dac_continuous_enable(dac);
  if (result != ESP_OK) {
    Serial.printf("DAC initialization failed (%s)\n", esp_err_to_name(result));
    if (dac) dac_continuous_del_channels(dac);
    return;
  }

  audioRequests = xQueueCreate(1, sizeof(Voice));
  if (audioRequests &&
      xTaskCreate(audioTask, "arena-audio", 4096, nullptr, 2, nullptr) == pdPASS) return;

  Serial.println("Could not start the audio task");
  if (audioRequests) vQueueDelete(audioRequests);
  audioRequests = nullptr;
  dac_continuous_disable(dac);
  dac_continuous_del_channels(dac);
}

void announce(Voice voice) {
  if (audioRequests) xQueueOverwrite(audioRequests, &voice);
}

void setLight(Light mode, uint32_t now) {
  light = mode;
  effectStart = now;
}

void restoreBackground(uint32_t now) {
  setLight(Light::Off, now);
}

void updateLights(uint32_t now) {
  if (!lightsReady) return;
  uint8_t levels[4] = {0, 0, 0, whiteLevel};
  const uint32_t elapsed = now - effectStart;
  const uint32_t phase = elapsed % 1200;
  const uint8_t pulse = (phase < 600 ? phase : 1200 - phase) * 255 / 600;
  const uint8_t blink = (elapsed / 100) % 2 == 0 ? 255 : 0;

  switch (light) {
    case Light::Off: break;
    case Light::Full:
      for (auto &level : levels) level = 255;
      break;
    case Light::Blue: levels[2] = 255; break;
    case Light::Red: levels[0] = 255; break;
    case Light::BluePulse: levels[2] = pulse; break;
    case Light::RedPulse: levels[0] = pulse; break;
    case Light::BlueBlink: levels[2] = blink; break;
    case Light::RedBlink: levels[0] = blink; break;
    case Light::Alternate: {
      const bool redTurn = (elapsed / 300) % 2 == 0;
      levels[0] = redTurn ? 255 : 0;
      levels[2] = redTurn ? 0 : 255;
      break;
    }
    case Light::Finale:
      for (auto &level : levels) level = 48;
      levels[(elapsed / 180) % 4] = 255;
      break;
  }

  for (size_t i = 0; i < 4; ++i) {
    if (levels[i] == outputLevels[i]) continue;
    ledcWrite(lightPins[i], lightsActiveHigh ? levels[i] : 255 - levels[i]);
    outputLevels[i] = levels[i];
  }
}

void finishRound(uint32_t now) {
  roundFinished = true;
  restoreBackground(now);
  announce(Voice::Round);
}

void dispatchClicks(Button button, uint8_t count, uint32_t now) {
  switch (button) {
    case A:
      if (count == 2) {
        whiteLevel = 255;
        setLight(Light::Full, now);
      } else {
        whiteLevel = whiteLevel ? 0 : defaultBrightness;
        if (light == Light::Full) restoreBackground(now);
      }
      break;
    case B:
      setLight(count == 1 ? Light::Blue : count == 2 ? Light::BluePulse : Light::BlueBlink, now);
      break;
    case C:
      setLight(count == 1 ? Light::Red : count == 2 ? Light::RedPulse : Light::RedBlink, now);
      break;
    case D:
      roundFinished = false;
      restoreBackground(now);
      announce(Voice::Fight);
      break;
    case NoButton: break;
  }
}

bool dispatchPair(Button first, Button second, uint32_t now) {
  if (first == D && second == A) {
    announce(Voice::Stop);
  } else if ((first == D && second == B) || (first == B && second == D)) {
    setLight(Light::BluePulse, now);
    announce(Voice::Blue);
  } else if ((first == D && second == C) || (first == C && second == D)) {
    setLight(Light::RedPulse, now);
    announce(Voice::Red);
  } else if (first == B && second == C) {
    setLight(Light::Alternate, now);
  } else if (first == A && second == D) {
    if (!roundFinished) return true;
    roundFinished = false;
    setLight(Light::Finale, now);
    announce(Voice::Final);
  } else {
    return false;
  }
  return true;
}

void flushClicks(uint32_t now) {
  dispatchClicks(pending, clicks, now);
  pending = NoButton;
  clicks = 0;
}

void acceptPress(Button button, uint32_t now) {
  const uint32_t elapsed = now - lastClick;
  if (pending == D && button == D && elapsed <= pairMs) {
    lastClick = now;
    return;
  }
  if (pending == button && elapsed > repeatMs) flushClicks(now);
  if (pending != NoButton && pending != button) {
    if (clicks == 1 && elapsed <= pairMs && dispatchPair(pending, button, now)) {
      pending = NoButton;
      clicks = 0;
      return;
    }
    flushClicks(now);
  }

  pending = button;
  lastClick = now;
  ++clicks;
  if (clicks == (button == A ? 2 : 3)) flushClicks(now);
}

void pollInputs(uint32_t now) {
  Button pressed = NoButton;
  uint8_t pressCount = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    auto &input = inputs[i];
    const bool raw = digitalRead(buttonPins[i]) == inputActiveLevel[i];
    if (raw != input.raw) {
      input.raw = raw;
      input.changedAt = now;
    }
    const auto debounceMs = raw ? pressDebounceMs : releaseDebounceMs;
    if (input.stable == raw || now - input.changedAt < debounceMs) continue;

    input.stable = raw;
    if (inputDiagnostics) {
      Serial.printf("%c GPIO%u = %d\n", 'A' + i, buttonPins[i], digitalRead(buttonPins[i]));
    }
    if (raw) {
      pressed = static_cast<Button>(i);
      ++pressCount;
    }
  }

  if (inputDiagnostics) return;
  if (pressCount == 1) acceptPress(pressed, now);
  if (pending == D && inputs[D].stable && inputs[D].raw && now - lastClick >= holdMs) {
    pending = NoButton;
    clicks = 0;
    finishRound(now);
  }
  // A first tap can still start a pair. A double tap can only become a triple.
  const auto waitMs = clicks == 1 ? pairMs : repeatMs;
  if (pending != NoButton && now - lastClick > waitMs) flushClicks(now);
}
void executeWebCommand(WebCommand command, uint32_t now) {
  switch (command) {
    case WebCommand::Start: dispatchClicks(D, 1, now); break;
    case WebCommand::Stop: dispatchPair(D, A, now); break;
    case WebCommand::Finish: finishRound(now); break;
    case WebCommand::Winner: dispatchPair(A, D, now); break;
    case WebCommand::BlueIntro: dispatchPair(D, B, now); break;
    case WebCommand::RedIntro: dispatchPair(D, C, now); break;
    case WebCommand::Toggle: dispatchClicks(A, 1, now); break;
    case WebCommand::Full: dispatchClicks(A, 2, now); break;
    case WebCommand::Blue: dispatchClicks(B, 1, now); break;
    case WebCommand::BluePulse: dispatchClicks(B, 2, now); break;
    case WebCommand::BlueBlink: dispatchClicks(B, 3, now); break;
    case WebCommand::Red: dispatchClicks(C, 1, now); break;
    case WebCommand::RedPulse: dispatchClicks(C, 2, now); break;
    case WebCommand::RedBlink: dispatchClicks(C, 3, now); break;
    case WebCommand::Alternate: dispatchPair(B, C, now); break;
    case WebCommand::AllOff:
      pending = NoButton;
      clicks = 0;
      whiteLevel = 0;
      restoreBackground(now);
      break;
  }
}

void handleWebCommand() {
  if (!webServer.hasHeader("Content-Type") ||
      !webServer.header("Content-Type").startsWith("text/plain") ||
      !webServer.hasHeader("X-Arena-Command")) {
    webServer.send(403);
    return;
  }
  const String name = webServer.arg("plain");
  for (uint8_t i = 0; i < sizeof(webNames) / sizeof(webNames[0]); ++i) {
    if (name != webNames[i]) continue;
    const auto command = static_cast<WebCommand>(i);
    if (command == WebCommand::Winner && !webWinnerReady.load()) {
      webServer.send(409);
    } else if (xQueueSend(webCommands, &command, 0) == pdTRUE) {
      webServer.send(202);
    } else {
      webServer.send(503);
    }
    return;
  }
  webServer.send(400);
}

void handlePhoneAudio() {
  if (!audioRequests || !webServer.hasHeader("X-Arena-Command")) {
    webServer.send(403);
    return;
  }
  const String key = webServer.arg("key");
  char *end = nullptr;
  const uint32_t token = strtoul(key.c_str(), &end, 16);
  if (key.length() != 8 || !end || *end || token == 0) {
    webServer.send(400);
    return;
  }
  const String action = webServer.arg("action");
  const bool live = action == "mic";
  const uint32_t now = millis();
  uint8_t chunk[phoneAudioChunk];
  size_t size = 0;
  if (action == "chunk") {
    const String encoded = webServer.arg("plain");
    if (encoded.isEmpty() || encoded.length() > (phoneAudioChunk + 2) / 3 * 4 ||
        mbedtls_base64_decode(chunk, sizeof chunk, &size,
          reinterpret_cast<const unsigned char *>(encoded.c_str()), encoded.length()) != 0) {
      webServer.send(400);
      return;
    }
  }

  int status = 204;
  portENTER_CRITICAL(&phoneAudioMux);
  auto &stream = phoneAudio;
  if (action == "start" || live) {
    if (stream.token) {
      status = 409;
    } else {
      stream.token = token;
      stream.live = live;
      stream.lastPacket = now;
      stream.head = stream.bytes = stream.received = 0;
      stream.ended = stream.started = false;
    }
  } else if (stream.token != token) {
    status = action == "status" ? 200 : 409;
  } else if (action == "chunk" && !stream.ended) {
    stream.lastPacket = now;
    const size_t bufferLimit = stream.live ? 4608 : phoneAudioCapacity;
    if (!stream.live && stream.received + size > phoneAudioLimit) {
      status = 413;
    } else if (size > bufferLimit - stream.bytes) {
      status = 429;
    } else {
      const size_t tail = (stream.head + stream.bytes) % phoneAudioCapacity;
      const size_t first = min(size, phoneAudioCapacity - tail);
      memcpy(stream.data + tail, chunk, first);
      memcpy(stream.data, chunk + first, size - first);
      stream.bytes += size;
      if (!stream.live) stream.received += size;
    }
  } else if (action == "end") {
    stream.ended = true;
  } else if (action == "stop") {
    stream.token = 0;
  } else if (action != "status") {
    status = 400;
  }
  portEXIT_CRITICAL(&phoneAudioMux);
  webServer.send(status);
}

void webTask(void *) {
  while (true) {
    webServer.handleClient();
    vTaskDelay(1);
  }
}

void startWebControl() {
  webCommands = xQueueCreate(8, sizeof(WebCommand));
  if (!webCommands || !WiFi.mode(WIFI_AP) || !WiFi.softAP(wifiName, wifiPassword)) {
    Serial.println("Arena Wi-Fi setup failed");
    return;
  }
  const char *headers[] = {"Content-Type", "X-Arena-Command"};
  webServer.collectHeaders(headers, 2);
  webServer.on("/", HTTP_GET, [] {
    webServer.send_P(200, "text/html; charset=utf-8", arenaPage);
  });
  webServer.on("/api/status", HTTP_GET, [] {
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send(200, "application/json",
                   webWinnerReady.load() ? "{\"winnerReady\":true}" : "{\"winnerReady\":false}");
  });
  webServer.on("/api/command", HTTP_POST, handleWebCommand);
  webServer.on("/api/audio", HTTP_POST, handlePhoneAudio);
  webServer.begin();
  if (xTaskCreate(webTask, "arena-web", 10240, nullptr, 1, nullptr) != pdPASS) {
    webServer.stop();
    WiFi.softAPdisconnect(true);
    Serial.println("Arena web task could not start");
    return;
  }
  Serial.printf("Arena control: http://%s/\n", WiFi.softAPIP().toString().c_str());
}
}  // namespace

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(buttonPins[i], INPUT);
    const int level = digitalRead(buttonPins[i]);
    const bool active = level == inputActiveLevel[i];
    inputs[i] = {active, active, millis()};
    if (inputDiagnostics) Serial.printf("%c GPIO%u boot level = %d\n", 'A' + i, buttonPins[i], level);
  }

  lightsReady = true;
  for (auto pin : lightPins) {
    if (ledcAttach(pin, 5000, 8)) {
      ledcWrite(pin, lightsActiveHigh ? 0 : 255);
    } else {
      lightsReady = false;
      Serial.printf("PWM initialization failed on GPIO%u\n", pin);
    }
  }

  if (!inputDiagnostics) {
    startAudio();
    startWebControl();
  }
}

void loop() {
  const uint32_t now = millis();
  pollInputs(now);
  if (!inputDiagnostics) {
    WebCommand command;
    if (webCommands && xQueueReceive(webCommands, &command, 0) == pdTRUE) executeWebCommand(command, now);
    webWinnerReady.store(roundFinished);
    updateLights(now);
  }
  vTaskDelay(1);
}
