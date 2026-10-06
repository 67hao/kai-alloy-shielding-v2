//====================================================================
// ShieldingDetectorConstruction.hh
//
// Configurable multi-layer slab geometry for shielding studies.
// Supports: single-slab, multi-layer (alloy + LBE + alloy), and
// full LFR component (fuel/clad/coolant/wrapper) configurations.
//====================================================================
#ifndef SHIELDING_DETECTOR_CONSTRUCTION_HH
#define SHIELDING_DETECTOR_CONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4GenericMessenger.hh"

class ShieldingDetectorConstruction : public G4VUserDetectorConstruction {
public:
    ShieldingDetectorConstruction();
    ~ShieldingDetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    // Setters for macro commands
    void SetShieldMaterial(const G4String& name);
    void SetShieldThickness(G4double t);

private:
    void DefineCommands();

    G4GenericMessenger* fMessenger = nullptr;

    G4String  fShieldMatName  = "FeCoNiCr";
    G4double  fShieldThick    = 10.0;  // mm

    G4LogicalVolume* fShieldLogical  = nullptr;
    G4LogicalVolume* fScorerLogical  = nullptr;
};

#endif
