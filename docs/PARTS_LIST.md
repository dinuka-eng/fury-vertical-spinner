# Parts list

## Weapon system

| # | Item | Specification | Qty | Notes |
|---|---|---|---|---|
| 1 | Weapon motor | D4250 800 kV brushless outrunner | 1 | Check max current and shaft size in the datasheet |
| 2 | Weapon ESC | 80 A brushless ESC, 3S capable, forward-only | 1 | **One ESC** runs the weapon motor; confirm continuous rating and configuration options |
| 3 | Blade | AR500 / Hardox plate, single-tooth profile, about 0.5–0.9 kg | 1 | Mass to be taken from CAD |
| 4 | Dead shaft and bearings | 10 mm hardened shaft (or grade 12.9 shoulder bolt), 2 × flanged bearings, shaft collars | 1 set | |
| 5 | Pulleys and belt | 20–30 mm drive pulley, 60 mm driven pulley, urethane round belt | 1 set | Keep a spare belt |
| 6 | Uprights | 10–12 mm 6061/7075 aluminium A-frames | 2 | Bolted to baseplate with M5/M6 grade 12.9 bolts |
| 7 | Feeder forks | Angled steel or aluminium plates | 2 | |

## Control and power

| Item | Note |
|---|---|
| ESP32 DevKit | single controller, dual-core FreeRTOS |
| MC7RE transmitter and receiver | 2.4 GHz, 3 channels used |
| 2 × IBT-2 (BTS7960) H-bridge | drive motors (brushed) |
| 3S LiPo (11.1 V), high discharge | check real continuous rating |
| 5 V DC/DC buck converter, ≥ 3 A | dedicated logic supply |
| Master switches (×2), fuses, XT60 connectors, 12 AWG wire | rated for the current they carry |
| 10 kΩ pull-down resistors (ESC signal, drive enable), LED + resistor | |
| Polycarbonate test box | for weapon testing |

## Weight tracking
Record the mass of each part in this table as the design is finalised; the robot total must stay at or below 10 kg.

| Part | Mass (g) |
|---|---|
| Blade | |
| Motor | |
| ESC | |
| Uprights and shaft assembly | |
| Pulleys and belt | |
| Feeder forks | |
