# System and firmware architecture

## 1. Signal flow (stick to motor)

1. **Operator** moves a stick. The MC7RE transmitter digitises it and sends it over 2.4 GHz.
2. **Receiver** outputs one servo-style PWM signal per channel: a pulse repeating about every 20 ms, 1000–2000 µs wide (1500 µs = centre).
3. **ESP32 interrupts** timestamp the rising and falling edge of each pulse; the width is `fall − rise`. The latest width and the time of the last pulse are stored per channel.
4. **Tasks (every 20 ms):**
   - *Weapon task (core 0)* checks the weapon channel is fresh and in range, runs the arming/ramp/failsafe state machine, and outputs a 1000–2000 µs, 50 Hz PWM signal to the ESC.
   - *Drive task (core 1)* checks the two stick channels, applies a deadband, mixes throttle and steering into left/right commands, limits the rate of change, and outputs 20 kHz PWM to the two IBT-2 H-bridges.
5. **Power stage:** the ESC converts the 1000–2000 µs command into three-phase drive for the brushless motor; each IBT-2 converts the PWM duty into an average motor voltage from the battery.

## 2. Why interrupts instead of `pulseIn()`

`pulseIn()` blocks the calling task until the pulse ends or times out (up to tens of milliseconds). Reading three channels one after another could stall a task for tens of milliseconds, which defeats the purpose of having a fast failsafe. Interrupt capture costs a few microseconds per edge and the tasks only read stored values.

## 3. Dual-core design — what it does and does not do

**Task layout**

| Task | Core | Priority | Period | Function |
|---|---|---|---|---|
| `weaponTask` | 0 | 3 | 20 ms | weapon failsafe, arming, ramp, ESC output |
| `driveTask` | 1 | 2 | 20 ms | mixing, slew limit, IBT-2 output |

**Benefits**
- A bug, long computation or future feature (telemetry, logging, sensors) in the drive task cannot delay the weapon task, because they run on different cores.
- Worst-case weapon reaction time is bounded: up to 100 ms for signal-timeout detection plus one 20 ms tick.
- Both tasks feed the task watchdog; if either stops, the chip resets.

**Limits (be honest in reviews and vivas)**
- The current workload is tiny. With interrupt-driven input, a single core could do the job; the dual-core layout is chosen for isolation and growth, not because the load demands it.
- Both tasks share one ESP32, one power supply and one receiver. A brown-out or receiver fault affects both, so the failsafe cannot rely on the ESP32 alone (see layering below).
- "Instantaneous" is not accurate. The ESC and weapon still take time to slow down; the weapon coasts unless the ESC actively brakes.

## 4. Failsafe layering

| # | Layer | Covers |
|---|---|---|
| 1 | Receiver failsafe (set in transmitter/receiver menu: weapon channel → 1000 µs) | link loss, even if the ESP32 misbehaves |
| 2 | Pulse validity (800–2200 µs) and freshness (<100 ms) check | no pulse, stuck line, garbage |
| 3 | Arming interlock (0.5 s at zero before arming; disarm on any invalid input) | power-up and link-return surprises |
| 4 | Task watchdog | firmware hang or crash |
| 5 | 10 kΩ pull-down on ESC signal; ESC stops when no valid signal | ESP32 reset, unplugged wire |
| 6 | Master switches / power isolation | everything else |

Layers 1, 5 and 6 are independent of the firmware by design.

## 5. Weapon state machine

```
              signal invalid (any time)
        ┌─────────────────────────────────────────┐
        ▼                                         │
   ┌──────────┐  throttle ≤1050 µs for 25 ticks ┌───────┐
   │ DISARMED │ ───────────────────────────────►│ ARMED │
   │ out=1000 │ ◄─── (counter resets if stick   │ ramp  │
   └──────────┘       goes high while disarmed)  └───────┘
```
- In DISARMED the output is always 1000 µs.
- In ARMED the output follows the stick, capped at `kWeaponMaxUs`, rising at most 10 µs per tick (2 s for full range) and falling immediately.

The logic lives in `firmware/fury_firmware/fury_logic.h` and is covered by `firmware/tests/test_logic.cpp`: power-up with stick high, arming hold time, ramp limit, immediate stop, link loss while spinning, link return with stick high, and the throttle cap.

## 6. Drive mixing

`left = throttle + steer`, `right = throttle − steer`. If either result exceeds ±1000, both are scaled down by the same factor so the turn ratio is preserved. A ±30 µs deadband around centre (re-scaled so full stick still gives full output) removes stick jitter. A slew limit of 150 units per tick protects the gearmotors and traction.

## 7. Known weak points / future work

- Replace the 10 µs-resolution ISR timing with the ESP32 RMT or MCPWM capture peripheral for hardware timestamps.
- Add current sensing on the weapon supply to detect stalls and cut power.
- Log pulse-validity events for post-match analysis.
- Verify the receiver's real failsafe behaviour on the bench and document it here.
