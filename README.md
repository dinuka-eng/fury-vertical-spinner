# Fury Vertical Spinner — Weapon System Design

Engineering design package for a belt-driven asymmetrical vertical spinner on **Fury**, a 10 kg, 45 × 45 cm, 12 cm-tall combat robot, for the **CY-BOTS'26** competition: system architecture, ESP32 firmware, calculations, electrical design, mechanical design and a staged test plan.

The Fury chassis and drive system are in a separate repository: [Xcode1234/battlebot](https://github.com/Xcode1234/battlebot). This repository covers the weapon system and the shared control electronics and firmware.

**Author:** Dinuka Senevirathne — BEng (Hons) Electrical and Electronic Engineering, ICBT Kandy Campus / LJMU

> **Note on figures:** speeds, energies and masses in this repository are design calculations. Measured values should replace them as the test plan in [docs/TEST_PLAN.md](docs/TEST_PLAN.md) is carried out.

---

## Special features

### 1. Dual-core FreeRTOS task separation
A single ESP32 runs two pinned tasks: a **weapon/safety task on core 0** (higher priority) and a **drive-mixing task on core 1**. A slow or stalled drive loop cannot delay the weapon failsafe, and both tasks are registered with the ESP32 task watchdog so a hang causes a reset rather than a frozen output. RC pulses are captured by **interrupts**, not blocking `pulseIn()` calls, so neither task ever waits on the receiver.

The layout reduces coupling between the two functions and gives a bounded worst-case response (20 ms control period). Both cores still share one supply and one receiver, which is why the failsafe is layered (feature 4). Details: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

### 2. Asymmetrical single-tooth "bite" geometry
One tooth on a 10.5 cm blade lets an opponent's chassis travel deeper into the weapon's path between hits than a symmetrical bar would, which favours a hit that launches rather than one that grinds. Ground-scraping feeder forks guide the opponent into the strike zone.

Design values (3:1 reduction, 800 kV motor, 3S): about 2960 RPM no-load and **16.3 m/s tip speed** (about 13.8 m/s loaded estimate). A blade that fits the 10.5 cm profile weighs roughly 0.5–0.9 kg; see [docs/MECHANICAL.md](docs/MECHANICAL.md) and [calculations/results.md](calculations/results.md). A 2:1 reduction (about 24 m/s, roughly 2.25× the energy) is the main alternative to evaluate.

### 3. Belt drive as a torque limiter
A urethane round belt between motor and weapon limits the peak torque transmitted when the blade is suddenly arrested, reducing shock loading of the motor shaft and ESC. Slip torque depends on belt tension and wrap angle and is tuned and measured on the assembly; a belt does not protect against a sustained stall, so short, controlled spin-ups still matter.

### 4. Layered failsafe and power architecture
| Layer | Mechanism |
|---|---|
| Receiver | Failsafe configured so the weapon channel goes to 1000 µs on link loss |
| Firmware | Pulse-validity check + 100 ms freshness timeout → weapon output forced to stop and **disarmed** |
| Firmware | **Arming interlock:** weapon only arms after the throttle has been at zero for 0.5 s; it cannot spin up because the radio was switched on or the link returned with the stick raised |
| Firmware | Throttle ramp-up limited (2 s to full), ramp-down immediate |
| Firmware | Drive failsafe zeroes PWM and drops the IBT-2 enable line |
| Firmware | Task watchdog resets the ESP32 if either task stops |
| Hardware | Pull-down on the ESC signal line; ESC red (BEC) wire removed; ESP32 and receiver on a dedicated regulated supply |
| Hardware | Independent weapon and drive master switches, external arming LED |

Details: [docs/ELECTRICAL.md](docs/ELECTRICAL.md), [docs/SAFETY_AND_COMPLIANCE.md](docs/SAFETY_AND_COMPLIANCE.md).

---

## System overview

```
 MC7RE transmitter ──2.4 GHz──► MC7RE receiver
                                   │ CH3 weapon   CH2 throttle   CH1 steering   (PWM, 1000–2000 µs)
                                   ▼
                            ┌────────────────── ESP32 ──────────────────┐
                            │  GPIO interrupts capture pulse widths      │
                            │  Core 0: weaponTask  (arm/failsafe/ramp)   │
                            │  Core 1: driveTask   (deadband/mix/slew)   │
                            └───────┬─────────────────────┬─────────────┘
                              50 Hz PWM                20 kHz PWM + EN
                                    ▼                     ▼
                              80 A brushless ESC     2 × IBT-2 H-bridge
                                    ▼                     ▼
                          D4250 800 kV motor      2 × brushed drive motors
                                    ▼ 3:1 urethane belt
                           asymmetrical vertical spinner
```

| Subsystem | Specification |
|---|---|
| Robot | 10 kg, 45 × 45 cm, 12 cm max height, 3S (11.1 V) LiPo |
| Weapon | Belt-driven asymmetrical vertical spinner, 10.5 cm blade, 10 mm dead shaft, aluminium uprights |
| Weapon motor / ESC | D4250 800 kV outrunner / one 80 A brushless ESC (3S capable) |
| Drive | Brushed motors, 2 × IBT-2 (BTS7960) drivers, differential mixing |
| Controller | ESP32 DevKit, dual-core FreeRTOS |
| Radio | MC7RE 2.4 GHz transmitter/receiver |

---

## Repository layout

```
fury-vertical-spinner/
├── README.md
├── firmware/
│   ├── fury_firmware/
│   │   ├── fury_firmware.ino     ESP32 application (pins, interrupts, PWM, tasks)
│   │   └── fury_logic.h          hardware-independent logic (arming, mixing, failsafe)
│   └── tests/test_logic.cpp      PC unit tests for fury_logic.h
├── calculations/
│   ├── weapon_calcs.py           speed / mass / energy / spin-up / gyro estimates
│   └── results.md                output of the script
├── docs/
│   ├── ARCHITECTURE.md          signal flow, firmware design, dual-core discussion
│   ├── ELECTRICAL.md            pin map, wiring, power, failsafe layers
│   ├── MECHANICAL.md            envelope, blade sizing, integration, CoG
│   ├── PARTS_LIST.md            parts and weight tracking
│   ├── TEST_PLAN.md             staged bench → test-box → arena tests
│   └── SAFETY_AND_COMPLIANCE.md safe-handling rules and rulebook checklist
├── mechanical/                   CAD, drawings
└── electrical/                   schematics, KiCad/Proteus sources
```

## Build and test

**Unit tests (PC, no hardware):**
```bash
cd firmware/tests
g++ -std=c++14 -Wall -Wextra -I../fury_firmware test_logic.cpp -o test_logic && ./test_logic
```

**Calculations:**
```bash
python3 calculations/weapon_calcs.py > calculations/results.md
```

**Firmware:** open `firmware/fury_firmware/fury_firmware.ino` in Arduino IDE with the **esp32 by Espressif** board package (2.x or 3.x). No extra libraries are needed. Before any hardware test, read [docs/TEST_PLAN.md](docs/TEST_PLAN.md): the first tests are done with the weapon belt removed and the blade off the shaft.

## Notes

- With a 10.5 cm blade on 3S, stored energy is roughly 15–60 J at 3:1 for 0.5–0.9 kg blades (see results). This follows from the 12 cm height limit and the 11.1 V supply; the reduction ratio and blade mass are the main design levers.
- Competition limits (tip speed, spin-down time, switches, failsafe) must be checked against the official CY-BOTS'26 rulebook; use the checklist in [docs/SAFETY_AND_COMPLIANCE.md](docs/SAFETY_AND_COMPLIANCE.md).

## Safety

A vertical spinner stores enough energy to cause serious injury. Never spin the weapon outside a rated test box, never work on the robot with a battery connected, and follow [docs/SAFETY_AND_COMPLIANCE.md](docs/SAFETY_AND_COMPLIANCE.md). Provided for documentation and education, without warranty.

## License

MIT — see [LICENSE](LICENSE).
