// fury_firmware.ino
// Fury 10 kg combat robot - ESP32 firmware (single board, dual-core FreeRTOS)
//
// Target:  ESP32 DevKit (Xtensa dual-core), Arduino-ESP32 core 2.x or 3.x
// Author:  Dinuka Senevirathne
// Validation: control logic (fury_logic.h) has PC unit tests. Follow docs/TEST_PLAN.md
//          for hardware bring-up, with the weapon belt REMOVED for the first tests.
//
// What runs where
//   Core 0, priority 3  : weaponTask  - reads weapon channel, runs arming/failsafe
//                                       state machine, drives the weapon ESC.
//   Core 1, priority 2  : driveTask   - reads X/Y sticks, mixes, drives 2x IBT-2.
//   RC pulses are captured by GPIO interrupts (no blocking pulseIn()).
//   Both tasks are registered with the task watchdog; if either stops running,
//   the ESP32 resets and the ESC/IBT-2 inputs fall back to their pull-down state.
//
// Wiring: see docs/ELECTRICAL.md

#include <Arduino.h>
#include "esp_task_wdt.h"
#include "driver/gpio.h"
#include "fury_logic.h"

// ------------------------------------------------------------------ pins
constexpr int PIN_RX_WEAPON = 19;  // receiver CH3
constexpr int PIN_RX_Y      = 21;  // receiver CH2 (throttle)
constexpr int PIN_RX_X      = 22;  // receiver CH1 (steering)

constexpr int PIN_ESC       = 18;  // weapon ESC signal

constexpr int PIN_L_LPWM    = 32;  // left  IBT-2: forward
constexpr int PIN_L_RPWM    = 33;  // left  IBT-2: reverse
constexpr int PIN_R_LPWM    = 25;  // right IBT-2: forward
constexpr int PIN_R_RPWM    = 26;  // right IBT-2: reverse
constexpr int PIN_DRIVE_EN  = 27;  // wired to R_EN + L_EN of BOTH IBT-2 boards

constexpr int PIN_ARMED_LED = 23;  // external LED (via resistor): ON = weapon armed

// ------------------------------------------------------------------ PWM setup
// LEDC channels: the ESC (50 Hz) and the drive outputs (20 kHz) must not share
// a LEDC timer. On core 2.x channels 0/1 share a timer, 2/3 share one, etc.
constexpr uint8_t  CH_ESC   = 0;                 // timer 0 (channel 1 left unused)
constexpr uint8_t  CH_L_F   = 2, CH_L_R = 3;     // timer 1
constexpr uint8_t  CH_R_F   = 4, CH_R_R = 5;     // timer 2
constexpr uint32_t ESC_FREQ_HZ   = 50;
constexpr uint8_t  ESC_BITS      = 14;           // 0..16383 over a 20 000 us period
constexpr uint32_t DRIVE_FREQ_HZ = 20000;
constexpr uint8_t  DRIVE_BITS    = 8;            // 0..255

static void pwmBegin(int pin, uint8_t ch, uint32_t freq, uint8_t bits) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)ch;
  ledcAttach(pin, freq, bits);
#else
  ledcSetup(ch, freq, bits);
  ledcAttachPin(pin, ch);
#endif
}

static void pwmWrite(int pin, uint8_t ch, uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)ch;
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(ch, duty);
#endif
}

static void escWriteUs(uint16_t us) {
  pwmWrite(PIN_ESC, CH_ESC, (static_cast<uint32_t>(us) * ((1u << ESC_BITS) - 1)) / 20000u);
}

// One H-bridge channel pair. cmd: -1000..+1000
static void setMotor(int pinF, uint8_t chF, int pinR, uint8_t chR, int32_t cmd) {
  uint32_t duty = static_cast<uint32_t>(cmd < 0 ? -cmd : cmd) * 255u / 1000u;
  if (cmd > 0)      { pwmWrite(pinF, chF, duty); pwmWrite(pinR, chR, 0); }
  else if (cmd < 0) { pwmWrite(pinF, chF, 0);    pwmWrite(pinR, chR, duty); }
  else              { pwmWrite(pinF, chF, 0);    pwmWrite(pinR, chR, 0); }
}

static void driveStop() {
  setMotor(PIN_L_LPWM, CH_L_F, PIN_L_RPWM, CH_L_R, 0);
  setMotor(PIN_R_LPWM, CH_R_F, PIN_R_RPWM, CH_R_R, 0);
  digitalWrite(PIN_DRIVE_EN, LOW);  // also disable the H-bridges
}

// ------------------------------------------------------------------ RC pulse capture (interrupt based)
struct PulseChannel {
  uint8_t pin;
  volatile uint32_t riseUs;
  volatile uint16_t widthUs;
  volatile uint32_t lastEdgeUs;  // time of last falling edge (0 = never seen a pulse)
};

PulseChannel chWeapon = {PIN_RX_WEAPON, 0, 0, 0};
PulseChannel chY      = {PIN_RX_Y, 0, 0, 0};
PulseChannel chX      = {PIN_RX_X, 0, 0, 0};

static void IRAM_ATTR onEdge(void* arg) {
  PulseChannel* c = static_cast<PulseChannel*>(arg);
  uint32_t now = micros();
  if (gpio_get_level(static_cast<gpio_num_t>(c->pin))) {
    c->riseUs = now;
  } else {
    uint32_t w = now - c->riseUs;
    if (w < 65535u) c->widthUs = static_cast<uint16_t>(w);
    c->lastEdgeUs = now ? now : 1;
  }
}

// Returns true only if a fresh, in-range pulse has been seen.
static bool readChannel(const PulseChannel& c, uint16_t& us) {
  uint32_t last = c.lastEdgeUs;
  uint16_t w = c.widthUs;
  if (last == 0) return false;
  if (static_cast<uint32_t>(micros() - last) > fury::kSignalTimeoutUs) return false;
  if (!fury::pulseValid(w)) return false;
  us = w;
  return true;
}

// ------------------------------------------------------------------ tasks
static void weaponTask(void*) {
  esp_task_wdt_add(NULL);
  fury::WeaponState state;
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    uint16_t pulse = 0;
    bool ok = readChannel(chWeapon, pulse);
    uint16_t out = fury::weaponUpdate(state, ok, pulse);
    escWriteUs(out);
    digitalWrite(PIN_ARMED_LED, state.mode == fury::WeaponMode::kArmed ? HIGH : LOW);
    esp_task_wdt_reset();
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(20));  // 50 Hz
  }
}

static void driveTask(void*) {
  esp_task_wdt_add(NULL);
  int32_t left = 0, right = 0;
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    uint16_t pY = 0, pX = 0;
    bool ok = readChannel(chY, pY) && readChannel(chX, pX);
    if (!ok) {
      left = right = 0;
      driveStop();  // failsafe: immediate stop, no ramp
    } else {
      digitalWrite(PIN_DRIVE_EN, HIGH);
      int32_t tl, tr;
      fury::mixTank(fury::normalizeStick(pY), fury::normalizeStick(pX), tl, tr);
      left  = fury::slewToward(left,  tl, fury::kDriveSlewPerTick);
      right = fury::slewToward(right, tr, fury::kDriveSlewPerTick);
      setMotor(PIN_L_LPWM, CH_L_F, PIN_L_RPWM, CH_L_R, left);
      setMotor(PIN_R_LPWM, CH_R_F, PIN_R_RPWM, CH_R_R, right);
    }
    esp_task_wdt_reset();
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(20));
  }
}

// ------------------------------------------------------------------ setup
static void watchdogInit() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t cfg;
  cfg.timeout_ms = 500;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic = true;
  esp_task_wdt_reconfigure(&cfg);
#else
  esp_task_wdt_init(1, true);  // 1 s, panic (reset) on timeout
#endif
}

void setup() {
  // Safe output states first, before anything else happens.
  pinMode(PIN_DRIVE_EN, OUTPUT);  digitalWrite(PIN_DRIVE_EN, LOW);
  pinMode(PIN_ARMED_LED, OUTPUT); digitalWrite(PIN_ARMED_LED, LOW);

  pwmBegin(PIN_ESC,    CH_ESC, ESC_FREQ_HZ, ESC_BITS);
  escWriteUs(fury::kEscStopUs);   // start sending "stop" immediately

  pwmBegin(PIN_L_LPWM, CH_L_F, DRIVE_FREQ_HZ, DRIVE_BITS);
  pwmBegin(PIN_L_RPWM, CH_L_R, DRIVE_FREQ_HZ, DRIVE_BITS);
  pwmBegin(PIN_R_LPWM, CH_R_F, DRIVE_FREQ_HZ, DRIVE_BITS);
  pwmBegin(PIN_R_RPWM, CH_R_R, DRIVE_FREQ_HZ, DRIVE_BITS);
  driveStop();

  pinMode(PIN_RX_WEAPON, INPUT);
  pinMode(PIN_RX_Y, INPUT);
  pinMode(PIN_RX_X, INPUT);
  attachInterruptArg(PIN_RX_WEAPON, onEdge, &chWeapon, CHANGE);
  attachInterruptArg(PIN_RX_Y,      onEdge, &chY,      CHANGE);
  attachInterruptArg(PIN_RX_X,      onEdge, &chX,      CHANGE);

  watchdogInit();

  // Weapon/safety task on core 0 (higher priority), drive task on core 1.
  xTaskCreatePinnedToCore(weaponTask, "weapon", 4096, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(driveTask,  "drive",  4096, NULL, 2, NULL, 1);
}

void loop() {
  vTaskDelete(NULL);  // all work happens in the two tasks
}
