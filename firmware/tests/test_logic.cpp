// Host-side unit tests for fury_logic.h
// Build and run:  g++ -std=c++14 -Wall -Wextra -I../fury_firmware test_logic.cpp -o test_logic && ./test_logic

#include <cstdio>
#include <cstdlib>
#include "fury_logic.h"

using namespace fury;

static int g_failures = 0;
#define CHECK(cond)                                                          \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::printf("FAIL  line %d: %s\n", __LINE__, #cond);                   \
      g_failures++;                                                          \
    }                                                                        \
  } while (0)

static void test_pulse_valid() {
  CHECK(!pulseValid(0));
  CHECK(!pulseValid(799));
  CHECK(pulseValid(800));
  CHECK(pulseValid(1500));
  CHECK(pulseValid(2200));
  CHECK(!pulseValid(2201));
}

static void test_normalize() {
  CHECK(normalizeStick(1500) == 0);
  CHECK(normalizeStick(1500 + kDeadbandUs - 1) == 0);
  CHECK(normalizeStick(1500 - kDeadbandUs + 1) == 0);
  CHECK(normalizeStick(2000) == 1000);
  CHECK(normalizeStick(1000) == -1000);
  CHECK(normalizeStick(2100) == 1000);   // clamped
  CHECK(normalizeStick(900) == -1000);   // clamped
  CHECK(normalizeStick(1600) > 0 && normalizeStick(1600) < 1000);
}

static void test_mix() {
  int32_t l, r;
  mixTank(0, 0, l, r);       CHECK(l == 0 && r == 0);
  mixTank(500, 0, l, r);     CHECK(l == 500 && r == 500);
  mixTank(0, 500, l, r);     CHECK(l == 500 && r == -500);   // spin on the spot
  mixTank(1000, 1000, l, r);                                  // would be 2000 / 0
  CHECK(l == 1000 && r == 0);                                 // scaled, ratio kept
  mixTank(-1000, 500, l, r);                                  // -500 / -1500 -> scaled
  CHECK(l >= -1000 && r >= -1000 && l > r);
}

static void test_slew() {
  CHECK(slewToward(0, 1000, 150) == 150);
  CHECK(slewToward(900, 1000, 150) == 1000);
  CHECK(slewToward(0, -1000, 150) == -150);
  CHECK(slewToward(100, 100, 150) == 100);
}

static void test_weapon_failsafe_and_arming() {
  WeaponState s;

  // Power-up with the stick already high: must NOT arm, must output stop.
  for (int i = 0; i < 100; i++) CHECK(weaponUpdate(s, true, 2000) == kEscStopUs);
  CHECK(s.mode == WeaponMode::kDisarmed);

  // Stick to zero, but not long enough: still disarmed.
  for (int i = 0; i < kArmHoldTicks - 1; i++) weaponUpdate(s, true, 1000);
  CHECK(s.mode == WeaponMode::kDisarmed);
  // One more tick arms.
  weaponUpdate(s, true, 1000);
  CHECK(s.mode == WeaponMode::kArmed);

  // Raising the stick: output ramps, limited per tick.
  uint16_t prev = weaponUpdate(s, true, 2000);
  CHECK(prev == kEscStopUs + kWeaponRampUpUsPerTick);
  for (int i = 0; i < 200; i++) {
    uint16_t out = weaponUpdate(s, true, 2000);
    CHECK(out >= prev);
    CHECK(out - prev <= kWeaponRampUpUsPerTick);
    prev = out;
  }
  CHECK(prev == 2000);

  // Stick down: immediate stop.
  CHECK(weaponUpdate(s, true, 1000) == kEscStopUs);

  // Run it up again, then lose the link: immediate stop AND disarm.
  for (int i = 0; i < 200; i++) weaponUpdate(s, true, 2000);
  CHECK(s.outputUs == 2000);
  CHECK(weaponUpdate(s, false, 0) == kEscStopUs);
  CHECK(s.mode == WeaponMode::kDisarmed);

  // Link returns with the stick still high: must stay stopped.
  for (int i = 0; i < 100; i++) CHECK(weaponUpdate(s, true, 2000) == kEscStopUs);
}

static void test_weapon_max_cap() {
  // kWeaponMaxUs is a compile-time constant; verify the output never exceeds it.
  WeaponState s;
  for (int i = 0; i < 30; i++) weaponUpdate(s, true, 1000);  // arm
  for (int i = 0; i < 300; i++) CHECK(weaponUpdate(s, true, 2200) <= kWeaponMaxUs);
}

int main() {
  test_pulse_valid();
  test_normalize();
  test_mix();
  test_slew();
  test_weapon_failsafe_and_arming();
  test_weapon_max_cap();
  if (g_failures == 0) {
    std::printf("All tests passed.\n");
    return 0;
  }
  std::printf("%d check(s) failed.\n", g_failures);
  return 1;
}
