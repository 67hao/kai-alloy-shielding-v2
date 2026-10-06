//====================================================================
// ShieldingActionInitialization.cc
//====================================================================
#include "ShieldingActionInitialization.hh"
#include "ShieldingPrimaryGenerator.hh"
#include "ShieldingRunAction.hh"
#include "ShieldingSteppingAction.hh"

void ShieldingActionInitialization::BuildForMaster() const {
    SetUserAction(new ShieldingRunAction());
}

void ShieldingActionInitialization::Build() const {
    SetUserAction(new ShieldingPrimaryGenerator());

    auto runAction = new ShieldingRunAction();
    SetUserAction(runAction);

    SetUserAction(new ShieldingSteppingAction(runAction));
}
