//====================================================================
// KaiAlloyMaterials.hh
//
// Defines all alloy compositions from Prof. Ji-Jung Kai's published
// papers (2023–2026), plus reference materials (SS316L, Inconel 718,
// LBE, YH2). Compositions are converted from published atomic
// percentages to mass fractions with Vegard's-law density estimates.
//
// Source papers:
//   [P1] Intermetallics 190 (2026) 109170 — Fe30Ni30Cr30Al(10-x)Tax
//   [P2] Intermetallics 191 (2026) 109193 — FeCoNiCr at 107 dpa
//   [P3] J. Nucl. Mater. 623 (2026) 156439 — YH2 deep potential
//   [P4] J. Nucl. Mater. 613 (2025) 155862 — FeCoNiCr minor alloying
//   [P5] JALCOM 1043 (2025) 184264 — EHEA radiation defects
//   [P6] JALCOM 1034 (2025) 181380 — L12-HEA oxidation
//   [P7] JALCOM 1042 (2025) 183893 — CCIMA in LBE at 800°C
//   [P8] JALCOM (2025) — FeCoNiCrAl0.3 in CO2/CO
//   [P9] Corros. Sci. 230 (2024) 111948 — L12-HEA embrittlement
//
// Author: Nghia
//====================================================================
#ifndef KAI_ALLOY_MATERIALS_HH
#define KAI_ALLOY_MATERIALS_HH

#include "G4Material.hh"

class KaiAlloyMaterials {
public:
    static KaiAlloyMaterials* GetInstance();
    ~KaiAlloyMaterials() = default;

    // --- Prof. Kai's alloy compositions ---
    G4Material* GetFeCoNiCr()         const { return fFeCoNiCr; }
    G4Material* GetTaHEA()            const { return fTaHEA; }
    G4Material* Get0TaHEA()           const { return f0TaHEA; }
    G4Material* GetL12HEA()           const { return fL12HEA; }
    G4Material* GetCCIMA()            const { return fCCIMA; }
    G4Material* GetFeCoNiCrAl03()     const { return fFeCoNiCrAl03; }

    // --- Reference / environment materials ---
    G4Material* GetYH2()              const { return fYH2; }
    G4Material* GetLBE()              const { return fLBE; }
    G4Material* GetSS316L()           const { return fSS316L; }
    G4Material* GetInconel718()       const { return fInconel718; }
    G4Material* GetPureFe()           const { return fPureFe; }
    G4Material* GetPurePb()           const { return fPurePb; }

    // Look up by string name (for macro-driven selection)
    G4Material* GetMaterialByName(const G4String& name) const;

    // Print summary of all defined materials
    void PrintAll() const;

private:
    KaiAlloyMaterials();
    void DefineMaterials();

    static KaiAlloyMaterials* fInstance;

    // Kai alloys
    G4Material* fFeCoNiCr      = nullptr;
    G4Material* fTaHEA         = nullptr;
    G4Material* f0TaHEA        = nullptr;
    G4Material* fL12HEA        = nullptr;
    G4Material* fCCIMA         = nullptr;
    G4Material* fFeCoNiCrAl03  = nullptr;

    // References
    G4Material* fYH2           = nullptr;
    G4Material* fLBE           = nullptr;
    G4Material* fSS316L        = nullptr;
    G4Material* fInconel718    = nullptr;
    G4Material* fPureFe        = nullptr;
    G4Material* fPurePb        = nullptr;
};

#endif
