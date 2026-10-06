#!/bin/bash
#====================================================================
# SPECTRA-PKA: Installation & Usage Guide for Prof. Kai's HEA Alloys
#
# This script documents the complete workflow to:
# 1. Install SPECTRA-PKA
# 2. Download pre-computed PKA nuclear data (TENDL libraries)
# 3. Set up input files for Kai's multi-element HEA compositions
# 4. Run and interpret results
#
# Author: Nghia
# Date: March 2026
#====================================================================

# ==================================================================
# STEP 1: INSTALL SPECTRA-PKA
# ==================================================================
# SPECTRA-PKA is a Fortran90 program. Needs GNU Make + gfortran.
# No external libraries required (self-contained).

git clone https://github.com/fispact/SPECTRA-PKA.git
cd SPECTRA-PKA
make
# This produces the executable: spectra-pka (or SPECTRA_PKA)

# Verify:
ls -la spectra-pka
# Should see an executable file

# Quick test with built-in Zr example:
cd manual/test
../../spectra-pka ZR.in
# Last line should say "END as expected"

# ==================================================================
# STEP 2: DOWNLOAD PRE-COMPUTED PKA NUCLEAR DATA
# ==================================================================
# SPECTRA-PKA needs NJOY-processed recoil cross-section matrices.
# These are NON-STANDARD outputs from a modified NJOY-GROUPR module.
#
# Pre-computed databases are available from UKAEA:
#   https://fispact.ukaea.uk/nuclear-data/downloads/
#
# Download the TENDL-2017 (or latest) PKA data archive.
# The archive contains .asc files for each isotope, e.g.:
#   Fe054s.asc, Fe056s.asc, Fe057s.asc, Fe058s.asc
#   Ni058s.asc, Ni060s.asc, Ni061s.asc, Ni062s.asc, Ni064s.asc
#   Cr050s.asc, Cr052s.asc, Cr053s.asc, Cr054s.asc
#   Co059s.asc
#   Al027s.asc
#   Ta181s.asc
#   Ti046s.asc, Ti047s.asc, Ti048s.asc, Ti049s.asc, Ti050s.asc
#   Nb093s.asc
#   W180s.asc, W182s.asc, W183s.asc, W184s.asc, W186s.asc
#   Mo092s.asc ... Mo100s.asc
#   Hf174s.asc ... Hf180s.asc
#   B010s.asc, B011s.asc
#   Zr090s.asc ... Zr096s.asc
#   etc.
#
# IMPORTANT: For Kai's 13-element L12-HEA, you need PKA data for
# ALL isotopes of ALL 13 elements (Ni, Co, Fe, Cr, Ti, Al, Ta, W,
# Nb, Mo, Hf, B, Zr). This is ~70+ isotope files.
#
# Set the path to your downloaded data:
export PKA_DATA="/path/to/tendl2017_pka_data/"

# ==================================================================
# STEP 3: PREPARE NEUTRON SPECTRUM
# ==================================================================
# You need a neutron energy spectrum in the CCFE-709 group structure.
# Options:
#   a) Use a pre-defined spectrum (PWR, DEMO FW, etc.)
#      - Example files come with SPECTRA-PKA (e.g., fluxes_specter.dat)
#   b) Generate from Geant4/MCNP transport simulation
#      - Run Geant4 simulation of your LFR geometry
#      - Tally the neutron flux spectrum in the alloy region
#      - Rebin into the 709-group structure
#
# Format: 2 columns (upper energy bound in eV, flux in n/cm²/s)
# or specific SPECTER-like format (see manual)
#
# For proof-of-concept, use the built-in PWR spectrum:
# (located at manual/test/fluxes_specter.dat)
# Later, replace with Geant4-simulated LFR spectrum.

# ==================================================================
# STEP 4: CREATE INPUT FILE FOR FE30NI30CR30AL8TA2 (2Ta-HEA)
# ==================================================================
# This is Prof. Kai's key alloy from Intermetallics 2026.
# Composition (at%): Fe30, Ni30, Cr30, Al8, Ta2
#
# Need: natural isotopic abundances × atomic fraction
# Fe: 54(5.845%), 56(91.754%), 57(2.119%), 58(0.282%)
# Ni: 58(68.077%), 60(26.223%), 61(1.140%), 62(3.635%), 64(0.926%)
# Cr: 50(4.345%), 52(83.789%), 53(9.501%), 54(2.365%)
# Al: 27(100%)
# Ta: 181(99.988%)

cat > TaHEA.in << 'EOF'
flux_filename="fluxes_specter.dat"
results_stub="TaHEA"
num_columns=6
columns= pka_filename pka_ratios parent_ele parent_num ngamma_parent_mass ngamma_daughter_mass
# Fe isotopes (30 at% total)
"${PKA_DATA}Fe054s.asc" 0.01754 Fe 54 53.939608899 54.938291696
"${PKA_DATA}Fe056s.asc" 0.27526 Fe 56 55.934935613 56.935390744
"${PKA_DATA}Fe057s.asc" 0.00636 Fe 57 56.935390744 57.933273806
"${PKA_DATA}Fe058s.asc" 0.00085 Fe 58 57.933273806 58.934874338
# Ni isotopes (30 at% total)
"${PKA_DATA}Ni058s.asc" 0.20423 Ni 58 57.935341780 58.934346106
"${PKA_DATA}Ni060s.asc" 0.07867 Ni 60 59.930785256 60.931054945
"${PKA_DATA}Ni061s.asc" 0.00342 Ni 61 60.931054945 61.928342880
"${PKA_DATA}Ni062s.asc" 0.01091 Ni 62 61.928342880 62.929667236
"${PKA_DATA}Ni064s.asc" 0.00278 Ni 64 63.927966341 64.930085440
# Cr isotopes (30 at% total)
"${PKA_DATA}Cr050s.asc" 0.01304 Cr 50 49.946041835 50.944763910
"${PKA_DATA}Cr052s.asc" 0.25137 Cr 52 51.940505100 52.940646943
"${PKA_DATA}Cr053s.asc" 0.02850 Cr 53 52.940646943 53.938878305
"${PKA_DATA}Cr054s.asc" 0.00710 Cr 54 53.938878305 54.940836427
# Al (8 at% total, single isotope)
"${PKA_DATA}Al027s.asc" 0.08000 Al 27 26.981538408 27.981910193
# Ta (2 at% total, effectively single isotope)
"${PKA_DATA}Ta181s.asc" 0.02000 Ta 181 180.947992000 181.950148000
flux_norm_type=2
pka_filetype=2
do_mtd_sums=.true.
do_ngamma_estimate=.t.
do_global_sums=.t.
do_exclude_light_from_total=.t.
number_pka_files=15
flux_rescale_value=3.25e14
max_global_recoils=800
energies_once_perfile=.t.
EOF

# Run:
../../spectra-pka TaHEA.in

# Output files:
# TaHEA_total_pka.out        - Total PKA spectrum (all species combined)
# TaHEA_element_pka_sums.out - PKA spectra summed by element
# TaHEA_nuclide_pka_sums.out - PKA spectra summed by nuclide
# TaHEA.indexes              - Index mapping for GNUPLOT
# TaHEA_MTd_by_nuclide.out   - DPA contributions per reaction channel

# ==================================================================
# STEP 5: SIMILARLY FOR OTHER KAI ALLOYS
# ==================================================================
# Repeat Step 4 for:
# - FeCoNiCr (equiatomic): Fe25Co25Ni25Cr25
# - Fe30Ni30Cr30Al10 (0Ta-HEA)
# - L12-HEA (13 elements): need ~70 isotope files
# - CCIMA CoNiAlTiTaNb: need Co,Ni,Al,Ti,Ta,Nb isotope files
# - FeCoNiCrAl0.3
# - SS316L (reference)
#
# The L12-HEA with 13 elements is the most complex case.
# It requires PKA data for: Ni, Co, Fe, Cr, Ti, Al, Ta, W, Nb, Mo, Hf, B, Zr
# Total: ~70 isotope files with correctly calculated atomic fractions

# ==================================================================
# STEP 6: KEY OUTPUT INTERPRETATION
# ==================================================================
# The _total_pka.out file contains:
#   Column 1: PKA energy bin center (eV)
#   Column 2: PKA rate (PKAs/s per target atom)
#
# The _MTd_by_nuclide.out file contains:
#   DPA contributions from each reaction channel
#   → This directly shows which nuclear reactions dominate damage
#   → Critical for understanding why HEA composition matters
#
# For Kai's alloys, look specifically for:
# 1. Do Ta/W/Hf (n,2n) reactions contribute significantly to DPA?
# 2. What fraction of DPA comes from transmuting reactions?
# 3. How do α and proton PKA rates compare across compositions?
# 4. Does 59Co(n,γ)60Co capture rate match Kai's paper argument?

echo "Setup complete. See comments above for detailed instructions."
