# Safety and compliance

## 1. Safe-handling rules

1. Never spin the weapon outside a rated polycarbonate test box. Never stand in line with the blade.
2. Battery disconnected for all wiring work; connect it last and only when the area is clear.
3. Weapon belt removed and blade off the shaft for all electronics bring-up.
4. Use a physical **blade lock** (safety pin or block) whenever the robot is handled or in the pits.
5. LiPo safety: charge in a fire-safe bag/container, never unattended, never charge a damaged or puffed cell.
6. Wear eye protection near the robot during any powered test.
7. Have a second person present and agree a stop signal before any weapon test.

## 2. Compliance checklist — verify against the official rulebook

The limits below are working assumptions and are **not verified**. Do not rely on them. Tick each item against the current, official CY-BOTS'26 rulebook.

| Item | Working value | Rulebook says | OK? |
|---|---|---|---|
| Weight class / max mass | 10 kg | | |
| Footprint | 18 × 18 in | | |
| Maximum height | 12 cm | | |
| Maximum battery voltage | 60 V (robot is 11.1 V) | | |
| Weapon tip-speed limit | 113 m/s (design is ~16–24 m/s) | | |
| Weapon spin-down time limit | 60 s | | |
| Master switch requirements (number, type, location, accessibility) | two switches | | |
| Indicator light requirements | armed LED | | |
| Failsafe requirements (what must happen on signal loss) | weapon off | | |
| Blade lock / safety device requirement | blade lock | | |
| Weapon ESC braking allowed/required | brake off | | |
| Battery type/containment rules | 3S LiPo | | |
| Radio / frequency rules | 2.4 GHz | | |

## 3. Design decisions that depend on the rules

- The ESC brake setting, the switch arrangement and the failsafe behaviour must follow the rulebook, not this repository, if they differ.
- If the rules require a failsafe that stops the weapon within a defined time, measure that time in TEST_PLAN stage 6 and record it.

## 4. Disclaimer

This repository documents an engineering design for educational purposes. It comes without warranty, and the author takes no responsibility for use of the information. Anyone building from it is responsible for their own safety assessment, for following their event rules, and for testing independently.
