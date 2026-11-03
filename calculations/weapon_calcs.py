#!/usr/bin/env python3
"""Weapon sizing calculations for the Fury vertical spinner.

Run:  python3 weapon_calcs.py            (prints Markdown tables)

Everything here is a first-order ESTIMATE for design decisions. The moment of
inertia should be replaced with the value from the CAD model once the blade is
drawn, and the speeds should be replaced with measured values after testing.
Edit the INPUTS section to explore alternatives.
"""
import math

# ------------------------------------------------------------------ INPUTS
KV_RPM_PER_V = 800        # motor speed constant (D4250 800 kV, from design notes)
V_NOMINAL = 11.1          # 3S LiPo nominal voltage
LOAD_FACTOR = 0.85        # fraction of no-load RPM reached under load (assumption)
BLADE_DIAMETER_M = 0.105  # tip-to-tip, limited by the 12 cm height envelope
STEEL_DENSITY = 7850      # kg/m^3 (AR500 / Hardox ~ plain steel)
GEAR_RATIOS = [2.0, 2.5, 3.0]   # motor:weapon reduction options
MASS_OPTIONS_KG = [0.5, 0.75, 1.0, 1.5]
THICKNESS_OPTIONS_MM = [10, 12, 15, 20]
PROFILE_FILL = 0.5        # fraction of a full disc the single-tooth profile occupies (assumption)
# Shape factor k in I = k * (1/2) m r^2 :  1.0 = solid disc (upper bound),
# 0.67 = straight bar (I = m L^2 / 12 about its centre with L = 2r).
SHAPE_FACTORS = {"bar (k=0.67)": 0.67, "disc (k=1.0)": 1.0}
# Spin-up estimate assumptions
ESC_CURRENT_A = 60.0      # sustained current assumed from an 80 A ESC (derated)
V_LOADED = 10.5           # battery voltage under load
DRIVETRAIN_EFF = 0.70     # motor + ESC + belt efficiency (assumption)
# Gyroscopic check
TURN_RATE_RAD_S = 3.0     # about 170 deg/s, a fast turn for a 10 kg robot
TRACK_WIDTH_M = 0.40

R = BLADE_DIAMETER_M / 2.0


def rpm_weapon(ratio, loaded=False):
    rpm = KV_RPM_PER_V * V_NOMINAL / ratio
    return rpm * (LOAD_FACTOR if loaded else 1.0)


def omega(rpm):
    return rpm * 2 * math.pi / 60.0


def inertia(m, k):
    return k * 0.5 * m * R ** 2


def energy(m, k, rpm):
    return 0.5 * inertia(m, k) * omega(rpm) ** 2


def table(headers, rows):
    out = ["| " + " | ".join(headers) + " |", "|" + "|".join("---" for _ in headers) + "|"]
    for r in rows:
        out.append("| " + " | ".join(str(c) for c in r) + " |")
    return "\n".join(out)


def main():
    print("# Calculation results\n")
    print(f"Inputs: {KV_RPM_PER_V} kV, {V_NOMINAL} V, blade diameter {BLADE_DIAMETER_M*100:.1f} cm "
          f"(r = {R*100:.2f} cm), load factor {LOAD_FACTOR}.\n")

    # 1. Speeds
    rows = []
    for g in GEAR_RATIOS:
        nl = rpm_weapon(g)
        ld = rpm_weapon(g, loaded=True)
        rows.append([f"{g}:1", f"{nl:.0f}", f"{ld:.0f}",
                     f"{omega(nl):.0f}", f"{omega(nl)*R:.1f}", f"{omega(ld)*R:.1f}"])
    print("## 1. Weapon speed and tip speed\n")
    print(table(["Reduction", "No-load RPM", "Loaded RPM (est.)", "No-load rad/s",
                 "Tip speed no-load (m/s)", "Tip speed loaded (m/s)"], rows))
    print("\nTip speed = omega x r. All values are far below the 113 m/s limit assumed for "
          "this design (verify against the official rulebook).\n")

    # 2. Geometry-limited mass
    area_disc = math.pi * R ** 2
    rows = []
    for t in THICKNESS_OPTIONS_MM:
        mass = STEEL_DENSITY * PROFILE_FILL * area_disc * (t / 1000.0)
        rows.append([t, f"{mass:.2f}"])
    print("## 2. What mass actually fits in a 10.5 cm profile?\n")
    print(f"Single-tooth profile assumed to fill {PROFILE_FILL*100:.0f}% of a full disc.\n")
    print(table(["Plate thickness (mm)", "Blade mass (kg)"], rows))
    t_needed = 1.5 / (STEEL_DENSITY * PROFILE_FILL * area_disc) * 1000
    print(f"\nA 1.5 kg blade would need about {t_needed:.0f} mm of steel at this profile - "
          "not practical in this envelope. Design mass should be taken from CAD; "
          "plan on roughly 0.5 - 0.9 kg.\n")

    # 3. Energy
    print("## 3. Stored kinetic energy  E = 1/2 I w^2  (loaded RPM)\n")
    for g in GEAR_RATIOS:
        rpm = rpm_weapon(g, loaded=True)
        rows = []
        for m in MASS_OPTIONS_KG:
            row = [f"{m:.2f}"]
            for name, k in SHAPE_FACTORS.items():
                row.append(f"{energy(m, k, rpm):.0f}")
            rows.append(row)
        print(f"**Reduction {g}:1 ({rpm:.0f} RPM loaded)**\n")
        print(table(["Mass (kg)"] + [f"E (J), {n}" for n in SHAPE_FACTORS], rows))
        print()
    print("Energy scales with mass and with RPM squared. Moving from 3:1 to 2:1 raises RPM by 1.5x "
          "and energy by 2.25x for the same blade, but needs more motor torque to spin it up - "
          "this is the main design trade-off to test.\n")

    # 4. Spin-up time
    p_avg = V_LOADED * ESC_CURRENT_A * DRIVETRAIN_EFF
    print("## 4. Spin-up time (energy / average power)\n")
    print(f"Assumed mechanical power into the weapon: {p_avg:.0f} W "
          f"({V_LOADED} V x {ESC_CURRENT_A:.0f} A x {DRIVETRAIN_EFF}). Lower bound only - ignores "
          "motor torque limits, voltage sag and belt slip. Measure this on the real build.\n")
    rows = []
    for g in GEAR_RATIOS:
        rpm = rpm_weapon(g, loaded=True)
        for m in (0.5, 0.75):
            e = energy(m, 1.0, rpm)
            rows.append([f"{g}:1", f"{m:.2f}", f"{e:.0f}", f"{e/p_avg:.2f}"])
    print(table(["Reduction", "Mass (kg)", "E (J), disc bound", "Min spin-up time (s)"], rows))
    print()

    # 5. Gyroscopic effect
    m, k, g = 0.75, 1.0, 3.0
    I = inertia(m, k)
    w = omega(rpm_weapon(g))
    torque = I * w * TURN_RATE_RAD_S
    force = torque / TRACK_WIDTH_M
    print("## 5. Gyroscopic effect when turning\n")
    print(f"For m = {m} kg, {g}:1, turn rate {TURN_RATE_RAD_S} rad/s: "
          f"gyro torque = I w Omega = {torque:.2f} N m; as a force across a "
          f"{TRACK_WIDTH_M*100:.0f} cm track = {force:.1f} N, against a robot weight of "
          f"{10*9.81:.0f} N. At this scale the effect is small (a few percent); "
          "it matters more for larger, faster weapons.\n")

    # 6. Belt
    print("## 6. Belt drive geometry (reference)\n")
    print("Drive pulley 20 mm : driven pulley 60 mm gives 3:1. Belt slip torque is set by belt "
          "tension and pulley wrap angle and must be measured on the build - it limits peak torque "
          "to the motor but does not by itself protect it in a sustained stall.\n")


if __name__ == "__main__":
    main()
