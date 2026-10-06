//====================================================================
// ShieldingActionInitialization.hh
//====================================================================
#ifndef SHIELDING_ACTION_INITIALIZATION_HH
#define SHIELDING_ACTION_INITIALIZATION_HH

#include "G4VUserActionInitialization.hh"

class ShieldingActionInitialization : public G4VUserActionInitialization {
public:
    void BuildForMaster() const override;
    void Build() const override;
};

#endif
