# Calculation results

Inputs: 800 kV, 11.1 V, blade diameter 10.5 cm (r = 5.25 cm), load factor 0.85.

## 1. Weapon speed and tip speed

| Reduction | No-load RPM | Loaded RPM (est.) | No-load rad/s | Tip speed no-load (m/s) | Tip speed loaded (m/s) |
|---|---|---|---|---|---|
| 2.0:1 | 4440 | 3774 | 465 | 24.4 | 20.7 |
| 2.5:1 | 3552 | 3019 | 372 | 19.5 | 16.6 |
| 3.0:1 | 2960 | 2516 | 310 | 16.3 | 13.8 |

Tip speed = omega x r. All values are far below the 113 m/s limit assumed for this design (verify against the official rulebook).

## 2. What mass actually fits in a 10.5 cm profile?

Single-tooth profile assumed to fill 50% of a full disc.

| Plate thickness (mm) | Blade mass (kg) |
|---|---|
| 10 | 0.34 |
| 12 | 0.41 |
| 15 | 0.51 |
| 20 | 0.68 |

A 1.5 kg blade would need about 44 mm of steel at this profile - not practical in this envelope. Design mass should be taken from CAD; plan on roughly 0.5 - 0.9 kg.

## 3. Stored kinetic energy  E = 1/2 I w^2  (loaded RPM)

**Reduction 2.0:1 (3774 RPM loaded)**

| Mass (kg) | E (J), bar (k=0.67) | E (J), disc (k=1.0) |
|---|---|---|
| 0.50 | 36 | 54 |
| 0.75 | 54 | 81 |
| 1.00 | 72 | 108 |
| 1.50 | 108 | 161 |

**Reduction 2.5:1 (3019 RPM loaded)**

| Mass (kg) | E (J), bar (k=0.67) | E (J), disc (k=1.0) |
|---|---|---|
| 0.50 | 23 | 34 |
| 0.75 | 35 | 52 |
| 1.00 | 46 | 69 |
| 1.50 | 69 | 103 |

**Reduction 3.0:1 (2516 RPM loaded)**

| Mass (kg) | E (J), bar (k=0.67) | E (J), disc (k=1.0) |
|---|---|---|
| 0.50 | 16 | 24 |
| 0.75 | 24 | 36 |
| 1.00 | 32 | 48 |
| 1.50 | 48 | 72 |

Energy scales with mass and with RPM squared. Moving from 3:1 to 2:1 raises RPM by 1.5x and energy by 2.25x for the same blade, but needs more motor torque to spin it up - this is the main design trade-off to test.

## 4. Spin-up time (energy / average power)

Assumed mechanical power into the weapon: 441 W (10.5 V x 60 A x 0.7). Lower bound only - ignores motor torque limits, voltage sag and belt slip. Measure this on the real build.

| Reduction | Mass (kg) | E (J), disc bound | Min spin-up time (s) |
|---|---|---|---|
| 2.0:1 | 0.50 | 54 | 0.12 |
| 2.0:1 | 0.75 | 81 | 0.18 |
| 2.5:1 | 0.50 | 34 | 0.08 |
| 2.5:1 | 0.75 | 52 | 0.12 |
| 3.0:1 | 0.50 | 24 | 0.05 |
| 3.0:1 | 0.75 | 36 | 0.08 |

## 5. Gyroscopic effect when turning

For m = 0.75 kg, 3.0:1, turn rate 3.0 rad/s: gyro torque = I w Omega = 0.96 N m; as a force across a 40 cm track = 2.4 N, against a robot weight of 98 N. At this scale the effect is small (a few percent); it matters more for larger, faster weapons.

## 6. Belt drive geometry (reference)

Drive pulley 20 mm : driven pulley 60 mm gives 3:1. Belt slip torque is set by belt tension and pulley wrap angle and must be measured on the build - it limits peak torque to the motor but does not by itself protect it in a sustained stall.

