//====================================================================
// NeutronSpectrumScorer.cc
//
// Implementation of the neutron flux spectrum scorer.
// Scores neutron track-length in a target volume, bins into the
// CCFE-709 (or user-defined) energy group structure, and exports
// in the format required by SPECTRA-PKA.
//
// SPECTRA-PKA flux file format (fluxes_specter.dat style):
//   - Lines starting with # are comments
//   - Data lines: upper_energy_bound(eV)  flux_value
//   - Energy in descending order (highest energy first)
//   - 709 data lines for 709-group structure
//   - flux_value: n/cm²/s per energy group
//
// The track-length flux estimator is used because it has better
// statistics than surface-crossing tallies for the same number
// of histories. The flux in group g is:
//   φ_g = (1/V) × Σ_tracks (track_length_in_group_g)
// normalized to per-source-neutron, then multiplied by source rate.
//====================================================================
#include "NeutronSpectrumScorer.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Neutron.hh"
#include "G4VPhysicalVolume.hh"
#include "G4SystemOfUnits.hh"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <numeric>

//--------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------
NeutronSpectrumScorer::NeutronSpectrumScorer(
    const G4String& volumeName, G4int nGroups)
    : fVolumeName(volumeName), fNGroups(nGroups)
{
    // By default, generate logarithmic energy groups
    // spanning the CCFE-709 range: 1e-5 eV to 55.5 MeV
    GenerateLogGroups(1.0e-5, 5.55e7, nGroups);
}

NeutronSpectrumScorer::~NeutronSpectrumScorer() = default;

//--------------------------------------------------------------------
// GenerateLogGroups
//
// Creates a logarithmically-spaced energy group structure.
// The true CCFE-709 structure has non-uniform spacing optimized
// for nuclear data resonances. This function creates a uniform-
// log-spacing approximation that is sufficient for proof-of-concept
// and can be replaced with the exact structure via LoadEnergyGroupsFromFile().
//
// Parameters:
//   Emin: lower edge of first group (eV), default 1e-5 eV
//   Emax: upper edge of last group (eV), default 55.5 MeV
//   nGroups: number of groups, default 709
//--------------------------------------------------------------------
void NeutronSpectrumScorer::GenerateLogGroups(
    G4double Emin, G4double Emax, G4int nGroups)
{
    fNGroups = nGroups;
    fEnergyBins.resize(nGroups + 1);
    fCounts.assign(nGroups, 0.0);
    fTrackLengthTally.assign(nGroups, 0.0);

    // Logarithmic spacing: E_i = Emin * (Emax/Emin)^(i/N)
    // This gives equal spacing on a log scale
    G4double logEmin = std::log(Emin);
    G4double logEmax = std::log(Emax);
    G4double dlogE   = (logEmax - logEmin) / nGroups;

    for (G4int i = 0; i <= nGroups; ++i) {
        fEnergyBins[i] = std::exp(logEmin + i * dlogE);
    }

    G4cout << "NeutronSpectrumScorer: Generated " << nGroups
           << " log-spaced energy groups"
           << " from " << Emin << " eV to " << Emax << " eV"
           << G4endl;
}

//--------------------------------------------------------------------
// LoadEnergyGroupsFromFile
//
// Reads the exact CCFE-709 energy group boundaries from a text file.
// The file should contain one energy value (in eV) per line,
// with N+1 values for N groups.
//
// The official CCFE-709 structure can be downloaded from:
//   https://fispact.ukaea.uk/wiki/CCFE-709_group_structure
//
// Format example:
//   1.00000E-05     ← lower edge of group 0
//   1.04750E-05     ← lower edge of group 1  (= upper edge of group 0)
//   ...
//   5.55000E+07     ← upper edge of last group (group 708)
//--------------------------------------------------------------------
void NeutronSpectrumScorer::LoadEnergyGroupsFromFile(
    const G4String& filename)
{
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        G4cerr << "*** NeutronSpectrumScorer: Cannot open energy group file '"
               << filename << "'. Using default log groups." << G4endl;
        return;
    }

    std::vector<G4double> bins;
    G4double energy;
    G4String line;

    while (std::getline(infile, line)) {
        // Skip comment lines (starting with # or !)
        if (line.empty() || line[0] == '#' || line[0] == '!') continue;

        // Try to parse a number from the line
        std::istringstream iss(line);
        if (iss >> energy) {
            bins.push_back(energy);  // energy in eV
        }
    }
    infile.close();

    if (bins.size() < 2) {
        G4cerr << "*** NeutronSpectrumScorer: File '" << filename
               << "' has fewer than 2 energy values. Using default."
               << G4endl;
        return;
    }

    // Ensure bins are sorted in ascending order
    std::sort(bins.begin(), bins.end());

    // Set up the scorer with the loaded bins
    fNGroups = bins.size() - 1;
    fEnergyBins = bins;
    fCounts.assign(fNGroups, 0.0);
    fTrackLengthTally.assign(fNGroups, 0.0);

    G4cout << "NeutronSpectrumScorer: Loaded " << fNGroups
           << " energy groups from '" << filename << "'"
           << " range: " << fEnergyBins.front() << " to "
           << fEnergyBins.back() << " eV" << G4endl;
}

//--------------------------------------------------------------------
// FindGroup
//
// Binary search for the energy group index containing a given energy.
// Energy is in Geant4 internal units (MeV), converted to eV for
// comparison with fEnergyBins.
//
// Returns: group index [0, fNGroups-1], or -1 if outside range
//--------------------------------------------------------------------
G4int NeutronSpectrumScorer::FindGroup(G4double energy_MeV) const
{
    // Convert Geant4 internal units (MeV) to eV for group lookup
    G4double E_eV = energy_MeV / eV;  // eV = 1e-6 * MeV in G4 units

    // Check bounds
    if (E_eV < fEnergyBins.front()) return -1;  // below range
    if (E_eV >= fEnergyBins.back())  return -2;  // above range

    // Binary search: find the group whose [lower, upper) contains E_eV
    // std::upper_bound returns iterator to first element > E_eV
    auto it = std::upper_bound(fEnergyBins.begin(), fEnergyBins.end(), E_eV);

    // The group index is (it - begin - 1), clamped to valid range
    G4int idx = static_cast<G4int>(it - fEnergyBins.begin()) - 1;

    // Safety clamp
    if (idx < 0) idx = 0;
    if (idx >= fNGroups) idx = fNGroups - 1;

    return idx;
}

//--------------------------------------------------------------------
// ScoreStep
//
// Called from ShieldingSteppingAction for every simulation step.
// Scores the neutron track-length flux estimator:
//
//   For each step of a neutron inside the target volume:
//     1. Determine the neutron's kinetic energy (at pre-step point)
//     2. Find the corresponding energy group
//     3. Add the step length to that group's tally
//
// The track-length estimator is:
//   φ_g = (1/V) × Σ (step_length for neutrons in group g)
// This is statistically more efficient than counting boundary crossings.
//
// Returns true if this step was scored (neutron in target volume).
//--------------------------------------------------------------------
G4bool NeutronSpectrumScorer::ScoreStep(const G4Step* step)
{
    // ---- Filter: only neutrons ----
    if (step->GetTrack()->GetParticleDefinition() != G4Neutron::Definition())
        return false;

    // ---- Filter: only in the target volume ----
    auto preVol = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
    if (!preVol || preVol->GetName() != fVolumeName)
        return false;

    // ---- Get neutron kinetic energy at the start of this step ----
    // Using pre-step point energy (conventional for track-length tallies)
    G4double kineticEnergy = step->GetPreStepPoint()->GetKineticEnergy();

    // ---- Find the energy group ----
    G4int group = FindGroup(kineticEnergy);

    if (group == -1) {
        // Below energy range — count but don't score
        fNeutronsBelowRange++;
        return false;
    }
    if (group == -2) {
        // Above energy range — count but don't score
        fNeutronsAboveRange++;
        return false;
    }

    // ---- Score the track length in this group ----
    G4double stepLength = step->GetStepLength();  // in mm (Geant4 units)

    {
        std::lock_guard<std::mutex> lock(fMutex);
        // Track-length tally: sum of step lengths (will normalize later)
        fTrackLengthTally[group] += stepLength / cm;  // convert mm → cm
        // Also count number of contributions (for statistics)
        fCounts[group] += 1.0;
    }

    fTotalNeutronsCounted++;
    return true;
}

//--------------------------------------------------------------------
// WriteSpectrum
//
// Exports the scored neutron spectrum in SPECTRA-PKA compatible format.
//
// SPECTRA-PKA expects the "SPECTER-like" flux format:
//   - Comment lines starting with #
//   - Data: two columns per line
//     Column 1: upper energy boundary of group (eV)
//     Column 2: neutron flux in that group (n/cm²/s)
//   - Groups listed from HIGHEST energy to LOWEST
//     (this is the SPECTER convention, descending energy order)
//   - The flux is the integrated flux per group, NOT per unit energy
//
// Normalization:
//   flux_g = (track_length_tally_g / volume_cm3) × (source_rate / N_histories)
//
//   But in practice, SPECTRA-PKA will re-normalize using its own
//   flux_rescale_value parameter. So we output the relative spectrum
//   shape normalized per source neutron, and let the user set the
//   absolute flux via flux_rescale_value in the .in file.
//
// Parameters:
//   filename:      output file path
//   totalNeutrons: number of source neutrons simulated
//   crossArea:     beam cross-section area (cm²), for flux conversion
//   runTime:       simulated real time (s); if >0, normalize to absolute flux
//--------------------------------------------------------------------
void NeutronSpectrumScorer::WriteSpectrum(
    const G4String& filename,
    G4int totalNeutrons,
    G4double crossArea,
    G4double runTime) const
{
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        G4cerr << "*** NeutronSpectrumScorer: Cannot open output file '"
               << filename << "'" << G4endl;
        return;
    }

    // ---- Header comments ----
    ofs << "# ================================================================\n"
        << "# Neutron flux spectrum for SPECTRA-PKA\n"
        << "# Generated by KaiAlloyShielding Geant4 simulation\n"
        << "# ================================================================\n"
        << "# Scored volume: " << fVolumeName << "\n"
        << "# Number of energy groups: " << fNGroups << "\n"
        << "# Energy range: " << fEnergyBins.front() << " to "
        << fEnergyBins.back() << " eV\n"
        << "# Source neutrons simulated: " << totalNeutrons << "\n"
        << "# Scoring area (cm²): " << crossArea << "\n";

    if (runTime > 0) {
        ofs << "# Simulated time (s): " << runTime << "\n"
            << "# Flux units: n/cm²/s per energy group (ABSOLUTE)\n";
    } else {
        ofs << "# Flux units: relative (per source neutron per group)\n"
            << "# NOTE: Set flux_rescale_value in SPECTRA-PKA .in file\n"
            << "#       to the actual total flux (n/cm²/s) for your scenario\n";
    }

    ofs << "# Format: upper_energy(eV)  flux(n/cm²/s/group)\n"
        << "# Energy ordering: DESCENDING (SPECTER convention)\n"
        << "# ================================================================\n";

    // ---- Calculate normalization factor ----
    // Track-length flux = (sum of track lengths in cm) / (volume in cm³)
    //                   / (number of source particles)
    // Then multiply by source rate to get absolute flux
    //
    // However, for SPECTRA-PKA, what matters is the RELATIVE shape.
    // The absolute scaling is done via flux_rescale_value.
    // So we normalize to: counts per source neutron
    G4double norm = 1.0;
    if (totalNeutrons > 0) {
        norm = 1.0 / totalNeutrons;
    }
    if (runTime > 0 && crossArea > 0) {
        // Absolute flux: n/cm²/s = (track_length_tally / volume) * (source_rate)
        // But we don't know volume easily, so use surface crossing approximation:
        // flux ≈ counts / (area × Δt)
        norm = 1.0 / (crossArea * runTime);
    }

    // ---- Write data in DESCENDING energy order ----
    // SPECTRA-PKA expects groups from high to low energy
    ofs << std::scientific << std::setprecision(6);

    for (G4int g = fNGroups - 1; g >= 0; --g) {
        // Upper edge of group g
        G4double upperE = fEnergyBins[g + 1];

        // Flux value for this group
        // Using track-length tally normalized by source neutrons
        G4double flux = fTrackLengthTally[g] * norm;

        ofs << std::setw(14) << upperE << "  "
            << std::setw(14) << flux << "\n";
    }

    ofs.close();

    G4cout << ">>> Neutron spectrum written to: " << filename
           << " (" << fNGroups << " groups, SPECTRA-PKA compatible)"
           << G4endl;
}

//--------------------------------------------------------------------
// WriteSummary
//
// Writes a human-readable summary with statistics about the scored
// spectrum, including total counts, peak energy group, and warnings
// about out-of-range neutrons.
//--------------------------------------------------------------------
void NeutronSpectrumScorer::WriteSummary(const G4String& filename) const
{
    std::ofstream ofs(filename);
    if (!ofs.is_open()) return;

    G4double totalCounts = GetTotalCounts();
    G4double totalTL = std::accumulate(
        fTrackLengthTally.begin(), fTrackLengthTally.end(), 0.0);

    // Find peak group
    G4int peakGroup = 0;
    G4double peakVal = 0;
    for (G4int g = 0; g < fNGroups; ++g) {
        if (fTrackLengthTally[g] > peakVal) {
            peakVal = fTrackLengthTally[g];
            peakGroup = g;
        }
    }

    ofs << "================================================================\n"
        << "  NEUTRON SPECTRUM SCORER SUMMARY\n"
        << "================================================================\n"
        << "  Scored volume:           " << fVolumeName << "\n"
        << "  Energy groups:           " << fNGroups << "\n"
        << "  Energy range:            " << fEnergyBins.front() << " – "
        << fEnergyBins.back() << " eV\n"
        << "\n"
        << "  Total neutron steps scored:  " << fTotalNeutronsCounted.load() << "\n"
        << "  Total step contributions:    " << totalCounts << "\n"
        << "  Total track length (cm):     " << totalTL << "\n"
        << "  Neutrons below range:        " << fNeutronsBelowRange.load() << "\n"
        << "  Neutrons above range:        " << fNeutronsAboveRange.load() << "\n"
        << "\n"
        << "  Peak energy group:  " << peakGroup
        << "  [" << fEnergyBins[peakGroup] << " – "
        << fEnergyBins[peakGroup + 1] << " eV]\n"
        << "  Peak track-length:  " << peakVal << " cm\n"
        << "================================================================\n";

    // Per-group detailed table (first 20 + last 20 + peak region)
    ofs << "\n  GROUP DETAILS (selected):\n"
        << "  " << std::setw(6) << "Group"
        << std::setw(14) << "E_low (eV)"
        << std::setw(14) << "E_high (eV)"
        << std::setw(14) << "Counts"
        << std::setw(14) << "TrackLen(cm)"
        << "\n";

    for (G4int g = 0; g < fNGroups; ++g) {
        // Print if: first 10, last 10, or near peak ±5
        G4bool print = (g < 10) || (g >= fNGroups - 10) ||
                       (std::abs(g - peakGroup) <= 5);
        if (!print) {
            if (g == 10) ofs << "  ...  (intermediate groups omitted)  ...\n";
            if (g == peakGroup - 6) ofs << "  ...  (peak region)  ...\n";
            continue;
        }
        ofs << "  " << std::setw(6) << g
            << std::scientific << std::setprecision(4)
            << std::setw(14) << fEnergyBins[g]
            << std::setw(14) << fEnergyBins[g + 1]
            << std::setw(14) << fCounts[g]
            << std::setw(14) << fTrackLengthTally[g]
            << "\n";
    }

    ofs.close();
    G4cout << ">>> Spectrum summary written to: " << filename << G4endl;
}

//--------------------------------------------------------------------
// Reset — clear all tallies for a new run
//--------------------------------------------------------------------
void NeutronSpectrumScorer::Reset()
{
    std::lock_guard<std::mutex> lock(fMutex);
    std::fill(fCounts.begin(), fCounts.end(), 0.0);
    std::fill(fTrackLengthTally.begin(), fTrackLengthTally.end(), 0.0);
    fTotalNeutronsCounted = 0;
    fNeutronsAboveRange = 0;
    fNeutronsBelowRange = 0;
}

//--------------------------------------------------------------------
// Merge — combine data from another scorer (for multi-threading)
//--------------------------------------------------------------------
void NeutronSpectrumScorer::Merge(const NeutronSpectrumScorer& other)
{
    std::lock_guard<std::mutex> lock(fMutex);
    for (G4int g = 0; g < fNGroups && g < other.fNGroups; ++g) {
        fCounts[g]            += other.fCounts[g];
        fTrackLengthTally[g]  += other.fTrackLengthTally[g];
    }
    fTotalNeutronsCounted += other.fTotalNeutronsCounted.load();
    fNeutronsAboveRange   += other.fNeutronsAboveRange.load();
    fNeutronsBelowRange   += other.fNeutronsBelowRange.load();
}

//--------------------------------------------------------------------
// GetTotalCounts
//--------------------------------------------------------------------
G4double NeutronSpectrumScorer::GetTotalCounts() const
{
    return std::accumulate(fCounts.begin(), fCounts.end(), 0.0);
}
