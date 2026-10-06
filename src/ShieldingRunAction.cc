#include "ShieldingRunAction.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "G4AccumulableManager.hh"
#include "G4Threading.hh"

#include <iostream>
#include <iomanip>
#include <cmath>

// Define static master accumulators
std::mutex ShieldingRunAction::fgMutex;
std::vector<G4double> ShieldingRunAction::fgMasterNeutronSpectrum(ShieldingRunAction::kNBins, 0.0);
std::vector<G4double> ShieldingRunAction::fgMasterGammaSpectrum(ShieldingRunAction::kNBins, 0.0);
std::vector<G4double> ShieldingRunAction::fgMasterIncidentSpectrum(ShieldingRunAction::kNBins, 0.0);
NeutronSpectrumScorer ShieldingRunAction::fgMasterSpectrumScorer("Shield", 709);

ShieldingRunAction::ShieldingRunAction()
    : fNeutronSpectrum(kNBins, 0.0),
      fGammaSpectrum(kNBins, 0.0),
      fIncidentSpectrum(kNBins, 0.0),
      fSpectrumScorer("Shield", 709)  // Score in Shield volume, 709 groups
{
    auto mgr = G4AccumulableManager::Instance();
    mgr->RegisterAccumulable(fNeutronTransmitted);
    mgr->RegisterAccumulable(fGammaTransmitted);
    mgr->RegisterAccumulable(fTotalEdep);
    mgr->RegisterAccumulable(fTotalNIEL);
    mgr->RegisterAccumulable(fNIncident);
}

ShieldingRunAction::~ShieldingRunAction() = default;

void ShieldingRunAction::BeginOfRunAction(const G4Run*) {
    G4AccumulableManager::Instance()->Reset();
    std::fill(fNeutronSpectrum.begin(), fNeutronSpectrum.end(), 0.0);
    std::fill(fGammaSpectrum.begin(), fGammaSpectrum.end(), 0.0);
    std::fill(fIncidentSpectrum.begin(), fIncidentSpectrum.end(), 0.0);
    fSpectrumScorer.Reset();  // Reset thread-local 709-group spectrum scorer

    if (IsMaster()) {
        std::lock_guard<std::mutex> lock(fgMutex);
        std::fill(fgMasterNeutronSpectrum.begin(), fgMasterNeutronSpectrum.end(), 0.0);
        std::fill(fgMasterGammaSpectrum.begin(), fgMasterGammaSpectrum.end(), 0.0);
        std::fill(fgMasterIncidentSpectrum.begin(), fgMasterIncidentSpectrum.end(), 0.0);
        fgMasterSpectrumScorer.Reset();
    }
}

void ShieldingRunAction::EndOfRunAction(const G4Run* run) {
    G4int nEvents = run->GetNumberOfEvent();
    if (nEvents == 0) return;

    // Merge accumulables from worker threads
    G4AccumulableManager::Instance()->Merge();

    // If worker thread in MT mode: merge local spectra into master buffer
    if (!IsMaster()) {
        std::lock_guard<std::mutex> lock(fgMutex);
        for (int i = 0; i < kNBins; ++i) {
            fgMasterNeutronSpectrum[i]  += fNeutronSpectrum[i];
            fgMasterGammaSpectrum[i]    += fGammaSpectrum[i];
            fgMasterIncidentSpectrum[i] += fIncidentSpectrum[i];
        }
        fgMasterSpectrumScorer.Merge(fSpectrumScorer);
        return;
    }

    // If sequential execution (single-thread), copy local to master buffer
    if (!G4Threading::IsMultithreadedApplication()) {
        fgMasterNeutronSpectrum  = fNeutronSpectrum;
        fgMasterGammaSpectrum    = fGammaSpectrum;
        fgMasterIncidentSpectrum = fIncidentSpectrum;
        fgMasterSpectrumScorer   = fSpectrumScorer;
    }

    // Master thread prints and exports merged results
    G4int    nTrans_n = fNeutronTransmitted.GetValue();
    G4int    nTrans_g = fGammaTransmitted.GetValue();
    G4double edep     = fTotalEdep.GetValue();
    G4double niel     = fTotalNIEL.GetValue();
    G4int    nInc     = fNIncident.GetValue();

    G4double transRatio_n = (nInc > 0) ? G4double(nTrans_n)/nInc : 0;
    G4double transRatio_g = (nInc > 0) ? G4double(nTrans_g)/nInc : 0;

    G4cout << "\n"
    << "============================================================\n"
    << "  SHIELDING RESULTS SUMMARY (45 Threads Merged)\n"
    << "============================================================\n"
    << "  Incident neutrons:       " << nInc << "\n"
    << "  Transmitted neutrons:    " << nTrans_n
    << "  (ratio: " << std::scientific << std::setprecision(4)
    << transRatio_n << ")\n"
    << "  Transmitted gammas:      " << nTrans_g
    << "  (ratio: " << transRatio_g << ")\n"
    << std::fixed << std::setprecision(4)
    << "  Total Edep in shield:    " << edep/MeV << " MeV\n"
    << "  Total NIEL in shield:    " << niel/keV << " keV\n"
    << "  Avg Edep/neutron:        " << (nInc>0 ? edep/nInc/MeV : 0) << " MeV\n";

    // Estimate linear attenuation coefficient
    if (transRatio_n > 0 && transRatio_n < 1.0) {
        G4cout << "  -ln(T_neutron):          "
               << -std::log(transRatio_n) << "\n"
               << "  (divide by thickness in cm for μ [cm⁻¹])\n";
    }
    G4cout << "============================================================\n"
           << G4endl;

    // Write transmitted neutron spectrum to CSV from merged master buffer
    {
        std::ofstream ofs("neutron_transmission_spectrum.csv");
        ofs << "# E_low(MeV),E_high(MeV),E_center(MeV),transmitted_neutrons,incident_neutrons,transmission_ratio\n";
        for (int i = 0; i < kNBins; ++i) {
            G4double eLow  = LogBinEdge(i) / MeV;
            G4double eHigh = LogBinEdge(i+1) / MeV;
            G4double eCenter = std::sqrt(eLow * eHigh); // geometric mean
            G4double inc  = fgMasterIncidentSpectrum[i];
            G4double trans = fgMasterNeutronSpectrum[i];
            G4double ratio = (inc > 0) ? trans/inc : 0;
            ofs << std::scientific << std::setprecision(6)
                << eLow << "," << eHigh << "," << eCenter << ","
                << trans << "," << inc << "," << ratio << "\n";
        }
        G4cout << ">>> Neutron transmission spectrum written to: "
               << "neutron_transmission_spectrum.csv" << G4endl;
    }

    // Write transmitted gamma spectrum from merged master buffer
    {
        std::ofstream ofs("gamma_transmission_spectrum.csv");
        ofs << "# E_low(MeV),E_high(MeV),E_center(MeV),transmitted_gammas\n";
        for (int i = 0; i < kNBins; ++i) {
            G4double eLow  = LogBinEdge(i) / MeV;
            G4double eHigh = LogBinEdge(i+1) / MeV;
            G4double eCenter = std::sqrt(eLow * eHigh);
            ofs << std::scientific << std::setprecision(6)
                << eLow << "," << eHigh << "," << eCenter << ","
                << fgMasterGammaSpectrum[i] << "\n";
        }
    }

    // ================================================================
    // SPECTRA-PKA COMPATIBLE SPECTRUM OUTPUT
    // ================================================================
    fgMasterSpectrumScorer.WriteSpectrum(
        "neutron_flux_for_spectra_pka.dat",  // Output filename
        nEvents,                              // Total source neutrons
        1.0,                                  // Cross-section area (cm²)
        -1.0                                  // Negative = relative output
    );
    fgMasterSpectrumScorer.WriteSummary("neutron_flux_summary.txt");
}

// Log-uniform binning: 10⁻⁸ MeV (0.01 eV) to 20 MeV
G4double ShieldingRunAction::LogBinEdge(int bin) const {
    static const G4double logMin = std::log(1.0e-8 * MeV);
    static const G4double logMax = std::log(20.0 * MeV);
    return std::exp(logMin + bin * (logMax - logMin) / kNBins);
}

int ShieldingRunAction::FindBin(G4double energy) const {
    if (energy <= 0) return -1;
    static const G4double logMin = std::log(1.0e-8 * MeV);
    static const G4double logMax = std::log(20.0 * MeV);
    G4double logE = std::log(energy);
    int bin = int((logE - logMin) / (logMax - logMin) * kNBins);
    if (bin < 0) bin = 0;
    if (bin >= kNBins) bin = kNBins - 1;
    return bin;
}

void ShieldingRunAction::CountTransmittedNeutron(G4double energy) {
    fNeutronTransmitted++;
    int bin = FindBin(energy);
    if (bin >= 0) fNeutronSpectrum[bin] += 1.0;
}

void ShieldingRunAction::CountTransmittedGamma(G4double energy) {
    fGammaTransmitted++;
    int bin = FindBin(energy);
    if (bin >= 0) fGammaSpectrum[bin] += 1.0;
}

void ShieldingRunAction::AddEdepInShield(G4double edep) {
    fTotalEdep += edep;
}

void ShieldingRunAction::AddNIELInShield(G4double niel) {
    fTotalNIEL += niel;
}

void ShieldingRunAction::RecordIncidentEnergy(G4double energy) {
    fNIncident++;
    int bin = FindBin(energy);
    if (bin >= 0) fIncidentSpectrum[bin] += 1.0;
}
