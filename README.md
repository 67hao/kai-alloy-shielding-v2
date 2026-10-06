# KaiAlloyShielding

**Geant4-Based Radiation Transport & Shielding Simulations for Prof. Ji-Jung Kai's Novel Nuclear Alloys**

A customized Geant4 framework to simulate the shielding effectiveness of newly developed high-entropy alloys (HEAs) and chemically complex intermetallic alloys (CCIMAs) designed for Generation IV nuclear reactor applications.

---

## Scientific Motivation

Prof. Ji-Jung Kai's group at City University of Hong Kong (Center for Advanced Nuclear Safety and Sustainable Development) develops novel alloys for extreme nuclear environments — specifically for LBE-cooled fast reactors operating at 800°C+. Their work spans:

- **Atomistic simulation**: Deep learning potentials, ML-kMC, DFT
- **Experimental characterization**: TEM, APT, EBSD, XPS
- **Ion irradiation**: MeV-scale ions up to ~10⁷ dpa

**The gap this framework fills**: No radiation transport simulation exists for these alloy compositions. This project bridges from atomic-scale radiation damage understanding to engineering-scale shielding predictions.

---

## Alloy Compositions Implemented

All compositions are extracted directly from published papers with full traceability:

| # | Alloy | Source | ρ (g/cm³) | Elements |
|---|-------|--------|-----------|----------|
| 1 | FeCoNiCr (equiatomic) | Multiple papers | 8.19 | 4 |
| 2 | Fe₃₀Ni₃₀Cr₃₀Al₈Ta₂ | Intermetallics 190 (2026) | 7.65 | 5 |
| 3 | Fe₃₀Ni₃₀Cr₃₀Al₁₀ | Intermetallics 190 (2026) | 7.24 | 4 |
| 4 | L1₂-HEA (13-elem) | JALCOM 1034 (2025) | 7.80 | 13 |
| 5 | CCIMA CoNiAlTiTaNb | JALCOM 1042 (2025) | 7.84 | 7 |
| 6 | FeCoNiCrAl₀.₃ | JALCOM (2025) | 7.65 | 5 |

Reference materials: SS316L, Inconel 718, Pure Fe, Pure Pb, YH₂ moderator, LBE coolant.

---

## Physics

- **Physics list**: `Shielding` (Geant4 reference list)
  - Fritiof + Binary Cascade (high energy hadronic)
  - Bertini cascade (intermediate energy)
  - **G4NeutronHP** (thermal/epithermal neutrons, ENDF/B-VIII.0 evaluated data)
  - G4EmStandardPhysics_option4 (highest-accuracy EM)
- **Neutron sources**: Watt fission (²³⁵U), mono-energetic, D-T fusion (14.1 MeV), log-uniform white
- **Scored quantities**: Neutron/gamma transmission spectra, energy deposition, NIEL, DPA estimates

---

## Build & Run

### Prerequisites
- Geant4 ≥ 11.0 (with data libraries, especially G4NDL for NeutronHP)
- CMake ≥ 3.16
- C++17 compiler
- Python 3 + matplotlib (for plotting)
- ROOT (optional, for histogramming)

### Build
```bash
mkdir build && cd build
cmake -DGeant4_DIR=/path/to/geant4-install/lib/cmake/Geant4 ..
make -j$(nproc)
```

### Quick test
```bash
./KaiShielding run_fission.mac
```

### Full material scan (proof-of-concept data)
```bash
python3 ../batch_scan.py --nEvents 500000 --thickness 20
python3 ../plot_shielding_comparison.py --results-dir results/
```

### Interactive visualization
```bash
./KaiShielding
# Then in Geant4 prompt:
/control/execute vis.mac
```

### Macro commands
```
/shield/material   TaHEA       # Material name (see table above)
/shield/thickness  20 mm        # Slab thickness
/source/spectrum   fission      # fission | mono | DT | white
/source/energy     1.0 MeV      # For mono mode only
```

---

## Output Files

| File | Content |
|------|---------|
| `neutron_transmission_spectrum.csv` | E_low, E_high, E_center, n_transmitted, n_incident, T(E) |
| `gamma_transmission_spectrum.csv` | E_low, E_high, E_center, n_gamma_transmitted |
| `results/summary.csv` | Integrated transmission ratio per material |
| `results/fig1_transmission_spectra.png` | Energy-dependent T(E) comparison |
| `results/fig2_attenuation_coefficient.png` | μ_eff(E) comparison |
| `results/fig3_integrated_comparison.png` | Bar chart ranking |
| `results/fig4_secondary_gammas.png` | Secondary gamma production |

---

## Project Structure

```
KaiAlloyShielding/
├── CMakeLists.txt
├── main.cc
├── README.md
├── batch_scan.py                    # Automated multi-material scan
├── plot_shielding_comparison.py     # Publication-quality figures
├── include/
│   ├── KaiAlloyMaterials.hh         # 6 Kai alloys + 6 references
│   ├── ShieldingDetectorConstruction.hh
│   ├── ShieldingPrimaryGenerator.hh
│   ├── ShieldingRunAction.hh
│   ├── ShieldingSteppingAction.hh
│   └── ShieldingActionInitialization.hh
├── src/
│   ├── KaiAlloyMaterials.cc         # Mass fractions from at% with derivation
│   ├── ShieldingDetectorConstruction.cc
│   ├── ShieldingPrimaryGenerator.cc # Watt fission spectrum sampling
│   ├── ShieldingRunAction.cc        # Transmission tallies + CSV output
│   ├── ShieldingSteppingAction.cc
│   └── ShieldingActionInitialization.cc
└── macros/
    ├── vis.mac                      # Interactive visualization
    ├── run_fission.mac              # Quick fission test
    ├── run_single.mac               # Template for any material
    └── run_scan_materials.mac       # Full 9-material comparison
```

---

## Roadmap

- [x] Phase 1: Material definitions + slab geometry shielding
- [ ] Phase 2: Multi-layer geometry (alloy cladding + LBE coolant)
- [ ] Phase 3: Cylindrical fuel assembly geometry (UO₂/MOX + HEA clad + LBE)
- [ ] Phase 4: Ion irradiation mode (MeV ions for SRIM benchmarking)
- [ ] Phase 5: Activation product inventory (⁶⁰Co from CCIMA, etc.)

---

## Citation

If you use this framework, please cite both:
1. The relevant Prof. Kai publication for the alloy composition
2. The Geant4 toolkit: Agostinelli et al., NIMA 506 (2003) 250-303

---

## Author

TUNG — Geant4 simulation specialist  
Contact: [your email]  
Affiliation: King Fahd University of Petroleum and Minerals (KFUPM)

*Developed as a proof-of-concept for research collaboration with Prof. Ji-Jung Kai, City University of Hong Kong.*
