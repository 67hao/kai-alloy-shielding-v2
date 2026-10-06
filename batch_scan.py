#!/usr/bin/env python3
"""
batch_scan.py — Run KaiShielding for each material, rename outputs.

Usage:
    python3 batch_scan.py [--exe ./KaiShielding] [--nEvents 200000]
                          [--thickness 20] [--spectrum fission]

Produces:
    results/
        neutron_SS316L.csv
        neutron_FeCoNiCr.csv
        neutron_TaHEA.csv
        ...
        gamma_SS316L.csv
        ...
        summary.csv   (integrated transmission ratios)
"""
import subprocess, os, shutil, sys, argparse

# All materials to scan
MATERIALS = [
    # (macro_name,  display_label)
    ("SS316L",         "SS316L"),
    ("Fe",             "Pure_Fe"),
    ("FeCoNiCr",       "FeCoNiCr"),
    ("TaHEA",          "Fe30Ni30Cr30Al8Ta2"),
    ("0TaHEA",         "Fe30Ni30Cr30Al10"),
    ("L12HEA",         "L12_HEA_13elem"),
    ("CCIMA",          "CCIMA_CoNiAlTiTaNb"),
    ("FeCoNiCrAl0.3",  "FeCoNiCrAl0.3"),
    ("Inconel718",     "Inconel718"),
]

def write_macro(mat_name, thickness_mm, spectrum, n_events):
    """Write a temporary macro file for a single material run."""
    macro = f"""
/run/numberOfThreads 45
/shield/material   {mat_name}
/shield/thickness  {thickness_mm} mm
/source/spectrum   {spectrum}
/run/initialize
/run/printProgress {max(1, n_events // 10)}
/run/beamOn        {n_events}
"""
    with open("_tmp_scan.mac", "w") as f:
        f.write(macro)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", default="./KaiShielding")
    parser.add_argument("--nEvents", type=int, default=200000)
    parser.add_argument("--thickness", type=float, default=20.0)
    parser.add_argument("--spectrum", default="fission")
    args = parser.parse_args()

    os.makedirs("results", exist_ok=True)

    summary = []

    for mat_name, label in MATERIALS:
        print(f"\n{'='*60}")
        print(f"  Running: {label}  ({args.thickness} mm, {args.spectrum}, 45 threads)")
        print(f"{'='*60}\n")

        write_macro(mat_name, args.thickness, args.spectrum, args.nEvents)

        ret = subprocess.run([args.exe, "_tmp_scan.mac"],
                             capture_output=False)
        if ret.returncode != 0:
            print(f"*** WARNING: Run failed for {label}")
            continue

        # Rename outputs
        for prefix in ["neutron_transmission_spectrum", "gamma_transmission_spectrum"]:
            src = f"{prefix}.csv"
            tag = "neutron" if "neutron" in prefix else "gamma"
            dst = f"results/{tag}_{label}.csv"
            if os.path.exists(src):
                shutil.move(src, dst)
                print(f"  → {dst}")

        # Preserve SPECTRA-PKA outputs per material
        if os.path.exists("neutron_flux_for_spectra_pka.dat"):
            pka_dst = f"results/neutron_flux_{label}_for_spectra_pka.dat"
            shutil.move("neutron_flux_for_spectra_pka.dat", pka_dst)
            print(f"  → {pka_dst}")

        if os.path.exists("neutron_flux_summary.txt"):
            summary_dst = f"results/neutron_flux_summary_{label}.txt"
            shutil.move("neutron_flux_summary.txt", summary_dst)
            print(f"  → {summary_dst}")

        # Parse quick summary from neutron file
        dst_n = f"results/neutron_{label}.csv"
        if os.path.exists(dst_n):
            total_inc = 0; total_trans = 0
            with open(dst_n) as f:
                for line in f:
                    if line.startswith("#"): continue
                    parts = line.strip().split(",")
                    if len(parts) >= 5:
                        total_trans += float(parts[3])
                        total_inc   += float(parts[4])
            ratio = total_trans / total_inc if total_inc > 0 else 0
            summary.append((label, total_inc, total_trans, ratio))

    # Write summary
    with open("results/summary.csv", "w") as f:
        f.write("material,n_incident,n_transmitted,transmission_ratio\n")
        for label, inc, trans, ratio in summary:
            f.write(f"{label},{inc:.0f},{trans:.0f},{ratio:.6f}\n")
    print(f"\n>>> Summary written to results/summary.csv")

    # Cleanup
    if os.path.exists("_tmp_scan.mac"):
        os.remove("_tmp_scan.mac")

    # Print comparison table
    print(f"\n{'='*60}")
    print(f"  NEUTRON TRANSMISSION COMPARISON ({args.thickness} mm, {args.spectrum})")
    print(f"{'='*60}")
    print(f"  {'Material':<28} {'T_neutron':>12} {'μ_eff (cm⁻¹)':>14}")
    print(f"  {'-'*56}")
    import math
    d_cm = args.thickness / 10.0
    for label, inc, trans, ratio in sorted(summary, key=lambda x: x[3]):
        mu = -math.log(ratio) / d_cm if 0 < ratio < 1 else float('inf')
        print(f"  {label:<28} {ratio:>12.6f} {mu:>14.4f}")
    print(f"{'='*60}\n")

if __name__ == "__main__":
    main()
