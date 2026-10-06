//====================================================================
// ShieldingSteppingAction.cc
//
// Tallies:
//   1. Particles entering the Scorer volume (transmitted)
//   2. Energy deposited in the Shield volume
//   3. NIEL in the Shield volume
//   4. Incident spectrum (particles entering the Shield)
//====================================================================
#include "ShieldingSteppingAction.hh"
#include "ShieldingRunAction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ParticleDefinition.hh"
#include "G4Neutron.hh"
#include "G4Gamma.hh"
#include "G4SystemOfUnits.hh"

ShieldingSteppingAction::ShieldingSteppingAction(ShieldingRunAction* ra)
    : fRunAction(ra) {}

void ShieldingSteppingAction::UserSteppingAction(const G4Step* step) {
    // Get volume names
    auto preVol  = step->GetPreStepPoint()->GetTouchableHandle()
                       ->GetVolume();
    auto postPoint = step->GetPostStepPoint();

    if (!preVol) return;
    G4String preVolName = preVol->GetName();

    // --- Score neutron flux spectrum for SPECTRA-PKA ---
    // This tallies track-length in 709 energy groups inside the Shield
    // volume, producing a spectrum directly usable by SPECTRA-PKA
    fRunAction->GetSpectrumScorer()->ScoreStep(step);

    // --- Tally energy deposited in the Shield ---
    if (preVolName == "Shield") {
        G4double edep = step->GetTotalEnergyDeposit();
        if (edep > 0) fRunAction->AddEdepInShield(edep);

        G4double niel = step->GetNonIonizingEnergyDeposit();
        if (niel > 0) fRunAction->AddNIELInShield(niel);
    }

    // --- Record incident particles entering the Shield (primary neutrons only) ---
    if (preVolName == "Shield" &&
        step->GetPreStepPoint()->GetStepStatus() == fGeomBoundary) {
        if (step->GetTrack()->GetTrackID() == 1 &&
            step->GetTrack()->GetParticleDefinition() == G4Neutron::Definition()) {
            G4double kE = step->GetPreStepPoint()->GetKineticEnergy();
            fRunAction->RecordIncidentEnergy(kE);
        }
    }

    // --- Tally particles entering the Scorer (transmitted) ---
    if (preVolName == "Scorer" &&
        step->GetPreStepPoint()->GetStepStatus() == fGeomBoundary) {

        auto particle = step->GetTrack()->GetParticleDefinition();
        G4double kE = step->GetPreStepPoint()->GetKineticEnergy();

        if (particle == G4Neutron::Definition()) {
            fRunAction->CountTransmittedNeutron(kE);
            step->GetTrack()->SetTrackStatus(fStopAndKill);
        }
        else if (particle == G4Gamma::Definition()) {
            fRunAction->CountTransmittedGamma(kE);
            step->GetTrack()->SetTrackStatus(fStopAndKill);
        }
    }
}
