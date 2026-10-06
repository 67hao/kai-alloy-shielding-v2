//====================================================================
// ShieldingSteppingAction.hh
//====================================================================
#ifndef SHIELDING_STEPPING_ACTION_HH
#define SHIELDING_STEPPING_ACTION_HH

#include "G4UserSteppingAction.hh"

class ShieldingRunAction;

class ShieldingSteppingAction : public G4UserSteppingAction {
public:
    explicit ShieldingSteppingAction(ShieldingRunAction* runAction);
    void UserSteppingAction(const G4Step* step) override;

private:
    ShieldingRunAction* fRunAction;
};

#endif
