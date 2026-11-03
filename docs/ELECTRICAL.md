# Electrical design

> Verify every connection against the datasheets of the parts you use before applying power.

## 1. Pin map (ESP32 DevKit)

| Function | GPIO | Direction | Connects to |
|---|---|---|---|
| Weapon channel in | 19 | in | Receiver CH3 signal |
| Throttle in | 21 | in | Receiver CH2 signal |
| Steering in | 22 | in | Receiver CH1 signal |
| Weapon ESC signal | 18 | out (50 Hz PWM) | ESC signal wire (+ 10 kΩ pull-down to GND) |
| Left IBT-2 RPWM/LPWM | 33 / 32 | out (20 kHz PWM) | left driver |
| Right IBT-2 RPWM/LPWM | 26 / 25 | out (20 kHz PWM) | right driver |
| Drive enable | 27 | out | R_EN + L_EN of **both** IBT-2 boards (+ pull-down) |
| Weapon-armed LED | 23 | out | LED + series resistor to GND |

None of these are the ESP32 strapping or flash pins (0, 2, 5, 12, 15, 6–11), so they do not interfere with boot.

> Note on IBT-2 naming: on the BTS7960 board, one PWM input drives the motor in one direction and the other in the other. If a wheel runs the wrong way, swap the motor wires or swap the two pin definitions in the `.ino`; do not change the logic.

## 2. Power architecture

```
                    ┌── fuse ── Main switch ──┬── DC/DC buck (5 V, ≥3 A) ──► ESP32 VIN, receiver, IBT-2 logic Vcc
 3S LiPo (11.1 V) ──┤                         ├── fuse ── Drive switch ─────► IBT-2 ×2 (B+ / B−) ─► drive motors
 (high-C pack)      │                         └── Weapon switch ────────────► 80 A ESC ─► D4250 motor
                    └── common ground (star point at battery)
```
Points to verify when you build:
- **Logic supply:** the ESP32 and receiver run from their own regulated 5 V buck, *not* from the ESC's BEC. A motor-current dip must not brown out the ESP32. Add 470–1000 µF near the buck output and a ferrite/TVS if resets appear.
- **ESC BEC:** remove the **red (+5 V) wire** from the ESC's 3-pin signal connector (tape it off). Keep signal (white/orange) and ground (black/brown). This prevents two 5 V supplies being tied together.
- **Splitting power at the XT60 does not by itself stop voltage sag** — everything shares one battery. What protects the logic from sag is the dedicated buck converter and bulk capacitance.
- **Switches** must be rated for the current they carry (weapon: ESC peak; use a proper high-current switch or removable link, not a small toggle). Check the rulebook for switch type, location and number.
- **Fusing:** fuse the battery output close to the pack; size to the wire and the ESC rating (fuse selection to be done with the final wire gauge, 12 AWG suggested for the weapon and main runs).
- **Battery:** high-discharge 3S LiPo; the weapon ESC plus drive can draw well over 100 A in short bursts, so check the pack's real continuous rating, not just its C label.
- **Wiring:** twist and shorten motor phase wires, route signal wires away from motor and battery wires, strain-relieve every connector, and use thread-locked or locking connectors (impacts vibrate things loose).

## 3. Weapon ESC

- One 80 A brushless ESC, rated for 3S operation at the required current. Confirm its continuous current rating, voltage range and configuration options in its datasheet.
- Configure: forward-only rotation, **brake off** (coasting is the safe default; confirm rule requirements), throttle range 1000–2000 µs, low-voltage cutoff appropriate for LiPo.
- Check that the ESC treats a **loss of signal** by stopping the motor, and confirm this on the bench (TEST_PLAN, step 4).
- Verify motor direction **with the blade and belt removed** and reverse by swapping two phase wires if needed. The blade must strike *upward* into the opponent, in the direction chosen in the CAD design.

## 4. Receiver

- MC7RE receiver channels: CH1 steering, CH2 throttle, CH3 weapon (set on the transmitter; the exact assignment must be confirmed on your unit).
- Configure the receiver/transmitter **failsafe** so that the weapon channel goes to 1000 µs (and throttle/steering to centre) on signal loss. This is layer 1 of the failsafe and does not depend on the ESP32.
- Receiver output is typically 3.3–5 V logic; the ESP32 inputs are 3.3 V. If a receiver outputs 5 V pulses, add a resistor divider (e.g. 2.2 kΩ / 3.3 kΩ) or level shifter before the GPIO — check your unit with a multimeter or scope first.

## 5. IBT-2 (BTS7960) drive boards

- Logic Vcc 5 V from the buck; motor supply from the drive switch.
- L_EN and R_EN of both boards are joined and driven from GPIO 27 (with a pull-down), so the firmware can disable the bridges on failsafe and they stay disabled while the ESP32 boots.
- 20 kHz PWM is above the audible range and within the board's PWM capability; if the boards heat unusually, lower the frequency in `DRIVE_FREQ_HZ`.
- Keep motor leads short and add a flyback-style TVS or capacitor across each driver supply if noise is observed.

## 6. Grounding and noise

One common ground between battery, ESC, IBT-2s, buck, ESP32 and receiver. Join at a single star point near the battery rather than daisy-chaining motor-current ground through the logic ground. Keep the receiver antenna away from the weapon motor and carbon-fibre or metal sheets.

## 7. Schematics

Place KiCad/Proteus sources and PDF exports in `electrical/`. Suggested sheets: (1) power distribution, (2) ESP32 and receiver connections, (3) drive and weapon drivers.
