
#include <Arduino.h>
#include <WiFiS3.h>
#include <Arduino_LED_Matrix.h>
#include <EEPROM.h>
#include <math.h>
#include <FspTimer.h>

// Suporte SD compilado por omissao: detecta a presença no arranque.
// Sem cartão ou modulo, a aquisição continua em RAM e no Safari.
#define ODL_ENABLE_SD 1
#define ODL_SD_CS_PIN 10
// Inicia automaticamente nova sessão, apenas se detectar um cartão válido.
#define ODL_SD_AUTOSTART 1

// OpenDataLogger v1.0.2 - aquisicao prioritaria, analise, alarmes, sessoes e rede Web
// Hardware: Arduino UNO R4 WiFi
// Web, 8 canais configuraveis, ring buffer,
// EEPROM, simulacao, matriz LED e relogio.

// As credenciais sao mantidas num ficheiro local excluido do Git.
#include "arduino_secrets.h"

// Configuracao compatível com ficheiros arduino_secrets.h de versões antigas.
// Para autorizar a mudança de Wi-Fi, adicionar neste ficheiro:
// #define ODL_WEB_ADMIN_KEY "uma-chave-forte-com-mais-de-12-caracteres"
#ifndef ODL_WEB_ADMIN_KEY
#define ODL_WEB_ADMIN_KEY "ALTERAR_CODIGO_ADMIN_FORTE"
#endif

// ---------- HARDWARE ----------
constexpr uint8_t CHANNELS = 8;
constexpr uint16_t HISTORY_BYTES = 4096;

const uint8_t channelPins[CHANNELS] = {
  A0, A1, A2, A3, A4, A5, 2, 3
};

enum ChannelType : uint8_t {
  OFF = 0,
  VOLTAGE = 1,
  NTC = 2,
  CURRENT = 3,
  POWER = 4,
  CUSTOM = 5,
  DIGITAL = 6
};

struct Channel {
  char name[20];
  char color[8];
  uint8_t type;
  uint8_t autoScale;

  float gain;
  float offset;
  float yMin;
  float yMax;

  float rFixed;
  float r25;
  float beta;
  float temperatureTrim;
};

#include "ChannelDefaults.h"

Channel channels[CHANNELS];

uint32_t sampleMs = 1000;
// Inicia a medir as entradas REAIS; DEMO e uma escolha explicita na Web.
bool simulation = false;
uint8_t matrixChannel = 0;

// ---------- ARMAZENAMENTO ----------
uint8_t history[HISTORY_BYTES];
uint8_t activeIds[CHANNELS];
uint8_t activeCount = 0;

uint8_t recordBytes = 4;
uint16_t capacity = 0;
uint16_t used = 0;
uint16_t head = 0;
uint32_t historyGeneration = 1;  // Distingue reinicios logicos do ring buffer.

uint16_t latest[CHANNELS] = { 0 };
uint32_t lastSampleMs = 0;

// O temporizador recolhe valores brutos num FIFO independente do HTTP.
// IMPORTANTE: analogRead() na ISR e experimental no core Renesas 1.6.0;
// validar na placa. ADC fica exclusivamente sob controlo da aquisicao.
// Nao usar Serial, String, WiFi, EEPROM ou renderBitmap na ISR.
constexpr uint8_t CAPTURE_QUEUE_SIZE = 64;
struct CaptureFrame {
  uint32_t timeMs;
  uint16_t values[CHANNELS];
};
CaptureFrame captureQueue[CAPTURE_QUEUE_SIZE];
volatile uint8_t captureWrite = 0;
volatile uint8_t captureRead = 0;
volatile uint32_t captureOverflows = 0;
volatile uint32_t captureTicks = 0;
volatile uint32_t maxIrqIntervalErrorMs = 0;
volatile uint32_t skippedIrqPeriods = 0;
volatile uint32_t lastIrqMs = 0;
volatile uint8_t maxQueueDepth = 0;
volatile bool acquisitionTimerActive = false;
bool acquisitionTimerReady = false;
FspTimer acquisitionTimer;

uint16_t readChannel(uint8_t id, uint32_t now);
void captureTimerCallback(timer_callback_args_t *args);
void stopTimedAcquisition();
void restartTimedAcquisition();
void clearCaptureQueue();
void consumeCaptureQueue();
void loadAdvancedSettings();
void saveAdvancedSettings();
void evaluateAlarms(uint32_t timestamp);
void processDigitalAlarmEvents();
void resetAlarms();
float calibratedNTC(uint8_t id,uint16_t raw);
void sdLogFrame(const CaptureFrame &frame);
void sdService();
void sdSetup();
void sdStop();
extern bool sdLogging;
void sendAdvancedStatus(Print &out);

// Instrumentacao: a amostragem e cooperativa, atrasos HTTP podem
// fazer perder instantes de aquisicao. Nao inventamos amostras.
uint32_t acquiredSamples = 0;
uint32_t missedSlots = 0;
uint32_t maxSampleLateMs = 0;
uint32_t lastSampleLateMs = 0;
uint32_t lastHttpUs = 0;
uint32_t maxHttpUs = 0;
uint32_t httpRequests = 0;
uint32_t lastLoopGapMs = 0;
uint32_t maxLoopGapMs = 0;
uint32_t previousLoopMs = 0;

// FASE 3: aquisicao continua ou captura por trigger.
// 0 = continuo; 1 = armado; 2 = a capturar pos-trigger; 3 = congelado.
volatile uint8_t triggerState = 0;
uint8_t triggerChannel = 6;    // D2 por defeito
uint8_t triggerCondition = 1;  // 1: subida, 2: descida, 3: superior, 4: inferior
float triggerLevel = 2.5f;     // unidade fisica do canal, para analogicos
uint32_t triggerPreMs = 5000UL;
uint32_t triggerPostMs = 5000UL;
uint32_t triggerTimeMs = 0;
uint16_t triggerPostCount = 0;
uint16_t triggerPostTarget = 0;
float triggerPrevValue = NAN;
volatile bool pendingDigitalTrigger = false;
volatile uint32_t pendingDigitalTimeMs = 0;

// Historico de transicoes independente das amostras analogicas.
// D2/D3 sao capturados por CHANGE, com a hora do evento.
// Circular de 128 transicoes, sobrescreve as mais antigas sem bloquear a ISR.
// Cada entrada ocupa 12 bytes; o envio Web e agrupado com /api/bin.
constexpr uint8_t DIGITAL_EVENT_CAPACITY = 128;
constexpr uint8_t DIGITAL_EVENT_BATCH = 32;
struct DigitalEdge {
  uint32_t sequence;
  uint32_t timeMs;
  uint16_t microPart; // fraccao aproximada de 1 ms, 0...999
  uint8_t channel;    // 6=D2, 7=D3
  uint8_t level;      // 0=OFF, 1=ON
};
DigitalEdge digitalEdges[DIGITAL_EVENT_CAPACITY];
volatile uint8_t digitalEdgeHead = 0;   // proxima posicao de escrita
volatile uint8_t digitalEdgeUsed = 0;
volatile uint32_t digitalEdgeSerial = 0;
volatile uint32_t digitalEdgeOverwritten = 0;
volatile uint8_t digitalPrevious[2] = {0, 0};
uint32_t nextSimEdgeMs[2] = {0, 0};

void appendDigitalEdge(uint8_t id, uint8_t level,
                       uint32_t instantMs, uint16_t microPart) {
  DigitalEdge &e = digitalEdges[digitalEdgeHead];
  e.sequence = ++digitalEdgeSerial;
  e.timeMs = instantMs;
  e.microPart = microPart;
  e.channel = id;
  e.level = level;
  digitalEdgeHead = (uint8_t)((digitalEdgeHead + 1) % DIGITAL_EVENT_CAPACITY);
  if (digitalEdgeUsed < DIGITAL_EVENT_CAPACITY) ++digitalEdgeUsed;
  else ++digitalEdgeOverwritten;
}

void resetDigitalEdgeTracking() {
  noInterrupts();
  digitalEdgeHead = 0;
  digitalEdgeUsed = 0;
  // Nao repor digitalEdgeSerial: o browser reconhece a mudanca pela geracao.
  interrupts();
  const uint32_t now=millis();
  for (uint8_t index=0; index<2; ++index) {
    const uint32_t period = index == 0 ? 10000UL : 8000UL;
    const uint32_t onMs   = index == 0 ?  5000UL :  250UL;
    const uint32_t phase = now % period;
    digitalPrevious[index] = simulation
      ? (phase < onMs)
      : (digitalRead(channelPins[index+6]) == LOW);
    nextSimEdgeMs[index] = now + (phase < onMs ? onMs-phase : period-phase);
  }
}

// Simulacao temporal exacta: mesmo que o loop atrase, recupera os
// flancos sintéticos que ocorreram, com a data devida, nao com a data
// tardia da leitura. O modo fisico usa as interrupcoes D2/D3.
void serviceSimulatedEdges(uint32_t now) {
  if (!simulation || triggerState == 3) return;
  // Intercalar os flancos de D2 e D3 pela sua data/hora, em vez de
  // criar todos os eventos de um canal antes dos do outro.
  for (uint8_t steps=0; steps<64; ++steps) {
    int8_t chosen=-1;
    for (uint8_t index=0; index<2; ++index) {
      if ((int32_t)(now-nextSimEdgeMs[index])<0) continue;
      if (chosen<0 || (int32_t)(nextSimEdgeMs[index]-nextSimEdgeMs[chosen])<0)
        chosen=index;
    }
    if (chosen<0) break;
    const uint8_t index=(uint8_t)chosen;
    const uint8_t id=index+6;
    const uint32_t period=index==0 ? 10000UL : 8000UL;
    const uint32_t onMs=index==0 ? 5000UL : 250UL;
    const uint32_t boundary=nextSimEdgeMs[index];
    const uint8_t level=!digitalPrevious[index];
    digitalPrevious[index]=level;
    nextSimEdgeMs[index]+=level ? onMs : (period-onMs);
    if(channels[id].type != DIGITAL) continue;
    appendDigitalEdge(id,level,boundary,0);
    if(triggerState==1 && !pendingDigitalTrigger &&
       triggerChannel==id &&
       ((triggerCondition==1 && level)||(triggerCondition==2 && !level))) {
      pendingDigitalTimeMs=boundary;
      pendingDigitalTrigger=true;
    }
  }
}

void observeDigitalEdge(uint8_t id) {
  // Na simulacao, as transicoes sinteticas sao criadas por acquire().
  if (simulation || channels[id].type != DIGITAL || triggerState == 3) return;
  // ISR: sem Serial, sem memoria dinamica, sem servidor Web.
  const uint8_t idx = id - 6;
  const uint8_t level = (digitalRead(channelPins[id]) == LOW) ? 1 : 0;
  if (level == digitalPrevious[idx]) return;
  digitalPrevious[idx] = level;
  const uint32_t stampUs = micros();
  const uint32_t tms = millis();
  appendDigitalEdge(id,level,tms,(uint16_t)(stampUs%1000UL));

  if (triggerState == 1 && !pendingDigitalTrigger &&
      triggerChannel == id &&
      ((triggerCondition == 1 && level) ||
       (triggerCondition == 2 && !level))) {
    pendingDigitalTimeMs = tms;
    pendingDigitalTrigger = true;
  }
}
void digitalIsrD2() { observeDigitalEdge(6); }
void digitalIsrD3() { observeDigitalEdge(7); }

// ---------- RELOGIO ----------
uint32_t epochAnchor = 0;
uint32_t millisAnchor = 0;
uint8_t clockSource = 0;
// 0: sem hora; 1: NTP; 2: navegador

// ---------- REDE E MATRIZ ----------
WiFiServer server(80);
bool usingAccessPoint = false;
ArduinoLEDMatrix matrix;

// Matriz 12 x 8: as mesmas fontes temporizadas utilizadas na Web.
// Analogicos: historico circular da aquisicao GPT.
// Digitais: historico de transicoes D2/D3 (tambem em modo simulacao).
// Nao criamos um segundo historico a partir de latest[]: essa era
// a causa do atraso e do desaparecimento intermitente dos impulsos.
uint32_t matrixWindowMs = 10000UL;
constexpr uint32_t MATRIX_RENDER_MS = 40UL;
constexpr uint8_t MATRIX_W = 12;
constexpr uint8_t MATRIX_H = 8;
uint8_t matrixTrackedChannel = 255;
uint8_t matrixTrackedType = 255;
uint32_t lastMatrixRenderMs = 0;

// ---------- EEPROM ----------
constexpr uint32_t SETTINGS_MAGIC = 0x4F444C33;
constexpr uint16_t SETTINGS_VERSION = 3;

struct StoredSettings {
  uint32_t magic;
  uint16_t version;
  uint32_t rate;
  Channel channel[CHANNELS];
};

// ---------- CONFIGURACAO ----------
void defaults() {
  // So o perfil inicial; nunca substitui uma configuracao EEPROM valida.
  memcpy(channels, ODL_DEFAULT_CHANNELS, sizeof(channels));
  sampleMs = 1000;
}

void loadSettings() {
  StoredSettings s = {};
  EEPROM.get(0, s);

  if (s.magic != SETTINGS_MAGIC || s.version != SETTINGS_VERSION || s.rate < 100 || s.rate > 10000) {
    defaults();
    return;
  }

  for (uint8_t i = 0; i < CHANNELS; i++) {
    const Channel &c = s.channel[i];

    if (c.type > DIGITAL || (i >= 6 && c.type != OFF && c.type != DIGITAL) || c.name[19] != '\0' || c.color[7] != '\0' || !isfinite(c.gain) || !isfinite(c.beta)) {
      defaults();
      return;
    }
  }

  memcpy(channels, s.channel, sizeof(channels));
  sampleMs = s.rate;
}

void saveSettings() {
  StoredSettings s = {};
  s.magic = SETTINGS_MAGIC;
  s.version = SETTINGS_VERSION;
  s.rate = sampleMs;

  memcpy(s.channel, channels, sizeof(channels));
  EEPROM.put(0, s);
  saveAdvancedSettings();
}

void configurePins() {
  for (uint8_t i = 0; i < CHANNELS; i++) {
    if (channels[i].type == DIGITAL) {
      pinMode(channelPins[i], INPUT_PULLUP);
    } else {
      pinMode(channelPins[i], INPUT);
    }
  }
}

// Reserva automatica por numero de canais.
// Cada registo: timestamp 4 bytes +
// 2 bytes por canal activo.
void rebuildHistory() {
  // Um ficheiro SD tem lista fixa de canais; nao mudar o formato a meio.
  if(sdLogging)sdStop();
  ++historyGeneration;
  activeCount = 0;

  for (uint8_t i = 0; i < CHANNELS; i++) {
    if (channels[i].type != OFF) {
      activeIds[activeCount++] = i;
    }
  }

  recordBytes = 4 + activeCount * 2;
  capacity = activeCount
               ? HISTORY_BYTES / recordBytes
               : 0;

  head = 0;
  used = 0;
  memset(latest, 0, sizeof(latest));
  matrixTrackedChannel = 255;  // Reconstruir plot se mudar configuracao/simulacao.
  triggerState = 0;   // Mudanca da disposicao invalida captura anterior.
  pendingDigitalTrigger = false;
  configurePins();
  resetAlarms();
  resetDigitalEdgeTracking();
}

// ---------- AQUISICAO ----------
// Sinusoide de demonstracao A1 por tabela pre-calculada (64 pontos).
// Sem sinf() na interrupcao GPT: custo curto e previsivel.
static const uint16_t ODL_SINE_64[64] = {
  2048, 2239, 2428, 2614, 2794, 2967, 3131, 3285,
  3427, 3555, 3669, 3768, 3850, 3914, 3961, 3989,
  3998, 3989, 3961, 3914, 3850, 3768, 3669, 3555,
  3427, 3285, 3131, 2967, 2794, 2614, 2428, 2239,
  2048, 1857, 1668, 1482, 1302, 1129, 965, 811,
  669, 541, 427, 328, 246, 182, 135, 107,
  98, 107, 135, 182, 246, 328, 427, 541,
  669, 811, 965, 1129, 1302, 1482, 1668, 1857
};
uint16_t simulatedRaw(uint8_t id, uint32_t now) {
  if (id == 1) {
    // 9,5 s por periodo: interpolacao linear de amostras da LUT.
    const uint32_t duration=9500UL;
    const uint32_t position=(now%duration)*64UL;
    const uint8_t index=(uint8_t)(position/duration);
    const uint32_t fraction=position%duration;
    const int32_t left=ODL_SINE_64[index];
    const int32_t delta=(int32_t)ODL_SINE_64[(index+1U)&63U]-left;
    return (uint16_t)(left+delta*(int32_t)fraction/(int32_t)duration);
  }
  if (id == 6) {
    return (now % 10000UL) < 5000UL;
  }

  if (id == 7) {
    return (now % 8000UL) < 250UL;
  }

  uint32_t period = 8000UL + id * 1500UL;
  uint32_t phase = now % period;
  uint32_t half = period / 2;

  uint32_t triangle = phase < half
                        ? phase * 4000UL / half
                        : (period - phase) * 4000UL / half;

  return 48 + triangle;
}

uint16_t readChannel(uint8_t id, uint32_t now) {
  if (simulation) {
    return simulatedRaw(id, now);
  }

  if (channels[id].type == DIGITAL) {
    return digitalRead(channelPins[id]) == LOW;
  }

  return analogRead(channelPins[id]);
}

float temperature(uint8_t id, uint16_t raw);

float sampleValue(uint8_t id, uint16_t raw) {
  const Channel &ch = channels[id];
  if (ch.type == DIGITAL) return raw ? 1.0f : 0.0f;
  if (ch.type == NTC) return temperature(id, raw);
  return (raw * (5.0f / 4095.0f)) * ch.gain + ch.offset;
}

void activateTrigger(uint32_t momentMs) {
  if (triggerState != 1) return;
  ++historyGeneration;
  triggerTimeMs = momentMs;
  triggerState = 2;
  triggerPostCount = 0;

  // Historico circular: retira apenas as amostras anteriores em excesso.
  const uint16_t requestedPost =
      (uint16_t)((triggerPostMs + sampleMs - 1) / sampleMs);
  triggerPostTarget = min((uint16_t)(capacity > 0 ? capacity - 1 : 0),
                          requestedPost);
  const uint16_t maxPre = capacity > triggerPostTarget
      ? capacity - triggerPostTarget : 0;
  const uint16_t requestedPre =
      (uint16_t)((triggerPreMs + sampleMs - 1) / sampleMs);
  used = min(used, min(maxPre, requestedPre));
  if (triggerPostTarget == 0) triggerState = 3;
}

void armTrigger() {
  ++historyGeneration;
  noInterrupts();
  pendingDigitalTrigger = false;
  interrupts();
  head = 0;
  used = 0;
  noInterrupts();
  digitalEdgeHead = 0;
  digitalEdgeUsed = 0;
  interrupts();
  triggerPostCount = 0;
  triggerTimeMs = 0;
  triggerPrevValue = NAN;
  lastSampleMs = millis();
  triggerState = 1;
}

void servicePendingTrigger() {
  if (triggerState != 1) return;
  uint32_t moment = 0;
  bool pending = false;
  noInterrupts();
  if (pendingDigitalTrigger) {
    moment = pendingDigitalTimeMs;
    pendingDigitalTrigger = false;
    pending = true;
  }
  interrupts();
  if (pending) activateTrigger(moment);
}

// ISR: executa-se mesmo quando o servidor HTTP bloqueia o loop().
// So faz ADC/GPIO e coloca amostras brutas em RAM. A ISR digital continua
// independente. Se o FIFO encher, descartamos a amostra nova e contabilizamos.
void captureTimerCallback(timer_callback_args_t *args) {
  (void)args;
  if (!acquisitionTimerActive || triggerState == 3 || activeCount == 0) return;
  const uint32_t now = millis();
  if (lastIrqMs) {
    const uint32_t elapsed = now - lastIrqMs;
    const uint32_t err = elapsed > sampleMs ? elapsed - sampleMs : sampleMs - elapsed;
    if (err > maxIrqIntervalErrorMs) maxIrqIntervalErrorMs = err;
    if (elapsed >= sampleMs * 2UL) skippedIrqPeriods += elapsed/sampleMs - 1;
  }
  lastIrqMs = now;
  ++captureTicks;

  const uint8_t next = (uint8_t)((captureWrite+1) % CAPTURE_QUEUE_SIZE);
  if (next == captureRead) {
    ++captureOverflows;
    return;
  }
  CaptureFrame &frame = captureQueue[captureWrite];
  frame.timeMs = now;
  for (uint8_t n = 0; n < activeCount; ++n) {
    frame.values[n] = readChannel(activeIds[n], now);
  }
  captureWrite = next; // Publicar a frame so depois da copia completa.
  const uint8_t depth = (uint8_t)((captureWrite+CAPTURE_QUEUE_SIZE-captureRead)%CAPTURE_QUEUE_SIZE);
  if (depth > maxQueueDepth) maxQueueDepth = depth;
}

void clearCaptureQueue() {
  noInterrupts();
  captureRead = captureWrite;
  lastIrqMs = 0;
  interrupts();
}

void stopTimedAcquisition() {
  acquisitionTimerActive = false;
  if (acquisitionTimerReady) acquisitionTimer.stop();
}

void restartTimedAcquisition() {
  clearCaptureQueue();
  if (!acquisitionTimerReady) return;
  // set_frequency so e chamado depois de begin/open (core 1.6.0).
  if (!acquisitionTimer.set_frequency(1000.0f/(float)sampleMs)) {
    acquisitionTimerReady = false;
    return;
  }
  lastIrqMs = 0;
  acquisitionTimerActive = true;
  if (!acquisitionTimer.start()) {
    acquisitionTimerActive = false;
    acquisitionTimerReady = false;
  }
}

void setupTimedAcquisition() {
  uint8_t type = GPT_TIMER;
  const int8_t timerIndex = FspTimer::get_available_timer(type);
  if (timerIndex < 0 ||
      !acquisitionTimer.begin(TIMER_MODE_PERIODIC, type, (uint8_t)timerIndex,
                             1000.0f/(float)sampleMs, 0.0f, captureTimerCallback) ||
      !acquisitionTimer.setup_overflow_irq(9) ||
      !acquisitionTimer.open()) {
    acquisitionTimerReady = false;
    Serial.println("Aviso: sem timer GPT; aquisicao cooperativa de reserva");
    return;
  }
  acquisitionTimerReady = true;
  restartTimedAcquisition();
  Serial.println("Aquisicao temporizada GPT iniciada");
}

void commitFrame(const CaptureFrame &frame) {
  if (!capacity || triggerState == 3) return;
  const uint32_t now = frame.timeMs;
  uint8_t *p = history + head * recordBytes;
  memcpy(p, &now, 4);
  p += 4;

  for (uint8_t n = 0; n < activeCount; n++) {
    uint8_t id = activeIds[n];
    uint16_t raw = frame.values[n];

    if (triggerState == 1 && id == triggerChannel &&
        id < 6 && (triggerCondition == 3 || triggerCondition == 4)) {
      float value = sampleValue(id, raw);
      if (isfinite(value) && isfinite(triggerPrevValue)) {
        if ((triggerCondition == 3 &&
             triggerPrevValue <= triggerLevel && value > triggerLevel) ||
            (triggerCondition == 4 &&
             triggerPrevValue >= triggerLevel && value < triggerLevel)) {
          pendingDigitalTimeMs = now;
          pendingDigitalTrigger = true;
        }
      }
      triggerPrevValue = value;
    }
    latest[id] = raw;
    memcpy(p, &raw, 2);
    p += 2;
  }

  head = (head + 1) % capacity;
  if (used < capacity) ++used;
  ++acquiredSamples;
  sdLogFrame(frame);
  evaluateAlarms(now);

  // O trigger analogico detectado nesta amostra inclui a amostra de disparo.
  if (triggerState == 1 && pendingDigitalTrigger) {
    uint32_t moment = pendingDigitalTimeMs;
    pendingDigitalTrigger = false;
    activateTrigger(moment);
  } else if (triggerState == 2) {
    if (++triggerPostCount >= triggerPostTarget) triggerState = 3;
  }
}

// Utilizado apenas antes de ligar o temporizador ou com ele parado.
void acquire() {
  CaptureFrame frame = {};
  frame.timeMs = millis();
  for (uint8_t n=0; n<activeCount; ++n)
    frame.values[n] = readChannel(activeIds[n], frame.timeMs);
  commitFrame(frame);
}

void consumeCaptureQueue() {
  // A aquisicao tem prioridade sobre a renderizacao e o HTTP.
  // Copiamos cada frame numa secção critica de ~20 bytes.
  CaptureFrame frame;
  for (uint8_t n=0; n<CAPTURE_QUEUE_SIZE; ++n) {
    bool hasFrame=false;
    noInterrupts();
    if (captureRead != captureWrite) {
      frame = captureQueue[captureRead];
      captureRead = (uint8_t)((captureRead+1)%CAPTURE_QUEUE_SIZE);
      hasFrame=true;
    }
    interrupts();
    if (!hasFrame) break;
    if (triggerState != 3) commitFrame(frame);
  }
}

void getRecord(uint16_t index,
               uint32_t &timestamp,
               uint16_t values[]) {
  uint16_t slot =
    (head + capacity - used + index) % capacity;

  const uint8_t *p =
    history + slot * recordBytes;

  memcpy(&timestamp, p, 4);
  p += 4;

  for (uint8_t n = 0; n < activeCount; n++) {
    memcpy(&values[n], p, 2);
    p += 2;
  }
}

// ---------- TEMPERATURA NTC ----------
// NTC ligada entre ADC e GND;
// resistencia fixa entre +5V e ADC.
float temperature(uint8_t id, uint16_t raw) {
  return calibratedNTC(id,raw);
}

// ---------- LED MATRIX: mesma base temporal da Web ----------
// O eixo temporal tem 12 colunas e 8 niveis; um impulso mais curto que
// uma coluna ocupa uma coluna, sem piscar devido a reamostragem tardia.
// A Web e a matriz usam as mesmas amostras / os mesmos flancos.
void getRecord(uint16_t index, uint32_t &timestamp, uint16_t values[]);

void resetMatrixTrace(uint32_t now) {
  matrixTrackedChannel = matrixChannel;
  matrixTrackedType = channels[matrixChannel].type;
  lastMatrixRenderMs = now - MATRIX_RENDER_MS;
}

void matrixPixel(uint8_t img[8][12], int x, int y) {
  if (x >= 0 && x < MATRIX_W && y >= 0 && y < MATRIX_H) img[y][x] = 1;
}

int matrixY(float v, float lo, float hi) {
  if (!(hi > lo)) return 4;
  const float position = (v - lo) / (hi - lo);
  return constrain((int)lroundf((1.0f - position) * 7.0f), 0, 7);
}

void matrixVertical(uint8_t img[8][12], int x, int y1, int y2) {
  if (y1 > y2) { int t=y1; y1=y2; y2=t; }
  for (int y=y1; y<=y2; ++y) matrixPixel(img,x,y);
}

void matrixLine(uint8_t img[8][12], int x1, int y1, int x2, int y2) {
  int dx=abs(x2-x1), sx=(x1<x2)?1:-1;
  int dy=-abs(y2-y1), sy=(y1<y2)?1:-1;
  int err=dx+dy;
  for(;;){
    matrixPixel(img,x1,y1);
    if(x1==x2 && y1==y2)break;
    const int e2=2*err;
    if(e2>=dy){err+=dy;x1+=sx;}
    if(e2<=dx){err+=dx;y1+=sy;}
  }
}

int matrixColumn(uint32_t t, uint32_t start) {
  const uint32_t offset=t-start;
  if(offset>=matrixWindowMs) return MATRIX_W-1;
  return constrain((int)((uint64_t)offset*MATRIX_W/matrixWindowMs),0,MATRIX_W-1);
}

// Estados digitais: reproduzir a forma de onda com os flancos exactos.
// Cada evento e copiado com interrupcoes desactivadas por poucos ciclos.
void drawMatrixDigital(uint8_t img[8][12], uint8_t id,
                       uint32_t start, uint32_t end) {
  const uint8_t idx=id-6;
  uint8_t snapHead,snapUsed,currentLevel;
  uint32_t snapSerial;
  noInterrupts();
  snapHead=digitalEdgeHead;
  snapUsed=digitalEdgeUsed;
  snapSerial=digitalEdgeSerial;
  currentLevel=digitalPrevious[idx];
  interrupts();

  // Encontrar o estado no inicio da janela: ultimo flanco anterior ou
  // estado oposto ao primeiro flanco posterior, se o anterior expirou.
  uint8_t initial=currentLevel;
  bool seenBefore=false, seenAfter=false;
  for(uint8_t i=0;i<snapUsed;++i){
    const uint8_t pos=(uint8_t)((snapHead+DIGITAL_EVENT_CAPACITY-snapUsed+i)%DIGITAL_EVENT_CAPACITY);
    DigitalEdge e;
    noInterrupts();e=digitalEdges[pos];interrupts();
    if((int32_t)(e.sequence-snapSerial)>0 || e.channel!=id)continue;
    if((int32_t)(e.timeMs-start)<0) {initial=e.level;seenBefore=true;}
    else if(!seenBefore && !seenAfter && (int32_t)(end-e.timeMs)>=0){
      initial=(uint8_t)!e.level;seenAfter=true;
    }
  }

  uint8_t level=initial;
  int previousX=0;
  for(uint8_t i=0;i<snapUsed;++i){
    const uint8_t pos=(uint8_t)((snapHead+DIGITAL_EVENT_CAPACITY-snapUsed+i)%DIGITAL_EVENT_CAPACITY);
    DigitalEdge e;
    noInterrupts();e=digitalEdges[pos];interrupts();
    if((int32_t)(e.sequence-snapSerial)>0 || e.channel!=id)continue;
    if((int32_t)(e.timeMs-start)<0 || (int32_t)(end-e.timeMs)<0)continue;
    const int x=matrixColumn(e.timeMs,start);
    const int y=level?0:7;
    for(int xx=previousX;xx<=x;++xx)matrixPixel(img,xx,y);
    if(level!=e.level)matrixVertical(img,x,0,7);
    level=e.level;
    previousX=x;
  }
  for(int x=previousX;x<MATRIX_W;++x)matrixPixel(img,x,level?0:7);
}

// Analogicos: utilizar apenas as amostras com timestamp do historico
// principal; nunca a variavel latest[] lida num instante posterior.
// Mostrador analogico com 1 pixel por coluna. As amostras REAIS sao
// agregadas por coluna; lacunas interiores sao interpoladas apenas para
// apresentacao na matriz. O historico/CSV permanece sempre sem filtragem.
// Isto evita o ziguezague produzido pelo algoritmo Bresenham quando
// duas amostras vizinhas saltam varios dos apenas 8 niveis verticais.
void drawMatrixAnalog(uint8_t img[8][12],uint8_t id,
                      uint32_t start,uint32_t end) {
  int position=-1;
  for(uint8_t k=0;k<activeCount;++k)
    if(activeIds[k]==id){position=k;break;}
  if(position<0 || !used)return;
  float sum[MATRIX_W]={};
  uint16_t amount[MATRIX_W]={};
  float values[MATRIX_W]={};
  bool valid[MATRIX_W]={};
  uint16_t raw[CHANNELS];
  for(int i=(int)used-1;i>=0;--i){
    uint32_t t;
    getRecord((uint16_t)i,t,raw);
    if((int32_t)(t-start)<0)break;
    if((int32_t)(end-t)<0)continue;
    const int x=matrixColumn(t,start);
    const float v=sampleValue(id,raw[position]);
    if(!isfinite(v))continue;
    sum[x]+=v;
    ++amount[x];
  }
  int first=-1,last=-1;
  for(int x=0;x<MATRIX_W;++x){
    if(!amount[x])continue;
    values[x]=sum[x]/(float)amount[x];
    valid[x]=true;
    if(first<0)first=x;
    last=x;
  }
  if(first<0)return;
  // Preencher apenas os intervalos entre colunas com dados reais,
  // sem inventar amostras antes/depois do intervalo medido.
  int prev=first;
  for(int x=first+1;x<=last;++x){
    if(!valid[x])continue;
    if(x>prev+1){
      const float a=values[prev],b=values[x];
      for(int k=prev+1;k<x;++k){
        values[k]=a+(b-a)*(float)(k-prev)/(float)(x-prev);
        valid[k]=true;
      }
    }
    prev=x;
  }
  float lo=INFINITY, hi=-INFINITY;
  for(int x=first;x<=last;++x){
    if(!valid[x])continue;
    lo=min(lo,values[x]);hi=max(hi,values[x]);
  }
  const Channel &ch=channels[id];
  if(!ch.autoScale && isfinite(ch.yMin) && isfinite(ch.yMax) && ch.yMax>ch.yMin){
    lo=ch.yMin;hi=ch.yMax;
  }else if(hi-lo<0.0001f){
    lo-=0.5f;hi+=0.5f;
  }
  // Uma unica celula por coluna para os analogicos; flancos digitais
  // continuam a usar tracos verticais em drawMatrixDigital().
  for(int x=first;x<=last;++x)
    if(valid[x])matrixPixel(img,x,matrixY(values[x],lo,hi));
}

void drawMatrixTrace(uint32_t now) {
  uint8_t img[8][12]={};
  const uint8_t id=matrixChannel;
  if(id>=CHANNELS || channels[id].type==OFF){
    matrix.renderBitmap(img,8,12);return;
  }
  uint32_t end=now;
  if(triggerState==3){
    // Captura congelada: a matriz nao deve continuar a deslocar os
    // dados depois de terminar a aquisicao por trigger.
    if(used){
      uint16_t values[CHANNELS];
      getRecord(used-1,end,values);
    } else if(triggerTimeMs) end=triggerTimeMs;
  }
  const uint32_t start=end-matrixWindowMs;
  if(channels[id].type==DIGITAL)drawMatrixDigital(img,id,start,end);
  else drawMatrixAnalog(img,id,start,end);
  matrix.renderBitmap(img,8,12);
}

void serviceMatrix(uint32_t now) {
  if(matrixChannel>=CHANNELS)return;
  if(matrixTrackedChannel!=matrixChannel ||
     matrixTrackedType!=channels[matrixChannel].type)resetMatrixTrace(now);
  if((uint32_t)(now-lastMatrixRenderMs)>=MATRIX_RENDER_MS){
    lastMatrixRenderMs=now;
    drawMatrixTrace(now);
  }
}

// Durante respostas Web extensas, manter a matriz responsiva.
// Nao altera o historico durante a transferencia binaria em curso.
void serviceMatrixDuringHttp() { serviceMatrix(millis()); }

// Na transferencia inicial do HTML podemos descarregar a FIFO entre blocos.
void serviceBackgroundDuringPage() {
  const uint32_t now=millis();
  serviceSimulatedEdges(now);
  processDigitalAlarmEvents();
  servicePendingTrigger();
  if(acquisitionTimerReady)consumeCaptureQueue();
  serviceMatrix(now);
}

// Print com buffer: milhares de print() curtos para o modulo WiFiS3
// passam a escritas de blocos. A resposta Web permanece em streaming,
// sem copiar o JSON inteiro para RAM.
void headers(Print &c,const char *contentType);
class BufferedHttpPrint : public Print {
public:
  explicit BufferedHttpPrint(WiFiClient &client):client_(client){}
  using Print::write;
  size_t write(uint8_t c) override {
    if(filled_==sizeof(bytes_))flushBlock();
    bytes_[filled_++]=c;
    return 1;
  }
  size_t write(const uint8_t *buf,size_t len) override {
    const size_t requested=len;
    while(len){
      if(filled_==sizeof(bytes_))flushBlock();
      const size_t n=min(len,sizeof(bytes_)-filled_);
      memcpy(bytes_+filled_,buf,n);
      filled_+=n;buf+=n;len-=n;
    }
    return requested;
  }
  void flushBlock() {
    size_t written=0;
    while(written<filled_){
      size_t n=client_.write(bytes_+written,filled_-written);
      if(!n)break;
      written+=n;
    }
    filled_=0;
    serviceMatrixDuringHttp();
  }
  void finish(){if(filled_)flushBlock();}
private:
  WiFiClient &client_;
  uint8_t bytes_[256];
  size_t filled_=0;
};

void sendShortJson(WiFiClient &c,const char* body){
  BufferedHttpPrint out(c);
  headers(out,"application/json");
  out.print(body);
  out.finish();
}

// Resposta pequena para alteracoes simples: evita transmitir novamente
// todos os canais a cada seleccao na Web.
void sendControlAck(WiFiClient &c,bool trigger=false) {
  char buf[240];
  if(trigger){
    snprintf(buf,sizeof(buf),
      "{\"ok\":true,\"trigger\":{\"state\":%u,\"channel\":%u,\"condition\":%u,\"level\":%.4f,\"preMs\":%lu,\"postMs\":%lu,\"momentMs\":%lu}}",
      (unsigned)triggerState,(unsigned)triggerChannel,(unsigned)triggerCondition,
      (double)triggerLevel,(unsigned long)triggerPreMs,
      (unsigned long)triggerPostMs,(unsigned long)triggerTimeMs);
  } else {
    snprintf(buf,sizeof(buf),"{\"ok\":true,\"matrix\":%u,\"matrixWindowMs\":%lu}",
      (unsigned)matrixChannel,(unsigned long)matrixWindowMs);
  }
  sendShortJson(c,buf);
}

// ---------- UTILITARIOS HTTP ----------
void headers(Print &c,
             const char *contentType) {
  c.println(F("HTTP/1.1 200 OK"));
  c.print(F("Content-Type: "));
  c.println(contentType);
  c.println(F("Cache-Control: no-store"));
  c.println(F("Connection: close"));
  c.println();
}

int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

String decodeUrl(const String &s) {
  String result;
  result.reserve(s.length());

  for (unsigned int i = 0; i < s.length(); i++) {
    if (s[i] == '+') {
      result += ' ';
    } else if (s[i] == '%' && i + 2 < s.length()) {
      int a = hexValue(s[i + 1]);
      int b = hexValue(s[i + 2]);

      if (a >= 0 && b >= 0) {
        result += char((a << 4) | b);
        i += 2;
      }
    } else {
      result += s[i];
    }
  }

  return result;
}

String param(const String &url, const char *name) {
  String key = String(name) + "=";
  int p = url.indexOf(key);

  if (p < 0) return "";

  p += key.length();
  int e = url.indexOf('&', p);

  if (e < 0) e = url.length();

  return decodeUrl(url.substring(p, e));
}

void jsonText(Print &c, const char *s) {
  c.print('"');

  while (*s) {
    if (*s == '"' || *s == '\\') {
      c.print('\\');
    }

    if ((uint8_t)*s >= 32) {
      c.print(*s);
    }

    s++;
  }

  c.print('"');
}

#include "AdvancedFeatures.h"
#include "OptionalSD.h"

void sendStatus(WiFiClient &c) {
  BufferedHttpPrint out(c);
  headers(out, "application/json");

  out.print(F("{\"version\":\"1.0.3\","));
  out.print(F("\"simulation\":"));
  out.print(simulation ? 1 : 0);

  out.print(F(",\"sampleMs\":"));
  out.print(sampleMs);

  out.print(F(",\"bytes\":"));
  out.print(HISTORY_BYTES);

  out.print(F(",\"recordBytes\":"));
  out.print(recordBytes);

  out.print(F(",\"capacity\":"));
  out.print(capacity);

  out.print(F(",\"used\":"));
  out.print(used);

  out.print(F(",\"retentionMs\":"));
  out.print(capacity
            ? (uint32_t)(capacity - 1) * sampleMs
            : 0);

  out.print(F(",\"matrix\":"));
  out.print(matrixChannel);
  out.print(F(",\"matrixWindowMs\":"));
  out.print(matrixWindowMs);
  out.print(F(",\"trigger\":{"));
  out.print(F("\"state\":")); out.print(triggerState);
  out.print(F(",\"channel\":")); out.print(triggerChannel);
  out.print(F(",\"condition\":")); out.print(triggerCondition);
  out.print(F(",\"level\":")); out.print(triggerLevel, 4);
  out.print(F(",\"preMs\":")); out.print(triggerPreMs);
  out.print(F(",\"postMs\":")); out.print(triggerPostMs);
  out.print(F(",\"momentMs\":")); out.print(triggerTimeMs);
  out.print(F("}"));

  out.print(F(",\"epoch\":"));
  out.print(epochAnchor);

  out.print(F(",\"clockMillis\":"));
  out.print(millisAnchor);

  out.print(F(",\"clockSource\":"));
  out.print(clockSource);

  out.print(F(",\"ip\":"));
  jsonText(out, WiFi.localIP().toString().c_str());

  out.print(F(",\"gateway\":"));
  jsonText(out, WiFi.gatewayIP().toString().c_str());

  out.print(F(",\"dns\":"));
  jsonText(out, WiFi.dnsIP().toString().c_str());

  out.print(F(",\"channels\":["));

  for (uint8_t i = 0; i < CHANNELS; i++) {
    if (i) out.print(',');

    Channel &ch = channels[i];

    out.print(F("{\"id\":"));
    out.print(i);

    out.print(F(",\"pin\":"));
    jsonText(out, i < 6
                  ? (String("A") + i).c_str()
                  : (i == 6 ? "D2" : "D3"));

    out.print(F(",\"name\":"));
    jsonText(out, ch.name);

    out.print(F(",\"color\":"));
    jsonText(out, ch.color);

    out.print(F(",\"type\":"));
    out.print(ch.type);

    out.print(F(",\"auto\":"));
    out.print(ch.autoScale);

    out.print(F(",\"gain\":"));
    out.print(ch.gain, 5);

    out.print(F(",\"offset\":"));
    out.print(ch.offset, 5);

    out.print(F(",\"min\":"));
    out.print(ch.yMin, 4);

    out.print(F(",\"max\":"));
    out.print(ch.yMax, 4);

    out.print(F(",\"rfixed\":"));
    out.print(ch.rFixed, 1);

    out.print(F(",\"r25\":"));
    out.print(ch.r25, 1);

    out.print(F(",\"beta\":"));
    out.print(ch.beta, 1);

    out.print(F(",\"trim\":"));
    out.print(ch.temperatureTrim, 3);

    out.print('}');
  }

  out.print(F("]"));
  sendAdvancedStatus(out);
  out.println(F("}"));
  out.finish();
}

void sendDiagnostics(WiFiClient &c) {
  BufferedHttpPrint out(c);
  headers(out,"application/json");
  out.print(F("{\"acquired\":")); out.print(acquiredSamples);
  out.print(F(",\"missedSlots\":")); out.print(missedSlots);
  out.print(F(",\"lastLateMs\":")); out.print(lastSampleLateMs);
  out.print(F(",\"maxLateMs\":")); out.print(maxSampleLateMs);
  out.print(F(",\"lastHttpUs\":")); out.print(lastHttpUs);
  out.print(F(",\"maxHttpUs\":")); out.print(maxHttpUs);
  out.print(F(",\"httpRequests\":")); out.print(httpRequests);
  out.print(F(",\"lastLoopGapMs\":")); out.print(lastLoopGapMs);
  out.print(F(",\"maxLoopGapMs\":")); out.print(maxLoopGapMs);
  out.print(F(",\"matrixWindowMs\":")); out.print(matrixWindowMs);
  uint32_t qDrops, ticks, irqErr, irqSkipped;
  uint8_t maxDepth, depth;
  noInterrupts();
  qDrops = captureOverflows;
  ticks = captureTicks;
  irqErr = maxIrqIntervalErrorMs;
  irqSkipped = skippedIrqPeriods;
  maxDepth = maxQueueDepth;
  depth = (uint8_t)((captureWrite+CAPTURE_QUEUE_SIZE-captureRead)%CAPTURE_QUEUE_SIZE);
  interrupts();
  out.print(F(",\"timerMode\":")); out.print(acquisitionTimerReady ? 1 : 0);
  out.print(F(",\"irqTicks\":")); out.print(ticks);
  out.print(F(",\"queueDrops\":")); out.print(qDrops);
  out.print(F(",\"queueDepth\":")); out.print(depth);
  out.print(F(",\"maxQueueDepth\":")); out.print(maxDepth);
  out.print(F(",\"maxIrqJitterMs\":")); out.print(irqErr);
  out.print(F(",\"skippedIrqPeriods\":")); out.print(irqSkipped);
  uint8_t edgeCount;
  uint32_t edgeLost, edgeSerial;
  noInterrupts();
  edgeCount=digitalEdgeUsed;
  edgeLost=digitalEdgeOverwritten;
  edgeSerial=digitalEdgeSerial;
  interrupts();
  out.print(F(",\"edgesStored\":")); out.print(edgeCount);
  out.print(F(",\"edgesOverwritten\":")); out.print(edgeLost);
  out.print(F(",\"edgesSerial\":")); out.print(edgeSerial);
  out.print(F(",\"alarmTotal\":"));out.print(alarmTotal);
  out.print(F(",\"alarmEdgeGaps\":"));out.print(alarmEdgeGaps);
  out.print(F(",\"sdLogging\":"));out.print(sdLogging?1:0);
  out.println('}');
  out.finish();
}

void sendData(WiFiClient &c, const String &url) {
  String afterText = param(url, "after");
  bool incremental = afterText.length() > 0;
  uint32_t after = strtoul(
    afterText.c_str(), nullptr, 10);

  bool reset = false;

  if (incremental && used > 0) {
    uint32_t newest;
    uint16_t values[CHANNELS];
    getRecord(used - 1, newest, values);

    // O Arduino pode ter reiniciado.
    if ((int32_t)(newest - after) < 0) {
      incremental = false;
      reset = true;
    }
  }

  headers(c, "application/json");

  c.print(F("{\"ids\":["));
  for (uint8_t i = 0; i < activeCount; i++) {
    if (i) c.print(',');
    c.print(activeIds[i]);
  }

  c.print(F("],\"reset\":"));
  c.print(reset ? F("true") : F("false"));

  c.print(F(",\"used\":"));
  c.print(used);
  c.print(F(",\"capacity\":"));
  c.print(capacity);
  c.print(F(",\"recordBytes\":"));
  c.print(recordBytes);
  c.print(F(",\"sampleMs\":"));
  c.print(sampleMs);

  c.print(F(",\"retentionMs\":"));
  c.print(capacity ?
    (uint32_t)(capacity - 1) * sampleMs : 0);

  c.print(F(",\"triggerState\":")); c.print(triggerState);
  c.print(F(",\"triggerMomentMs\":")); c.print(triggerTimeMs);
  c.print(F(",\"rows\":["));

  char buffer[384];
  char line[128];
  size_t buffered = 0;
  bool first = true;

  uint16_t values[CHANNELS];

  for (uint16_t i = 0; i < used; i++) {
    uint32_t t;
    getRecord(i, t, values);

    if (incremental &&
        (int32_t)(t - after) <= 0) {
      continue;
    }

    size_t len = snprintf(
      line, sizeof(line),
      "%s[%lu",
      first ? "" : ",",
      (unsigned long)t);

    for (uint8_t n = 0; n < activeCount; n++) {
      len += snprintf(
        line + len,
        sizeof(line) - len,
        ",%u", (unsigned)values[n]);
    }

    line[len++] = ']';
    first = false;

    if (buffered + len > sizeof(buffer)) {
      c.write((const uint8_t*)buffer, buffered);
      buffered = 0;
    }

    memcpy(buffer + buffered, line, len);
    buffered += len;
  }

  if (buffered) {
    c.write((const uint8_t*)buffer, buffered);
  }

  c.println(F("]}"));
}

// ---------- TRANSFERENCIA RAPIDA HTTP ----------
// ODLB v2 Little Endian. Header 64 bytes; amostras e depois transicoes.
// Primeiros 48 bytes mantem os offsets ODLB v1.
// 48: edgeCount(u16); 50: latestSeq(u32); 54: overwritten(u32).
// Eventos: 12B [serial:u32,timeMs:u32,subUs:u16,id:u8,level:u8].
// Flags: 1=historico redefinido, 2=transicoes perdidas pelo cliente.
static void put16(uint8_t *b, uint16_t x) {
  b[0]=(uint8_t)x; b[1]=(uint8_t)(x>>8);
}
static void put32(uint8_t *b, uint32_t x) {
  b[0]=(uint8_t)x; b[1]=(uint8_t)(x>>8);
  b[2]=(uint8_t)(x>>16); b[3]=(uint8_t)(x>>24);
}

void sendFastData(WiFiClient &c, const String &url) {
  serviceSimulatedEdges(millis());
  const String afterText=param(url,"after");
  const String genText=param(url,"gen");
  const bool wantsDelta=afterText.length() && genText.length();
  const uint32_t after=strtoul(afterText.c_str(),nullptr,10);
  const uint32_t clientGen=strtoul(genText.c_str(),nullptr,10);
  uint32_t oldest=0,newest=0;
  uint16_t scratch[CHANNELS];
  if(used) {
    getRecord(0,oldest,scratch);
    getRecord(used-1,newest,scratch);
  }
  const bool reset = !wantsDelta || clientGen != historyGeneration ||
    (used && ((int32_t)(newest-after)<0 ||
      (int32_t)(oldest-after) > (int32_t)sampleMs));
  uint16_t start=0;
  if(!reset) {
    start=used;
    while(start>0) {
      uint32_t stamp;
      getRecord(start-1,stamp,scratch);
      if((int32_t)(stamp-after)<=0)break;
      --start;
    }
  }
  const uint16_t count=used-start;
  const uint16_t stride=recordBytes;

  // Um unico bloco pequeno de transicoes por pedido, para nao atrasar
  // o WiFiS3. Copia atomica antes de escrever na rede.
  DigitalEdge edgeBatch[DIGITAL_EVENT_BATCH];
  uint8_t edgeCount=0;
  bool eventGap=false;
  const String edgeAfterText=param(url,"edgeAfter");
  const bool edgeDelta=wantsDelta && !reset && edgeAfterText.length();
  const uint32_t edgeAfter=strtoul(edgeAfterText.c_str(),nullptr,10);
  uint32_t edgeNewestSerial, overwrittenCount;
  noInterrupts();
  const uint8_t eventCountSnapshot=digitalEdgeUsed;
  const uint8_t firstSlot=(uint8_t)((digitalEdgeHead+DIGITAL_EVENT_CAPACITY-eventCountSnapshot)%DIGITAL_EVENT_CAPACITY);
  edgeNewestSerial=digitalEdgeSerial;
  overwrittenCount=digitalEdgeOverwritten;
  if(edgeDelta && eventCountSnapshot &&
     (int32_t)(digitalEdges[firstSlot].sequence-edgeAfter)>1) eventGap=true;
  for(uint8_t i=0;i<eventCountSnapshot && edgeCount<DIGITAL_EVENT_BATCH;++i) {
    const DigitalEdge &e=digitalEdges[(firstSlot+i)%DIGITAL_EVENT_CAPACITY];
    if(!edgeDelta || (int32_t)(e.sequence-edgeAfter)>0)
      edgeBatch[edgeCount++]=e;
  }
  interrupts();

  const uint32_t bodyLen=64UL+(uint32_t)count*stride+(uint32_t)edgeCount*12UL;
  char headersBuf[170];
  const int hlen=snprintf(headersBuf,sizeof(headersBuf),
    "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n"
    "Cache-Control: no-store\r\nConnection: close\r\n"
    "Content-Length: %lu\r\n\r\n", (unsigned long)bodyLen);
  if(hlen<=0 || (size_t)hlen>=sizeof(headersBuf))return;
  c.write((const uint8_t*)headersBuf,(size_t)hlen);
  serviceMatrixDuringHttp();

  uint8_t header[64]={};
  header[0]='O';header[1]='D';header[2]='L';header[3]='B';
  header[4]=2;
  header[5]=(reset?1:0)|(eventGap?2:0);
  header[6]=activeCount;header[7]=triggerState;
  put16(header+8,capacity);put16(header+10,used);
  put32(header+12,sampleMs);put32(header+16,triggerTimeMs);
  put32(header+20,historyGeneration);
  put32(header+24,oldest);put32(header+28,newest);
  put16(header+32,count);put16(header+34,stride);
  for(uint8_t i=0;i<activeCount;++i)header[36+i]=activeIds[i];
  put32(header+44,acquiredSamples);
  put16(header+48,edgeCount);
  put32(header+50,edgeNewestSerial);
  put32(header+54,overwrittenCount);
  put32(header+58,millis()); // Tempo do dispositivo para eixo Web alinhado com matriz.
  c.write(header,sizeof(header));
  serviceMatrixDuringHttp();

  uint8_t packet[384];
  size_t bytes=0;
  for(uint16_t i=start;i<used;++i) {
    uint32_t stamp;
    getRecord(i,stamp,scratch);
    if(bytes+stride>sizeof(packet)) {
      c.write(packet,bytes);bytes=0;
      serviceMatrixDuringHttp();
    }
    put32(packet+bytes,stamp);bytes+=4;
    for(uint8_t k=0;k<activeCount;++k) {
      put16(packet+bytes,scratch[k]);bytes+=2;
    }
  }
  if(bytes){c.write(packet,bytes);serviceMatrixDuringHttp();}

  uint8_t edgeWire[DIGITAL_EVENT_BATCH*12];
  for(uint8_t i=0;i<edgeCount;++i){
    uint8_t *rec=edgeWire+i*12;
    const DigitalEdge &e=edgeBatch[i];
    put32(rec,e.sequence);put32(rec+4,e.timeMs);
    put16(rec+8,e.microPart);rec[10]=e.channel;rec[11]=e.level;
  }
  if(edgeCount){c.write(edgeWire,(size_t)edgeCount*12U);serviceMatrixDuringHttp();}
}

void updateTrigger(const String &url) {
  const String action = param(url, "action");
  if (action == "continuous") {
    triggerState = 0;
    pendingDigitalTrigger = false;
    // Uma captura congelada nao deve criar flancos retroactivos
    // ao regressar ao modo continuo.
    resetDigitalEdgeTracking();
    ++historyGeneration;
    return;
  }
  if (action == "stop") {
    triggerState = 3;
    pendingDigitalTrigger = false;
    return;
  }
  if (action != "arm") return;

  int id = param(url, "id").toInt();
  int condition = param(url, "condition").toInt();
  float level = param(url, "level").toFloat();
  uint32_t pre = (uint32_t)param(url, "pre").toInt();
  uint32_t post = (uint32_t)param(url, "post").toInt();
  if (id < 0 || id >= CHANNELS || channels[id].type == OFF ||
      !isfinite(level) || pre > 30000 || post > 30000 ||
      post == 0 || pre == 0) return;
  pre = max(pre, sampleMs);
  post = max(post, sampleMs);
  if (channels[id].type == DIGITAL) {
    if (condition != 1 && condition != 2) return;
  } else if (condition != 3 && condition != 4) return;

  triggerChannel = (uint8_t)id;
  triggerCondition = (uint8_t)condition;
  triggerLevel = level;
  triggerPreMs = pre;
  triggerPostMs = post;
  armTrigger();
}

void updateChannel(const String &url) {
  int id = param(url, "id").toInt();
  if (id < 0 || id >= CHANNELS) return;

  int type = param(url, "type").toInt();

  if (type < OFF || type > DIGITAL) return;
  if (id >= 6 && type != OFF && type != DIGITAL)
    return;

  Channel &ch = channels[id];
  const bool layoutChanged = ch.type != type;
  if(layoutChanged){stopTimedAcquisition();consumeCaptureQueue();}
  String name = param(url, "name");
  String color = param(url, "color");

  name.trim();

  if (name.length() > 0 && name.length() < 20) {
    name.toCharArray(ch.name, sizeof(ch.name));
  }

  if (color.length() == 7 && color[0] == '#') {
    bool valid = true;

    for (int i = 1; i < 7; i++) {
      if (hexValue(color[i]) < 0) valid = false;
    }

    if (valid) {
      color.toCharArray(ch.color, sizeof(ch.color));
    }
  }

  ch.type = type;
  ch.autoScale = param(url, "auto").toInt() ? 1 : 0;

  float gain = param(url, "gain").toFloat();
  float offset = param(url, "offset").toFloat();
  float ymin = param(url, "min").toFloat();
  float ymax = param(url, "max").toFloat();
  float beta = param(url, "beta").toFloat();
  float r25 = param(url, "r25").toFloat();
  float fixed = param(url, "rfixed").toFloat();

  if (isfinite(gain) && fabsf(gain) < 100000)
    ch.gain = gain;

  if (isfinite(offset) && fabsf(offset) < 100000)
    ch.offset = offset;

  if (isfinite(ymin) && isfinite(ymax) && ymax > ymin) {
    ch.yMin = ymin;
    ch.yMax = ymax;
  }

  if (beta >= 1000 && beta <= 10000)
    ch.beta = beta;

  if (r25 > 0 && r25 <= 1000000)
    ch.r25 = r25;

  if (fixed > 0 && fixed <= 1000000)
    ch.rFixed = fixed;

  // A configuracao fisica mudou:
  // reiniciamos o historico para nao reinterpretar
  // registos com uma configuracao diferente.
  if(layoutChanged){
    rebuildHistory();
    lastSampleMs = millis();
    acquire();
    restartTimedAcquisition();
  }
}

void calibrateNTC(const String &url) {
  int id = param(url, "id").toInt();

  if (id < 0 || id >= CHANNELS || channels[id].type != NTC || simulation) return;

  float reference = param(url, "temp").toFloat();

  if (!isfinite(reference) || reference < -100 || reference > 150)
    return;

  stopTimedAcquisition();
  uint16_t raw = analogRead(channelPins[id]);
  float current = temperature(id, raw);

  if (isfinite(current)) {
    channels[id].temperatureTrim +=
      reference - current;
    rebuildHistory();
  }
  restartTimedAcquisition();
}

// ---------- SERVIDOR WEB ----------
extern const char WEB_PAGE[];

void handleWeb() {
  WiFiClient c = server.available();
  if (!c) return;
  const uint32_t beganUs=micros();
  c.setTimeout(180);

  String request = c.readStringUntil('\n');
  int p = request.indexOf(' ');
  int q = request.indexOf(' ', p + 1);

  if (p < 0 || q < 0) {
    c.stop();
    return;
  }

  String url = request.substring(p + 1, q);
  const bool post=request.startsWith("POST ");
  if(post && url=="/api/wifi"){
    int contentLength=-1;
    for(uint8_t i=0;i<25;++i){
      String h=c.readStringUntil('\n');h.trim();
      if(h.length()==0)break;
      if(h.startsWith("Content-Length:")){
        contentLength=h.substring(15).toInt();
      }
    }
    if(contentLength<=0||contentLength>768){
      sendHttpError(c,"413 Payload Too Large","Formulario invalido ou demasiado longo");
    }else{
      // readBytes respeita o timeout e espera pelo corpo completo,
      // mesmo que o TCP entregue headers e dados em pacotes diferentes.
      char postBody[769] = {};
      c.setTimeout(1000);
      const size_t received=c.readBytes(postBody,(size_t)contentLength);
      if((int)received!=contentLength){
        sendHttpError(c,"400 Bad Request","Corpo incompleto");
      }else if(applyNetworkPost(String(postBody))){
        sendShortJson(c,"{\"ok\":true,\"rebootRequired\":true}");
      }else{
        sendHttpError(c,"403 Forbidden","Codigo admin invalido ou campos incorrectos");
      }
    }
  } else if(post){
    sendHttpError(c,"405 Method Not Allowed","Metodo nao suportado");
  }else if (url.startsWith("/api/status")) {
    sendStatus(c);

  } else if (url.startsWith("/api/diagnostics")) {
    sendDiagnostics(c);

  } else if(url.startsWith("/api/alarmlog")){
    sendAlarmLog(c);

  } else if(url.startsWith("/api/alarm?")){
    updateAlarm(url);sendControlAck(c);

  } else if(url.startsWith("/api/virtual?")){
    updateVirtual(url);sendControlAck(c);

  } else if(url.startsWith("/api/calpoint?")){
    if(simulation && param(url,"action")!="clear"){
      sendHttpError(c,"409 Conflict","Calibracao NTC exige aquisicao real");
    }else{
      updateCalibrationPoint(url);sendControlAck(c);
    }

  } else if(url.startsWith("/api/sd/status")){
    sendSdStatus(c);

  } else if(url.startsWith("/api/sd/list")){
    sendSdList(c);

  } else if(url.startsWith("/api/sd/download?")){
    sendSdDownload(c,url);

  } else if(url.startsWith("/api/sd/start")){
    if(sdStart())sendShortJson(c,"{\"ok\":true}");
    else sendHttpError(c,"503 Service Unavailable","Cartao nao disponivel ou sessao activa");

  } else if(url.startsWith("/api/sd/stop")){
    sdStop();sendShortJson(c,"{\"ok\":true}");

  } else if (url.startsWith("/api/bin")) {
    sendFastData(c, url);

  } else if (url.startsWith("/api/data")) {
    sendData(c, url);

  } else if (url.startsWith("/api/trigger?")) {
    consumeCaptureQueue();
    updateTrigger(url);
    sendControlAck(c,true);

  } else if (url.startsWith("/api/channel?")) {
    updateChannel(url);
    sendControlAck(c);

  } else if (url.startsWith("/api/rate?")) {
    uint32_t v = param(url, "ms").toInt();

    if (v == 100 || v == 250 || v == 500 || v == 1000 || v == 2000 || v == 5000 || v == 10000) {
      stopTimedAcquisition();
      consumeCaptureQueue();
      sampleMs = v;
      lastSampleMs = millis();
      restartTimedAcquisition();
    }

    sendControlAck(c);

  } else if (url.startsWith("/api/sim?")) {
    stopTimedAcquisition();
    simulation = param(url, "on").toInt() != 0;
    if (!simulation) {
      digitalPrevious[0] = (digitalRead(2) == LOW) ? 1 : 0;
      digitalPrevious[1] = (digitalRead(3) == LOW) ? 1 : 0;
    }
    rebuildHistory();
    acquire();
    restartTimedAcquisition();
    sendControlAck(c);

  } else if (url.startsWith("/api/matrix?")) {
    String vText=param(url,"id");
    if(vText.length()){
      int v=vText.toInt();
      if(v>=0 && v<CHANNELS && matrixChannel!=v){
        matrixChannel=v;
        matrixTrackedChannel=255;
      }
    }
    String wText=param(url,"window");
    if(wText.length()){
      uint32_t w=(uint32_t)wText.toInt();
      if((w==1000UL||w==3000UL||w==10000UL||w==30000UL) && matrixWindowMs!=w){
        matrixWindowMs=w;
        matrixTrackedChannel=255;
      }
    }
    sendControlAck(c);

  } else if (url.startsWith("/api/cal?")) {
    if(simulation){
      sendHttpError(c,"409 Conflict","Calibracao NTC exige aquisicao real");
    }else{
      calibrateNTC(url);sendControlAck(c);
    }

  } else if (url.startsWith("/api/time?")) {
    if (clockSource != 1) {
      uint32_t epoch = strtoul(
        param(url, "epoch").c_str(), nullptr, 10);

      if (epoch > 1700000000UL) {
        epochAnchor = epoch;
        millisAnchor = millis();
        clockSource = 2;
      }
    }

    sendControlAck(c);

  } else if (url.startsWith("/api/save")) {
    consumeCaptureQueue();
    saveSettings(); // A IRQ continua activa durante a gravacao EEPROM.
    consumeCaptureQueue();
    sendControlAck(c);

  } else {
    headers(c, "text/html; charset=utf-8");
    // HTML em blocos; permite desenhar a matriz e descarregar a FIFO.
    const size_t pageLength=strlen(WEB_PAGE);
    for(size_t start=0;start<pageLength;start+=384){
      const size_t length=min((size_t)384,pageLength-start);
      c.write((const uint8_t*)WEB_PAGE+start,length);
      serviceBackgroundDuringPage();
    }
  }

  // Conservamos flush() para garantir que o buffer da resposta sai
  // antes de fechar a ligacao; o diagnostico mede o custo real.
  c.flush();
  c.stop();
  uint32_t elapsedUs=micros()-beganUs;
  lastHttpUs=elapsedUs;
  maxHttpUs=max(maxHttpUs,elapsedUs);
  ++httpRequests;
}

// ---------- WI-FI ----------

void connectWiFi() {
  WiFi.setHostname("opendatalogger");

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ERRO: Modulo Wi-Fi indisponivel!");
    return;
  }

  // Primeiro tenta as redes configuradas
  for (uint8_t i = 0; i < 2; i++) {
    if (strlen(networkSettings.ssid[i]) == 0) continue;

    Serial.print("A tentar rede: ");
    Serial.println(networkSettings.ssid[i]);

    WiFi.begin(networkSettings.ssid[i], networkSettings.password[i]);

    uint32_t start = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < 8000UL) {
      delay(100);
      consumeCaptureQueue();
      serviceSimulatedEdges(millis());
      processDigitalAlarmEvents();
      serviceMatrix(millis());
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Modo Station activo");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("Gateway: ");
      Serial.println(WiFi.gatewayIP());
      Serial.print("DNS: ");
      Serial.println(WiFi.dnsIP());
      return;
    }
  }

  // Nenhuma rede disponivel: activar AP
  Serial.println("Nenhuma rede encontrada.");
  Serial.println("A iniciar Access Point...");

  WiFi.end();
  delay(500);

  int status = WiFi.beginAP(networkSettings.apName, networkSettings.apPassword);

  if (status == WL_AP_LISTENING) {
    usingAccessPoint = true;
    delay(1500);

    Serial.println("Access Point activo!");
    Serial.print("SSID: ");
    Serial.println(networkSettings.apName);
    Serial.print("Abrir: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("ERRO ao criar Access Point!");
  }
}

void syncClock() {
  unsigned long epoch = WiFi.getTime();

  if (epoch > 1700000000UL) {
    epochAnchor = epoch;
    millisAnchor = millis();
    clockSource = 1;
    Serial.println("Hora sincronizada via rede");
  } else {
    Serial.println(
      "NTP indisponivel; navegador pode definir hora");
  }
}

// ---------- ARDUINO ----------
void setup() {
  Serial.begin(115200);
  delay(1200);

  loadSettings();
  loadAdvancedSettings();
  rebuildHistory();

  analogReadResolution(12);
  matrix.begin();

  // Captura digital independente da taxa de registo (D2 e D3).
  digitalPrevious[0] = (digitalRead(2) == LOW) ? 1 : 0;
  digitalPrevious[1] = (digitalRead(3) == LOW) ? 1 : 0;
  attachInterrupt(digitalPinToInterrupt(2), digitalIsrD2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), digitalIsrD3, CHANGE);

  Serial.println("OpenDataLogger 1.0.1");

  // ADC e GPT arrancam ANTES da ligacao Wi-Fi (pode demorar segundos).
  acquire();
  lastSampleMs = millis();
  for(uint8_t n=0;n<activeCount;++n){
    if(activeIds[n]<6&&!simulation)(void)analogRead(channelPins[activeIds[n]]);
  }
  setupTimedAcquisition();
  connectWiFi();

  if (!usingAccessPoint &&
      WiFi.status() == WL_CONNECTED) {
    syncClock();
  }

  server.begin();
  sdSetup();  // O cartao e detectado em todos os arranques.
#if ODL_ENABLE_SD && ODL_SD_AUTOSTART
  if (sdReady) {
    if (sdStart()) Serial.println(F("SD: sessao iniciada automaticamente"));
    else Serial.println(F("SD: cartao detectado mas nao foi possivel iniciar sessao"));
  }
#endif

  Serial.print("Abrir http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  const uint32_t now=millis();
  if(previousLoopMs){
    lastLoopGapMs=(uint32_t)(now-previousLoopMs);
    maxLoopGapMs=max(maxLoopGapMs,lastLoopGapMs);
  }
  previousLoopMs=now;

  serviceSimulatedEdges(now);
  processDigitalAlarmEvents();
  servicePendingTrigger();
  if (acquisitionTimerReady) {
    consumeCaptureQueue();
  } else if (triggerState != 3 && (uint32_t)(now-lastSampleMs) >= sampleMs) {
    // Reserva se nao existir um temporizador GPT disponivel.
    uint32_t elapsed=now-lastSampleMs;
    uint32_t slots=elapsed/sampleMs;
    lastSampleLateMs=elapsed-sampleMs;
    maxSampleLateMs=max(maxSampleLateMs,lastSampleLateMs);
    if(slots>1) missedSlots+=(slots-1);
    lastSampleMs+=slots*sampleMs;
    acquire();
  }
  serviceMatrix(millis());
  sdService();
  // As respostas HTTP podem demorar; a ISR continua a encher o FIFO.
  handleWeb();
}
