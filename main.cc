//====================================================================
// main.cc — KaiAlloyShielding
//
// Geant4-based radiation transport and shielding simulation for
// Prof. Ji-Jung Kai's newly developed high-entropy alloys and
// chemically complex intermetallic alloys.
//
// Physics list: "Shielding" (optimized for neutron transport)
//   - Fritiof + Binary Cascade (high energy)
//   - Bertini cascade (intermediate)
//   - G4NeutronHP (thermal/epithermal, ENDF/B data)
//   - G4EmStandardPhysics_option4 (EM with best accuracy)
//
// Usage:
//   ./KaiShielding                        # interactive mode
//   ./KaiShielding run_fission.mac        # batch mode
//   ./KaiShielding run_scan_materials.mac # scan all materials
//====================================================================
#include "ShieldingDetectorConstruction.hh"
#include "ShieldingActionInitialization.hh"

#include "G4RunManagerFactory.hh"
#include "G4PhysListFactory.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"

int main(int argc, char** argv) {

    // --- Run manager (auto-selects MT if available) ---
    auto runManager = G4RunManagerFactory::CreateRunManager(
        G4RunManagerType::Default);
    runManager->SetNumberOfThreads(45); // Fixed to 45 threads

    // --- Physics list: "Shielding" ---
    // Specifically designed for neutron penetration studies.
    // Includes G4NeutronHP for accurate thermal neutron transport
    // using ENDF/B-VIII evaluated nuclear data.
    G4PhysListFactory factory;
    auto physicsList = factory.GetReferencePhysList("Shielding");
    physicsList->SetVerboseLevel(0);
    runManager->SetUserInitialization(physicsList);

    // --- Geometry ---
    runManager->SetUserInitialization(new ShieldingDetectorConstruction());

    // --- User actions ---
    runManager->SetUserInitialization(new ShieldingActionInitialization());

    // --- Interactive or batch mode ---
    G4UImanager* uiManager = G4UImanager::GetUIpointer();

    if (argc > 1) {
        // Batch mode
        G4String command = "/control/execute ";
        G4String macroFile = argv[1];
        uiManager->ApplyCommand(command + macroFile);
    } else {
        // Interactive mode
        auto ui = new G4UIExecutive(argc, argv);
        auto visManager = new G4VisExecutive();
        visManager->Initialize();
        uiManager->ApplyCommand("/control/execute vis.mac");
        ui->SessionStart();
        delete ui;
        delete visManager;
    }

    delete runManager;
    return 0;
}
