//====================================================================
// NeutronSpectrumScorer.hh
//
// Scores the neutron flux spectrum at a specified surface/volume
// and exports it in the CCFE-709 group structure format compatible
// with SPECTRA-PKA input.
//
// The CCFE-709 group structure is the standard energy binning used
// by FISPACT-II and SPECTRA-PKA for nuclear data processing.
// It spans from 1.0e-5 eV to 55.5 MeV with 709 logarithmically-
// spaced energy groups, specifically designed to resolve resonance
// structures in nuclear cross-sections.
//
// This scorer can operate in two modes:
//   1. BUILT-IN: Generate a 709-bin log-spaced structure
//      (approximation of CCFE-709, suitable for proof-of-concept)
//   2. FILE-BASED: Read exact CCFE-709 bin edges from a file
//      (download from https://fispact.ukaea.uk/wiki/CCFE-709_group_structure)
//
// Output: text file directly usable as flux_filename in SPECTRA-PKA
//
// Author: Nghia
//====================================================================
#ifndef NEUTRON_SPECTRUM_SCORER_HH
#define NEUTRON_SPECTRUM_SCORER_HH

#include "G4UserSteppingAction.hh"
#include "G4GenericMessenger.hh"
#include "G4ThreeVector.hh"

#include <vector>
#include <string>
#include <mutex>
#include <atomic>

// Forward declarations
class ShieldingRunAction;

class NeutronSpectrumScorer {
public:
    // ---- Constructor ----
    // volumeName: name of the G4 volume where flux is scored
    // nGroups:    number of energy bins (709 for CCFE standard)
    NeutronSpectrumScorer(const G4String& volumeName = "Shield",
                          G4int nGroups = 709);
    ~NeutronSpectrumScorer();

    // ---- Configuration ----
    // Load exact CCFE-709 energy group boundaries from file
    // Format: one energy value (eV) per line, 710 values for 709 groups
    // (N+1 boundaries for N groups)
    void LoadEnergyGroupsFromFile(const G4String& filename);

    // Generate logarithmic energy groups (built-in approximation)
    // Emin and Emax in eV
    void GenerateLogGroups(G4double Emin = 1.0e-5,
                           G4double Emax = 5.55e7,
                           G4int nGroups = 709);

    // ---- Scoring interface ----
    // Called from SteppingAction for every neutron step
    // Returns true if the step was scored (neutron in target volume)
    G4bool ScoreStep(const G4Step* step);

    // ---- Output ----
    // Write the scored spectrum to file in SPECTRA-PKA format
    // Must be called from EndOfRunAction (master thread only)
    //
    // Parameters:
    //   filename:     output file path
    //   totalNeutrons: total number of source neutrons (for normalization)
    //   crossArea:    cross-sectional area of scoring surface (cm²)
    //                 used to convert counts to flux (n/cm²/s)
    //   runTime:      simulated real time (seconds), for flux normalization
    //                 if <= 0, output is in counts-per-source-neutron
    void WriteSpectrum(const G4String& filename,
                       G4int totalNeutrons,
                       G4double crossArea = 1.0,
                       G4double runTime = -1.0) const;

    // Write also a human-readable summary with statistics
    void WriteSummary(const G4String& filename) const;

    // ---- Reset for new run ----
    void Reset();

    // ---- Thread-safe merge ----
    // Merge another scorer's data into this one (for MT mode)
    void Merge(const NeutronSpectrumScorer& other);

    // ---- Getters ----
    G4int GetNGroups() const { return fNGroups; }
    G4double GetTotalCounts() const;

private:
    G4String fVolumeName;       // Name of G4 volume to score in
    G4int    fNGroups;          // Number of energy groups (default 709)

    // Energy group boundaries: fEnergyBins has (fNGroups + 1) entries
    // fEnergyBins[i] = lower edge of group i (in eV)
    // fEnergyBins[fNGroups] = upper edge of last group
    // Groups are indexed 0..fNGroups-1
    // Group i spans [fEnergyBins[i], fEnergyBins[i+1])
    std::vector<G4double> fEnergyBins;  // in eV, (nGroups+1) entries

    // Scored neutron counts per energy group
    // Thread-safe via mutex
    std::vector<G4double> fCounts;
    mutable std::mutex    fMutex;

    // Track-length based flux tally (alternative to crossing tally)
    // Stores sum of track_length / volume for each energy bin
    std::vector<G4double> fTrackLengthTally;

    // Statistics
    std::atomic<G4int> fTotalNeutronsCounted{0};
    std::atomic<G4int> fNeutronsAboveRange{0};
    std::atomic<G4int> fNeutronsBelowRange{0};

    // ---- Internal ----
    // Find the energy group index for a given kinetic energy (in Geant4 units)
    // Returns -1 if outside range
    G4int FindGroup(G4double energy_MeV) const;
};

#endif
