//====================================================================
// ShieldingRunAction.hh
//
// Accumulates per-run statistics:
//   - Transmitted neutron count and energy spectrum
//   - Transmitted gamma count and spectrum
//   - Total energy deposited in shield
//   - NIEL (Non-Ionizing Energy Loss) in shield
//   - DPA estimate
//====================================================================
#ifndef SHIELDING_RUN_ACTION_HH
#define SHIELDING_RUN_ACTION_HH

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "NeutronSpectrumScorer.hh"
#include <vector>
#include <mutex>
#include <fstream>

class ShieldingRunAction : public G4UserRunAction {
public:
    ShieldingRunAction();
    ~ShieldingRunAction() override;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

    // Called from SteppingAction
    void CountTransmittedNeutron(G4double energy);
    void CountTransmittedGamma(G4double energy);
    void AddEdepInShield(G4double edep);
    void AddNIELInShield(G4double niel);
    void RecordIncidentEnergy(G4double energy);

    // Access the SPECTRA-PKA compatible neutron spectrum scorer
    NeutronSpectrumScorer* GetSpectrumScorer() { return &fSpectrumScorer; }

private:
    // Thread-safe accumulables
    G4Accumulable<G4int>    fNeutronTransmitted{0};
    G4Accumulable<G4int>    fGammaTransmitted{0};
    G4Accumulable<G4double> fTotalEdep{0.0};
    G4Accumulable<G4double> fTotalNIEL{0.0};
    G4Accumulable<G4int>    fNIncident{0};

    // Energy spectrum histogramming (log bins: 10⁻⁸ to 20 MeV)
    static const int kNBins = 200;
    std::vector<G4double> fNeutronSpectrum;
    std::vector<G4double> fGammaSpectrum;
    std::vector<G4double> fIncidentSpectrum;

    G4double LogBinEdge(int bin) const;
    int      FindBin(G4double energy) const;

    // SPECTRA-PKA compatible neutron spectrum scorer
    // Scores track-length flux in the Shield volume, exports 709-group spectrum
    NeutronSpectrumScorer fSpectrumScorer;

    // Master thread global accumulators for multi-thread merging
    static std::mutex fgMutex;
    static std::vector<G4double> fgMasterNeutronSpectrum;
    static std::vector<G4double> fgMasterGammaSpectrum;
    static std::vector<G4double> fgMasterIncidentSpectrum;
    static NeutronSpectrumScorer fgMasterSpectrumScorer;
};

#endif
