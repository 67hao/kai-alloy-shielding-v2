//====================================================================
// ShieldingPrimaryGenerator.hh / .cc
//
// Neutron source with configurable spectrum:
//   - Mono-energetic (default: 1 MeV)
//   - Watt fission spectrum: P(E) ~ exp(-E/a) * sinh(sqrt(b*E))
//     with a = 0.988 MeV, b = 2.249 /MeV (²³⁵U thermal fission)
//   - 14.1 MeV (D-T fusion neutrons)
//   - Flat white spectrum (0.01 eV – 20 MeV, for attenuation curves)
//
// Beam geometry: parallel beam or isotropic point source.
//====================================================================
#ifndef SHIELDING_PRIMARY_GENERATOR_HH
#define SHIELDING_PRIMARY_GENERATOR_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4GenericMessenger.hh"

class ShieldingPrimaryGenerator : public G4VUserPrimaryGeneratorAction {
public:
    ShieldingPrimaryGenerator();
    ~ShieldingPrimaryGenerator() override;

    void GeneratePrimaries(G4Event* event) override;

private:
    void DefineCommands();
    G4double SampleWattSpectrum() const;

    G4ParticleGun*      fGun       = nullptr;
    G4GenericMessenger*  fMessenger = nullptr;

    G4String fSpectrumType = "fission";    // "mono", "fission", "DT", "white"
    G4double fMonoEnergy   = 1.0;          // MeV (for mono mode)

    // Watt fission spectrum parameters (²³⁵U thermal)
    static constexpr G4double fWattA = 0.988;   // MeV
    static constexpr G4double fWattB = 2.249;   // 1/MeV
};

#endif
