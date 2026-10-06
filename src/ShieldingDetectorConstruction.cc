//====================================================================
// ShieldingDetectorConstruction.cc
//====================================================================
#include "ShieldingDetectorConstruction.hh"
#include "KaiAlloyMaterials.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4SDManager.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4PSTrackLength.hh"
#include "G4PSFlatSurfaceCurrent.hh"
#include "G4RunManager.hh"

ShieldingDetectorConstruction::ShieldingDetectorConstruction() {
    DefineCommands();
}

ShieldingDetectorConstruction::~ShieldingDetectorConstruction() {
    delete fMessenger;
}

G4VPhysicalVolume* ShieldingDetectorConstruction::Construct() {
    // Initialize all Kai alloy materials
    KaiAlloyMaterials* mats = KaiAlloyMaterials::GetInstance();
    mats->PrintAll();

    G4NistManager* nist = G4NistManager::Instance();
    G4Material* air = nist->FindOrBuildMaterial("G4_Galactic"); // vacuum

    // Look up the selected shield material
    G4Material* shieldMat = mats->GetMaterialByName(fShieldMatName);
    if (!shieldMat) {
        G4cerr << "*** FATAL: Shield material '" << fShieldMatName
               << "' not found. Defaulting to FeCoNiCr." << G4endl;
        shieldMat = mats->GetFeCoNiCr();
    }

    G4cout << "\n>>> Shield material: " << shieldMat->GetName()
           << "  thickness = " << fShieldThick/mm << " mm\n" << G4endl;

    // ================================================================
    // GEOMETRY LAYOUT (beam along +Z):
    //
    //   Source plane (z=0)
    //        ↓
    //   [ air gap 5mm ] → [ SHIELD SLAB ] → [ air gap 5mm ] → [ SCORER ]
    //
    // The scorer is a thin slab (1 mm) behind the shield to tally
    // transmitted particles and energy deposition.
    // ================================================================

    G4double worldXY   = 400.0*mm;  // transverse size
    G4double worldZ    = 600.0*mm;  // fixed world spanning [-300mm, +300mm]
    G4double gapAfter  = 5.0*mm;
    G4double scorerZ   = 1.0*mm;

    // --- World ---
    auto worldSolid = new G4Box("World",
        worldXY/2, worldXY/2, worldZ/2);
    auto worldLogical = new G4LogicalVolume(worldSolid, air, "World");
    worldLogical->SetVisAttributes(G4VisAttributes::GetInvisible());
    auto worldPhys = new G4PVPlacement(nullptr, {}, worldLogical,
        "World", nullptr, false, 0, true);

    // --- Shield slab ---
    // Shield front face is at Z = 0; extends along +Z to Z = fShieldThick
    G4double shieldZ = fShieldThick;
    auto shieldSolid = new G4Box("Shield",
        worldXY/2 - 10*mm, worldXY/2 - 10*mm, shieldZ/2);
    fShieldLogical = new G4LogicalVolume(shieldSolid, shieldMat, "Shield");
    fShieldLogical->SetVisAttributes(
        new G4VisAttributes(G4Colour(0.2, 0.4, 0.8, 0.6)));

    G4double shieldCenter = shieldZ / 2.0;
    new G4PVPlacement(nullptr, {0, 0, shieldCenter}, fShieldLogical,
        "Shield", worldLogical, false, 0, true);

    // --- Scorer slab (behind shield) ---
    auto scorerSolid = new G4Box("Scorer",
        worldXY/2 - 10*mm, worldXY/2 - 10*mm, scorerZ/2);
    G4Material* scorerMat = air;
    fScorerLogical = new G4LogicalVolume(scorerSolid, scorerMat, "Scorer");
    fScorerLogical->SetVisAttributes(
        new G4VisAttributes(G4Colour(0.8, 0.2, 0.2, 0.4)));

    G4double scorerCenter = shieldZ + gapAfter + scorerZ/2.0;
    new G4PVPlacement(nullptr, {0, 0, scorerCenter}, fScorerLogical,
        "Scorer", worldLogical, false, 0, true);

    return worldPhys;
}

void ShieldingDetectorConstruction::ConstructSDandField() {
    // Multi-functional detector for the scorer volume
    auto det = new G4MultiFunctionalDetector("ScorerDet");
    G4SDManager::GetSDMpointer()->AddNewDetector(det);

    // Energy deposited in scorer
    det->RegisterPrimitive(new G4PSEnergyDeposit("Edep"));

    // Particle current through scorer surface (flat surface, cos-weighted)
    det->RegisterPrimitive(
        new G4PSFlatSurfaceCurrent("NeutronCurrent", fCurrent_In));

    fScorerLogical->SetSensitiveDetector(det);
}

void ShieldingDetectorConstruction::SetShieldMaterial(const G4String& name) {
    fShieldMatName = name;
    if (fShieldLogical) {
        KaiAlloyMaterials* mats = KaiAlloyMaterials::GetInstance();
        G4Material* mat = mats->GetMaterialByName(name);
        if (mat) {
            fShieldLogical->SetMaterial(mat);
            G4RunManager::GetRunManager()->GeometryHasBeenModified();
        }
    }
}

void ShieldingDetectorConstruction::SetShieldThickness(G4double t) {
    fShieldThick = t;
    G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void ShieldingDetectorConstruction::DefineCommands() {
    fMessenger = new G4GenericMessenger(this,
        "/shield/", "Shield geometry control");

    fMessenger->DeclareMethod("material",
        &ShieldingDetectorConstruction::SetShieldMaterial,
        "Set shield material by name (e.g. FeCoNiCr, TaHEA, L12HEA, CCIMA, SS316L)")
        .SetDefaultValue("FeCoNiCr");

    auto& thickCmd = fMessenger->DeclarePropertyWithUnit("thickness",
        "mm", fShieldThick,
        "Set shield slab thickness");
    thickCmd.SetRange("value > 0 && value < 1000");
}
