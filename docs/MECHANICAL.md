# Mechanical design

## 1. Envelope

| Item | Value |
|---|---|
| Robot mass | 10 kg total |
| Footprint | 45 × 45 cm (the competition limit assumed here is 18 × 18 in ≈ 45.7 cm — confirm in the rulebook) |
| Height | 12 cm maximum |
| Weapon | Asymmetrical vertical spinner, horizontal axis across the robot |

The 12 cm height limit is the dominant constraint: it caps the blade at about 10.5 cm tip-to-tip once floor clearance and the upright geometry are allowed for.

## 2. Weapon sizing (see `calculations/results.md`)

- **Speed:** 800 kV × 11.1 V ÷ 3 = **2960 RPM no-load**, tip speed ω·r = 310 × 0.0525 = **16.3 m/s**. Loaded RPM will be lower; 85 % is used as a planning assumption.
- **Mass that fits:** a single-tooth profile filling about half of a 10.5 cm disc weighs only about **0.4–0.7 kg** at 12–20 mm plate thickness. A 1.5 kg blade would need about 44 mm of steel at that profile, so **1.5 kg is not achievable in this envelope**. Take the real mass from the CAD model.
- **Energy:** roughly 15–60 J at 3:1 for 0.5–0.9 kg blades, depending on mass and shape; about 2.25× more at 2:1. Using the CAD moment of inertia in place of the shape-factor bounds is the next calculation to do.
- **Gearing trade-off:** lower reduction (2:1) gives more tip speed and energy but needs more motor torque and may reduce spin-up performance and ESC margin. Build the pulley set so the ratio can be changed (60 mm driven pulley with 20 mm or 30 mm drive pulley options).
- **Gyroscopic effect:** for these parameters the turning lift force is only a few newtons against a 98 N robot weight, so it is a minor effect here .

## 3. Layout

```
FRONT
  ┌──────────────────────────────┐
  │ fork ┌──────────────┐  fork  │   feeder forks scrape the floor at the corners
  │      │ blade pocket │        │   blade pocket ≥ 5–10 mm clear of the blade all round
  │      └──┬───────┬───┘        │
  │      upright  upright        │   2 × 10–12 mm 6061 aluminium uprights clamp the dead shaft
  │         weapon motor         │   motor behind the pocket, belt runs forward to the blade pulley
  │  ESC   ESP32   battery       │
  │ drive motors / wheels (rear) │   heavy items rearward to offset the front-mounted blade
  └──────────────────────────────┘
REAR
```

## 4. Components and design rules

| Part | Design note |
|---|---|
| Blade | AR500/Hardox or similar plate, single tooth, profile cut then heat-treat/finish as appropriate for the material; balance it so the tooth does not cause vibration (a counter-profile or mass offset on the opposite side) |
| Shaft | 10 mm hardened steel dead shaft or 12.9 shoulder bolt, clamped at both ends; the blade spins on bearings |
| Bearings | Flanged ball or needle-roller bearings pressed into the blade hub; check dynamic load rating against impact loads |
| Uprights | 10–12 mm 6061/7075 aluminium, bolted through the baseplate with M5/M6 grade 12.9 bolts and thread-lock |
| Belt | Urethane round belt; set tension so it slips before the motor stalls; carry spares |
| Pulleys | Drive pulley on motor shaft with grub screw + thread-lock (and a flat on the shaft); driven pulley bolted to the blade |
| Forks | Angled steel or aluminium plates on the front corners, ground-skimming; must not fold back into the wheels |
| Centre of gravity | Re-check with CAD: a front-heavy robot loses rear-wheel traction. Move battery and heavy electronics rearward |

## 5. Checks to do in CAD before cutting metal

1. Blade clearance all round (≥ 5–10 mm) including belt and pulley.
2. Total height ≤ 12 cm with tolerance margin.
3. Total mass ≤ 10 kg including the weapon (weigh parts as they arrive).
4. Centre-of-gravity position and wheel loading.
5. Belt path and pulley alignment; ability to retension and change the belt without disassembling the robot.
6. Access to the master switches and arming LED.
7. Blade lock / safety pin: a physical way to stop the blade from turning when the robot is being handled (typically required by event rules — check).
