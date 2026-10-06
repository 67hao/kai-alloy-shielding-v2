#!/usr/bin/env python3
"""
generate_spectra_pka_inputs.py

Generates SPECTRA-PKA input files for all 6 of Prof. Kai's alloy
compositions. Calculates the exact isotopic atomic fractions from:
  (element atomic fraction) × (natural isotopic abundance)

Also computes parent and daughter masses for the (n,γ) recoil estimate,
which SPECTRA-PKA uses to approximate neutron-capture recoil energies.

For each isotope with mass number A and element symbol El:
  parent_mass  = atomic mass of El-A  (the target)
  daughter_mass = atomic mass of El-(A+1)  (the product after neutron capture)

Data sources:
  - Natural isotopic abundances: IUPAC 2021 recommended values
  - Atomic masses: AME2020 (Atomic Mass Evaluation)
  - Alloy compositions: Prof. Kai's published papers (2024-2026)

Author: Nghia
Date: March 2026
"""
import os

# ====================================================================
# SECTION 1: NATURAL ISOTOPIC ABUNDANCES (IUPAC 2021)
# Format: { 'Element': [(mass_number, abundance_fraction, parent_mass_amu, daughter_mass_amu), ...] }
#
# parent_mass  = mass of isotope A (the neutron target)
# daughter_mass = mass of isotope A+1 (product of neutron capture)
# These are needed for SPECTRA-PKA's (n,γ) recoil energy estimate.
# ====================================================================
ISOTOPES = {
    # Iron (Fe): Z=26
    # 4 stable isotopes
    'Fe': [
        # (A, natural_abundance, mass_A, mass_A+1)
        (54, 0.05845, 53.9396090, 54.9382934),   # Fe-54 → Fe-55 via (n,γ)
        (56, 0.91754, 55.9349363, 56.9353928),   # Fe-56 → Fe-57 (most abundant)
        (57, 0.02119, 56.9353928, 57.9332744),   # Fe-57 → Fe-58
        (58, 0.00282, 57.9332744, 58.9348755),   # Fe-58 → Fe-59
    ],
    # Cobalt (Co): Z=27
    # Only 1 stable isotope
    'Co': [
        (59, 1.00000, 58.9331950, 59.9338222),   # Co-59 → Co-60 (radioactive! t½=5.27y)
    ],
    # Nickel (Ni): Z=28
    # 5 stable isotopes
    'Ni': [
        (58, 0.68077, 57.9353429, 58.9343467),   # Ni-58 → Ni-59
        (60, 0.26223, 59.9307864, 60.9310560),   # Ni-60 → Ni-61
        (61, 0.01140, 60.9310560, 61.9283451),   # Ni-61 → Ni-62
        (62, 0.03635, 61.9283451, 62.9296694),   # Ni-62 → Ni-63
        (64, 0.00926, 63.9279660, 64.9300843),   # Ni-64 → Ni-65
    ],
    # Chromium (Cr): Z=24
    # 4 stable isotopes
    'Cr': [
        (50, 0.04345, 49.9460422, 50.9447674),   # Cr-50 → Cr-51
        (52, 0.83789, 51.9405075, 52.9406494),   # Cr-52 → Cr-53 (most abundant)
        (53, 0.09501, 52.9406494, 53.9388804),   # Cr-53 → Cr-54
        (54, 0.02365, 53.9388804, 54.9408397),   # Cr-54 → Cr-55
    ],
    # Aluminum (Al): Z=13
    # Only 1 stable isotope
    'Al': [
        (27, 1.00000, 26.9815385, 27.9819102),   # Al-27 → Al-28
    ],
    # Tantalum (Ta): Z=73
    # Effectively monoisotopic (Ta-180m is 0.012%, negligible)
    'Ta': [
        (181, 0.99988, 180.9479958, 181.9501518), # Ta-181 → Ta-182
    ],
    # Titanium (Ti): Z=22
    # 5 stable isotopes
    'Ti': [
        (46, 0.0825, 45.9526316, 46.9517631),    # Ti-46 → Ti-47
        (47, 0.0744, 46.9517631, 47.9479463),    # Ti-47 → Ti-48
        (48, 0.7372, 47.9479463, 48.9478700),    # Ti-48 → Ti-49 (most abundant)
        (49, 0.0541, 48.9478700, 49.9447912),    # Ti-49 → Ti-50
        (50, 0.0518, 49.9447912, 50.9466102),    # Ti-50 → Ti-51
    ],
    # Tungsten (W): Z=74
    # 5 stable isotopes (W-180 is quasi-stable, very low abundance)
    'W': [
        (180, 0.0012,  179.9467108, 180.9481978), # W-180 → W-181
        (182, 0.2650,  181.9482042, 182.9502230), # W-182 → W-183
        (183, 0.1431,  182.9502230, 183.9509312), # W-183 → W-184
        (184, 0.3064,  183.9509312, 184.9534193), # W-184 → W-185
        (186, 0.2843,  185.9543641, 186.9571605), # W-186 → W-187
    ],
    # Niobium (Nb): Z=41
    # Only 1 stable isotope
    'Nb': [
        (93, 1.00000, 92.9063781, 93.9072839),   # Nb-93 → Nb-94
    ],
    # Molybdenum (Mo): Z=42
    # 7 stable isotopes
    'Mo': [
        (92,  0.14649, 91.9068099, 92.9068107),  # Mo-92 → Mo-93
        (94,  0.09187, 93.9050883, 94.9058421),  # Mo-94 → Mo-95
        (95,  0.15873, 94.9058421, 95.9046795),  # Mo-95 → Mo-96
        (96,  0.16673, 95.9046795, 96.9060215),  # Mo-96 → Mo-97
        (97,  0.09582, 96.9060215, 97.9054082),  # Mo-97 → Mo-98
        (98,  0.24292, 97.9054082, 98.9077119),  # Mo-98 → Mo-99
        (100, 0.09744, 99.9074718, 100.910347),  # Mo-100 → Mo-101
    ],
    # Hafnium (Hf): Z=72
    # 6 stable isotopes
    'Hf': [
        (174, 0.0016, 173.9400461, 174.9415086), # Hf-174 → Hf-175
        (176, 0.0526, 175.9414086, 176.9432207), # Hf-176 → Hf-177
        (177, 0.1860, 176.9432207, 177.9436988), # Hf-177 → Hf-178
        (178, 0.2728, 177.9436988, 178.9458161), # Hf-178 → Hf-179
        (179, 0.1362, 178.9458161, 179.9465500), # Hf-179 → Hf-180
        (180, 0.3508, 179.9465500, 180.9491012), # Hf-180 → Hf-181
    ],
    # Boron (B): Z=5
    # 2 stable isotopes — B-10 is critical for (n,α) thermal neutron capture!
    'B': [
        (10, 0.199,  10.0129370, 11.0093054),    # B-10 → B-11 (σ_nα = 3840 b thermal!)
        (11, 0.801,  11.0093054, 12.0143521),    # B-11 → B-12
    ],
    # Zirconium (Zr): Z=40
    # 5 stable isotopes
    'Zr': [
        (90, 0.5145, 89.9047044, 90.9056458),    # Zr-90 → Zr-91
        (91, 0.1122, 90.9056458, 91.9050408),    # Zr-91 → Zr-92
        (92, 0.1715, 91.9050408, 92.9064760),    # Zr-92 → Zr-93
        (94, 0.1738, 93.9063152, 94.9080426),    # Zr-94 → Zr-95
        (96, 0.0280, 95.9082734, 96.9109531),    # Zr-96 → Zr-97
    ],
    # Silicon (Si): Z=14 (for potential Si-bearing alloys)
    'Si': [
        (28, 0.92223, 27.9769265, 28.9764947),   # Si-28 → Si-29
        (29, 0.04685, 28.9764947, 29.9737702),   # Si-29 → Si-30
        (30, 0.03092, 29.9737702, 30.9753633),   # Si-30 → Si-31
    ],
}

# ====================================================================
# SECTION 2: PROF. KAI'S ALLOY COMPOSITIONS (atomic percent)
#
# Each alloy is defined as:
#   name: descriptive name
#   paper: source paper reference
#   composition: {element: atomic_percent} dictionary
#   Ed: threshold displacement energy (eV) for NRT-dpa
# ====================================================================
ALLOYS = [
    {
        'name': 'FeCoNiCr',
        'stub': 'FeCoNiCr',
        'paper': 'Multiple papers [P2,P4,P5] — equiatomic FCC HEA',
        'composition': {'Fe': 25.0, 'Co': 25.0, 'Ni': 25.0, 'Cr': 25.0},
        'Ed': 40,  # eV, standard for FCC Fe-based
    },
    {
        'name': 'Fe30Ni30Cr30Al8Ta2 (2Ta-HEA)',
        'stub': 'TaHEA',
        'paper': 'Intermetallics 190 (2026) 109170',
        'composition': {'Fe': 30.0, 'Ni': 30.0, 'Cr': 30.0, 'Al': 8.0, 'Ta': 2.0},
        'Ed': 40,
    },
    {
        'name': 'Fe30Ni30Cr30Al10 (0Ta-HEA)',
        'stub': '0TaHEA',
        'paper': 'Intermetallics 190 (2026) 109170 — control sample',
        'composition': {'Fe': 30.0, 'Ni': 30.0, 'Cr': 30.0, 'Al': 10.0},
        'Ed': 40,
    },
    {
        'name': 'L12-HEA (13-element)',
        'stub': 'L12HEA',
        'paper': 'JALCOM 1034 (2025) 181380',
        'composition': {
            'Ni': 41.98, 'Co': 23.70, 'Fe': 8.0, 'Cr': 10.0,
            'Ti': 5.0, 'Al': 8.6, 'Ta': 1.0, 'W': 0.5,
            'Nb': 0.5, 'Mo': 0.5, 'Hf': 0.1, 'B': 0.1, 'Zr': 0.02
        },
        'Ed': 40,  # Dominated by Ni/Co/Fe/Cr → use 40 eV
    },
    {
        'name': 'CCIMA CoNiAlTiTaNb (L12-type)',
        'stub': 'CCIMA',
        'paper': 'JALCOM 1042 (2025) 183893',
        'composition': {
            'Co': 38.8, 'Ni': 37.5, 'Al': 13.8,
            'Ti': 3.1, 'Ta': 2.8, 'Nb': 3.5, 'B': 0.5
        },
        'Ed': 40,
    },
    {
        'name': 'FeCoNiCrAl0.3',
        'stub': 'FeCoNiCrAl03',
        'paper': 'JALCOM (2025) — CO2/CO oxidation study',
        # Molar ratio 1:1:1:1:0.3 → each major = 100/4.3 = 23.256%
        'composition': {'Fe': 23.256, 'Co': 23.256, 'Ni': 23.256, 'Cr': 23.256, 'Al': 6.977},
        'Ed': 40,
    },
]

# ====================================================================
# SECTION 3: GENERATE SPECTRA-PKA INPUT FILES
# ====================================================================
def generate_input(alloy, pka_data_dir="${PKA_DATA}",
                   flux_file="fluxes_specter.dat",
                   flux_norm_type=2,
                   flux_rescale=3.25e14):
    """
    Generate a SPECTRA-PKA input file for one alloy.
    
    Parameters:
        alloy: dict with 'name', 'stub', 'paper', 'composition', 'Ed'
        pka_data_dir: path to NJOY-processed recoil cross-section files
        flux_file: neutron spectrum file (709-group or compatible)
        flux_norm_type: 2 = flux is in n/cm²/s per group
        flux_rescale: total flux normalization (n/cm²/s)
    
    Returns:
        String content of the .in file
    """
    lines = []
    
    # ---- Header comments ----
    lines.append(f'# ================================================================')
    lines.append(f'# SPECTRA-PKA input for: {alloy["name"]}')
    lines.append(f'# Source paper: {alloy["paper"]}')
    lines.append(f'# Composition (at%): {alloy["composition"]}')
    lines.append(f'# Threshold displacement energy Ed = {alloy["Ed"]} eV')
    lines.append(f'# Generated by: generate_spectra_pka_inputs.py')
    lines.append(f'# ================================================================')
    lines.append(f'')
    
    # ---- Flux file and output stub ----
    lines.append(f'# Path to neutron energy spectrum file')
    lines.append(f'# (replace with Geant4-generated spectrum for LFR conditions)')
    lines.append(f'flux_filename="{flux_file}"')
    lines.append(f'')
    lines.append(f'# Output file prefix — all results will be {alloy["stub"]}_*.out')
    lines.append(f'results_stub="{alloy["stub"]}"')
    lines.append(f'')
    
    # ---- Column format ----
    # SPECTRA-PKA reads isotope data in tabular format.
    # 6 columns: filename, atomic_fraction, parent_element, parent_A,
    #            parent_mass(amu), daughter_mass(amu)
    lines.append(f'# Number of data columns per isotope line')
    lines.append(f'num_columns=6')
    lines.append(f'')
    lines.append(f'# Column definitions:')
    lines.append(f'#   pka_filename         = path to NJOY recoil cross-section file')
    lines.append(f'#   pka_ratios           = atomic fraction of this isotope in alloy')
    lines.append(f'#   parent_ele           = element symbol of target')
    lines.append(f'#   parent_num           = mass number A of target')
    lines.append(f'#   ngamma_parent_mass   = atomic mass of target isotope (amu)')
    lines.append(f'#   ngamma_daughter_mass  = atomic mass of (n,γ) product (amu)')
    lines.append(f'columns= pka_filename pka_ratios parent_ele parent_num ngamma_parent_mass ngamma_daughter_mass')
    lines.append(f'')
    
    # ---- Isotope entries ----
    # For each element in the alloy, expand into individual isotopes
    # isotope_fraction = element_at% × isotopic_abundance / 100
    total_isotopes = 0
    
    for elem, at_pct in sorted(alloy['composition'].items(),
                                key=lambda x: -x[1]):  # Sort by abundance (descending)
        if elem not in ISOTOPES:
            print(f"  WARNING: No isotope data for {elem}, skipping!")
            continue
        
        # Element fraction as decimal (e.g., 30% → 0.30)
        elem_frac = at_pct / 100.0
        
        lines.append(f'# --- {elem} ({at_pct:.3f} at%) ---')
        
        for A, iso_abund, parent_mass, daughter_mass in ISOTOPES[elem]:
            # Isotopic fraction in the alloy = element fraction × isotopic abundance
            iso_frac = elem_frac * iso_abund
            
            # Skip negligible isotopes (< 1e-6 atomic fraction)
            if iso_frac < 1e-6:
                continue
            
            # SPECTRA-PKA expects the PKA data filename format:
            # El{A:03d}s.asc  (e.g., Fe056s.asc, Ta181s.asc)
            filename = f'{pka_data_dir}{elem}{A:03d}s.asc'
            
            # Write the isotope line with detailed comment
            lines.append(
                f'"{filename}" '
                f'{iso_frac:.8f} '      # atomic fraction in alloy
                f'{elem} {A} '           # parent element and mass number
                f'{parent_mass:.7f} '    # parent atomic mass (amu)
                f'{daughter_mass:.7f}'   # daughter atomic mass for (n,γ) estimate
                f'   # {elem}-{A}: {iso_abund*100:.3f}% of {elem}, '
                f'{iso_frac*100:.4f}% of alloy'
            )
            total_isotopes += 1
        
        lines.append(f'')  # Blank line between elements
    
    # ---- Control parameters ----
    lines.append(f'# ================================================================')
    lines.append(f'# CONTROL PARAMETERS')
    lines.append(f'# ================================================================')
    lines.append(f'')
    lines.append(f'# Flux normalization type:')
    lines.append(f'#   1 = flux is per unit lethargy (dφ/d(lnE))')
    lines.append(f'#   2 = flux is per energy group (φ_j in n/cm²/s)')
    lines.append(f'flux_norm_type={flux_norm_type}')
    lines.append(f'')
    lines.append(f'# PKA data file format type:')
    lines.append(f'#   2 = standard NJOY-GROUPR output format')
    lines.append(f'pka_filetype=2')
    lines.append(f'')
    lines.append(f'# Enable per-reaction-type DPA calculations')
    lines.append(f'# (This gives the crucial per-channel breakdown from Gilbert 2018)')
    lines.append(f'do_mtd_sums=.true.')
    lines.append(f'')
    lines.append(f'# Enable (n,γ) neutron-capture recoil estimate')
    lines.append(f'# (Uses parent/daughter masses defined above)')
    lines.append(f'do_ngamma_estimate=.t.')
    lines.append(f'')
    lines.append(f'# Enable global summed PKA distributions')
    lines.append(f'# (sum over all elements → total PKA spectrum for the alloy)')
    lines.append(f'do_global_sums=.t.')
    lines.append(f'')
    lines.append(f'# Exclude light particles (p, α, d, t) from total PKA spectrum')
    lines.append(f'# (They produce different damage types; handle separately)')
    lines.append(f'do_exclude_light_from_total=.t.')
    lines.append(f'')
    lines.append(f'# Total number of isotope input files')
    lines.append(f'number_pka_files={total_isotopes}')
    lines.append(f'')
    lines.append(f'# Flux rescaling value (total integrated flux, n/cm²/s)')
    lines.append(f'# Default: 3.25e14 (typical PWR value from SPECTRA-PKA test)')
    lines.append(f'# REPLACE with actual value from Geant4 transport simulation!')
    lines.append(f'flux_rescale_value={flux_rescale:.2e}')
    lines.append(f'')
    lines.append(f'# Maximum number of recoil species to track')
    lines.append(f'# (for multi-element alloys, need higher value)')
    lines.append(f'max_global_recoils=800')
    lines.append(f'')
    lines.append(f'# Energy bin structure appears once per PKA data file')
    lines.append(f'# (standard for TENDL PKA data files)')
    lines.append(f'energies_once_perfile=.t.')
    lines.append(f'')
    lines.append(f'# Threshold displacement energy for NRT-dpa calculation (eV)')
    lines.append(f'# Using {alloy["Ed"]} eV (standard for {list(alloy["composition"].keys())[0]}-based alloys)')
    lines.append(f'assumed_Ed={alloy["Ed"]}.0')
    
    return '\n'.join(lines), total_isotopes


def generate_summary_table(alloy_results):
    """Print a summary table of all alloys and their isotope counts."""
    print("\n" + "="*75)
    print("  SPECTRA-PKA INPUT FILES SUMMARY")
    print("="*75)
    print(f"  {'Alloy':<35} {'Elements':>8} {'Isotopes':>9} {'File':>18}")
    print(f"  {'-'*72}")
    for name, n_elem, n_iso, filename in alloy_results:
        print(f"  {name:<35} {n_elem:>8} {n_iso:>9} {filename:>18}")
    print("="*75)
    print()


# ====================================================================
# SECTION 4: MAIN EXECUTION
# ====================================================================
def main():
    output_dir = "spectra_pka_inputs"
    os.makedirs(output_dir, exist_ok=True)
    
    alloy_results = []
    
    for alloy in ALLOYS:
        content, n_isotopes = generate_input(alloy)
        filename = f'{alloy["stub"]}.in'
        filepath = os.path.join(output_dir, filename)
        
        with open(filepath, 'w') as f:
            f.write(content)
        
        n_elements = len(alloy['composition'])
        alloy_results.append((alloy['name'], n_elements, n_isotopes, filename))
        
        print(f"  → {filepath}  ({n_elements} elements, {n_isotopes} isotopes)")
    
    generate_summary_table(alloy_results)
    
    # ---- Also generate a verification table ----
    verify_path = os.path.join(output_dir, "VERIFICATION_TABLE.txt")
    with open(verify_path, 'w') as f:
        f.write("ISOTOPIC FRACTION VERIFICATION TABLE\n")
        f.write("="*80 + "\n\n")
        
        for alloy in ALLOYS:
            f.write(f"ALLOY: {alloy['name']}\n")
            f.write(f"Paper: {alloy['paper']}\n")
            f.write(f"{'Isotope':<12} {'Element at%':>12} {'Iso. abund.':>12} {'Alloy frac.':>12} {'Alloy at%':>10}\n")
            f.write("-"*60 + "\n")
            
            total_frac = 0.0
            for elem, at_pct in sorted(alloy['composition'].items(), key=lambda x: -x[1]):
                if elem not in ISOTOPES:
                    continue
                elem_frac = at_pct / 100.0
                for A, iso_abund, _, _ in ISOTOPES[elem]:
                    iso_frac = elem_frac * iso_abund
                    if iso_frac < 1e-6:
                        continue
                    total_frac += iso_frac
                    f.write(f"{elem}-{A:<7} {at_pct:>12.3f}% {iso_abund*100:>11.3f}% {iso_frac:>12.8f} {iso_frac*100:>9.4f}%\n")
            
            f.write(f"{'TOTAL':>12} {'':>12} {'':>12} {total_frac:>12.8f} {total_frac*100:>9.4f}%\n")
            f.write(f"\n{'='*60}\n\n")
    
    print(f"  → {verify_path}")
    print("\nAll input files generated successfully.")
    print("To run: cd spectra_pka_inputs && /path/to/spectra-pka <ALLOY>.in")


if __name__ == "__main__":
    main()
