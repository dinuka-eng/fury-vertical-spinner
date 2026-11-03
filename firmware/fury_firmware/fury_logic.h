// fury_logic.h
// Hardware-independent control logic for the Fury combat robot.
//
// Everything in this file is plain C++ with no Arduino/ESP32 dependency, so it
// can be unit-tested on a PC (see firmware/tests/test_logic.cpp). The .ino file
// only handles pins, interrupts and PWM output, and calls into this header.
//
// Units: pulse widths are in microseconds (us). Normalised stick / motor
// commands are integers in the range -1000..+1000.

#pragma once
#include <stdint.h>

namespace fury {

// ---------------------------------------------------------------- RC input
constexpr int32_t  kPulseNeutralUs   = 1500;
constexpr uint16_t kPulseMinValidUs  = 800;    // below this = no valid RC pulse
constexpr uint16_t kPulseMaxValidUs  = 2200;   // above this = no valid RC pulse
constexpr uint32_t kSignalTimeoutUs  = 100000; // no new pulse for 100 ms = link lost

constexpr int32_t  kDeadbandUs       = 30;     // +/- around neutral for drive sticks
constexpr int32_t  kNormMax          = 1000;

// ---------------------------------------------------------------- Weapon ESC
constexpr uint16_t kEscStopUs            = 1000;
constexpr uint16_t kEscMaxUs             = 2000;
constexpr uint16_t kWeaponIdleThresholdUs = 1050;  // at/below = "throttle at zero"
constexpr uint8_t  kArmHoldTicks         = 25;     // 25 ticks x 20 ms = 0.5 s at zero before arming
constexpr uint16_t kWeaponRampUpUsPerTick = 10;    // 1000 us of throttle takes 100 ticks = 2 s
// Cap for the weapon command. Keep this LOW (e.g. 1300) for first bench tests,
// then raise to kEscMaxUs only after the weapon has been tested in a test box.
constexpr uint16_t kWeaponMaxUs          = 2000;

// ---------------------------------------------------------------- Drive
constexpr int32_t  kDriveSlewPerTick     = 150;    // full reversal takes ~13 ticks (~0.27 s)

inline bool pulseValid(uint32_t us) {
  return us >= kPulseMinValidUs && us <= kPulseMaxValidUs;
}

// Convert a stick pulse (1000..2000 us) to -1000..+1000 with a deadband that is
// re-scaled so full stick still gives full output.
inline int32_t normalizeStick(uint16_t us) {
  int32_t d = static_cast<int32_t>(us) - kPulseNeutralUs;
  if (d > 500) d = 500;
  if (d < -500) d = -500;
  if (d > -kDeadbandUs && d < kDeadbandUs) return 0;
  const int32_t span = 500 - kDeadbandUs;
  if (d > 0) return (d - kDeadbandUs) * kNormMax / span;
  return (d + kDeadbandUs) * kNormMax / span;
}

// Differential ("tank") mixing. If either side exceeds full scale, both are
// scaled down together so the turn ratio is preserved (plain clipping would
// change the steering behaviour at full throttle).
inline void mixTank(int32_t throttle, int32_t steer, int32_t& left, int32_t& right) {
  int32_t l = throttle + steer;
  int32_t r = throttle - steer;
  int32_t al = l < 0 ? -l : l;
  int32_t ar = r < 0 ? -r : r;
  int32_t m = al > ar ? al : ar;
  if (m > kNormMax) {
    l = l * kNormMax / m;
    r = r * kNormMax / m;
  }
  left = l;
  right = r;
}

// Move 'current' toward 'target' by at most 'maxStep'.
inline int32_t slewToward(int32_t current, int32_t target, int32_t maxStep) {
  if (target > current + maxStep) return current + maxStep;
  if (target < current - maxStep) return current - maxStep;
  return target;
}

// ---------------------------------------------------------------- Weapon state machine
enum class WeaponMode : uint8_t { kDisarmed = 0, kArmed = 1 };

struct WeaponState {
  WeaponMode mode = WeaponMode::kDisarmed;
  uint8_t    lowTicks = 0;
  uint16_t   outputUs = kEscStopUs;
};

// Called once per control tick (20 ms). Returns the pulse width (us) to send to the ESC.
//
// Rules:
//  1. No valid RC signal  -> state reset to DISARMED, output = stop, immediately.
//  2. DISARMED            -> output = stop. Arms only after the weapon channel
//                            has been at zero throttle for kArmHoldTicks in a row.
//                            (So a weapon can never spin up just because the
//                            radio was switched on, or the link came back, with
//                            the stick already raised.)
//  3. ARMED               -> follow the stick, limited by kWeaponMaxUs.
//                            Throttle RISES at a limited rate (protects battery
//                            and ESC from a current spike) but FALLS immediately.
inline uint16_t weaponUpdate(WeaponState& s, bool signalValid, uint16_t pulseUs) {
  if (!signalValid) {
    s = WeaponState();
    return s.outputUs;
  }

  if (s.mode == WeaponMode::kDisarmed) {
    s.outputUs = kEscStopUs;
    if (pulseUs <= kWeaponIdleThresholdUs) {
      if (s.lowTicks < 255) s.lowTicks++;
      if (s.lowTicks >= kArmHoldTicks) s.mode = WeaponMode::kArmed;
    } else {
      s.lowTicks = 0;
    }
    return s.outputUs;
  }

  uint16_t target = pulseUs;
  if (target <= kWeaponIdleThresholdUs) target = kEscStopUs;
  if (target > kWeaponMaxUs) target = kWeaponMaxUs;

  if (target <= s.outputUs) {
    s.outputUs = target;  // ramp down: immediate
  } else {
    uint16_t room = static_cast<uint16_t>(target - s.outputUs);
    s.outputUs = static_cast<uint16_t>(
        s.outputUs + (room > kWeaponRampUpUsPerTick ? kWeaponRampUpUsPerTick : room));
  }
  return s.outputUs;
}

}  // namespace fury
