//====================================================================
// ShieldingPrimaryGenerator.cc
//====================================================================
#include "ShieldingPrimaryGenerator.hh"

#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"

#include <cmath>

ShieldingPrimaryGenerator::ShieldingPrimaryGenerator() {
    fGun = new G4ParticleGun(1);

    auto neutron = G4ParticleTable::GetParticleTable()->FindParticle("neutron");
    fGun->SetParticleDefinition(neutron);
    fGun->SetParticleMomentumDirection({0, 0, 1});  // +Z direction

    DefineCommands();
}

ShieldingPrimaryGenerator::~ShieldingPrimaryGenerator() {
    delete fGun;
    delete fMessenger;
}

void ShieldingPrimaryGenerator::GeneratePrimaries(G4Event* event) {
    // Source position: 50 mm in front of the shield front face (shield front face is at Z=0)
    // Safely inside world volume which extends from -300 mm to +300 mm
    fGun->SetParticlePosition({0, 0, -50.0*mm});

    // Energy selection based on spectrum type
    G4double energy = 1.0*MeV;

    if (fSpectrumType == "mono") {
        energy = fMonoEnergy * MeV;
    }
    else if (fSpectrumType == "fission") {
        energy = SampleWattSpectrum() * MeV;
    }
    else if (fSpectrumType == "DT") {
        // D-T fusion: 14.1 MeV with small Gaussian spread
        energy = G4RandGauss::shoot(14.1, 0.05) * MeV;
    }
    else if (fSpectrumType == "white") {
        // Log-uniform from 0.01 eV to 20 MeV (covers thermal to fast)
        G4double logEmin = std::log(0.01e-6);  // 0.01 eV in MeV
        G4double logEmax = std::log(20.0);     // 20 MeV
        G4double logE = logEmin + G4UniformRand() * (logEmax - logEmin);
        energy = std::exp(logE) * MeV;
    }

    fGun->SetParticleEnergy(energy);
    fGun->GeneratePrimaryVertex(event);
}

//--------------------------------------------------------------------
// Watt fission spectrum sampling
// P(E) = C * exp(-E/a) * sinh(sqrt(b*E))
//
// Exact physical formulation (B.E. Watt, Phys. Rev. 87, 1037 (1952)):
// Neutrons evaporate isotropically with Maxwellian energy Em (temp a)
// from fragments moving with drift energy Ed = a^2 * b / 4.
// In the laboratory frame:
//   E = Em + Ed + 2*sqrt(Em*Ed)*cos(theta)
// where cos(theta) is uniformly distributed on [-1, 1].
// This provides exact sampling with 100% efficiency and zero rejection.
//--------------------------------------------------------------------
G4double ShieldingPrimaryGenerator::SampleWattSpectrum() const {
    G4double a = fWattA; // default 0.988 MeV
    G4double b = fWattB; // default 2.249 MeV^-1
    G4double Ed = 0.25 * a * a * b;

    // Em ~ Maxwellian(a): sum of 3 squared Gaussian deviates with sigma = sqrt(a/2)
    G4double sigma = std::sqrt(a * 0.5);
    G4double gx = G4RandGauss::shoot(0.0, sigma);
    G4double gy = G4RandGauss::shoot(0.0, sigma);
    G4double gz = G4RandGauss::shoot(0.0, sigma);
    G4double Em = gx*gx + gy*gy + gz*gz;

    // Isotropic emission angle in the fragment rest frame
    G4double cosTheta = 2.0 * G4UniformRand() - 1.0;

    G4double E = Em + Ed + 2.0 * std::sqrt(Em * Ed) * cosTheta;
    return (E > 0.0) ? E : 0.0;
}

void ShieldingPrimaryGenerator::DefineCommands() {
    fMessenger = new G4GenericMessenger(this,
        "/source/", "Neutron source configuration");

    fMessenger->DeclareProperty("spectrum", fSpectrumType,
        "Spectrum type: mono, fission, DT, white")
        .SetDefaultValue("fission");

    fMessenger->DeclarePropertyWithUnit("energy", "MeV", fMonoEnergy,
        "Mono-energetic neutron energy (used when spectrum=mono)")
        .SetRange("value > 0");
}
