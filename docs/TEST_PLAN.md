# Test plan

Test in stages. Do not move to the next stage until the previous one passes. At every stage record *expected result vs actual result* in `docs/test_log.md` (create it) — this is the evidence that the design works.

**Throughout:** battery disconnected whenever wiring is touched; no one in line with the blade; weapon belt removed and blade off the shaft until stage 6.

## Stage 0 — PC unit tests
Run `firmware/tests/test_logic.cpp`. Expected: "All tests passed." Re-run after any change to `fury_logic.h`.

## Stage 1 — Receiver and pulse capture (no motors connected)
- Power ESP32 and receiver from USB / the buck converter. Add a temporary `Serial.println` of the three pulse widths.
- Expected: about 1000–2000 µs, centre about 1500 µs, updates about every 20 ms. Record the actual min/centre/max per channel and use them to adjust the deadband if needed.
- Check the pulse level with a multimeter/scope (3.3 V vs 5 V) before connecting it to the GPIO permanently.

## Stage 2 — Receiver failsafe behaviour
- Switch the transmitter off. Record what the receiver outputs: no pulse, held value, or the programmed failsafe value.
- Program the failsafe on the weapon channel to 1000 µs. Repeat and confirm.
- This determines how much each firmware layer is actually contributing.

## Stage 3 — Drive system (wheels off the ground)
- Connect IBT-2 boards and drive motors with the robot on blocks.
- Check: centre stick = stopped; forward/back/left/right directions correct; speeds proportional; ramp feels sensible.
- Switch the transmitter off: motors stop immediately and the drive enable line goes low. Record the stop time.

## Stage 4 — Weapon ESC and motor (belt removed, motor bolted down)
- Fix the motor securely, shaft free, nothing on it, area clear.
- Power up with the weapon stick **high**: expected result is *no spin* (arming interlock). Lower the stick for about half a second: the armed LED lights.
- Raise throttle slowly to a low value (set `kWeaponMaxUs` to about 1300 for this test): the motor spins smoothly and the ramp is gradual.
- Switch the transmitter off while spinning: motor must stop spinning up and the ESC must cut drive; record coast-down time.
- Switch the transmitter on with the stick high: expected *no spin* until the stick is lowered for 0.5 s.
- Pull the ESC signal wire while running: record what the ESC does (stops/continues).
- Reset the ESP32 mid-run (EN button): confirm the motor stops.
- Check ESC and motor temperature after the tests.

## Stage 5 — Mechanical assembly without the blade
- Fit the belt and pulleys; spin the motor at low throttle with no blade; check belt tracking, alignment, noise and belt tension.
- Measure the belt slip torque (increase load until the belt slips) and record it.

## Stage 6 — Blade fitted, polycarbonate test box
- Robot fully inside a rated test box, remote operated from outside. Check the rulebook and your institution's safety rules before this stage.
- Spin up in steps (cap the throttle at 1300, then 1500, then 1700, then full), recording RPM (tachometer or slow-motion video), current, battery sag and vibration at each step.
- Compare measured RPM with the 2960/2516 RPM predictions in `calculations/results.md` and update the documents with real numbers.
- Check for vibration; stop and rebalance the blade if there is any.
- Spin-down timing: record the time from the kill command to the blade stopping (compare to the rule limit).

## Stage 7 — Integration and drive-while-spinning
- Verify radio range and that weapon motor noise does not disturb the receiver or the ESP32 (watch for resets or invalid pulses).
- Drive tests with the weapon spinning inside the test box area; check handling and gyroscopic effects.

## Stage 8 — Pre-competition checklist
- All fasteners thread-locked and torque-checked; belt spares and tools packed.
- Failsafe tested again with the final configuration; test results recorded.
- Safety inspection items from the rulebook ticked off (see `SAFETY_AND_COMPLIANCE.md`).
