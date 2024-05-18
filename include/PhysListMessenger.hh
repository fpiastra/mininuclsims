#ifndef PHYS_LIST_MESSENGER_HH
#define PHYS_LIST_MESSENGER_HH

#include "globals.hh"
#include "G4UImessenger.hh"

class PhysList;
class G4UIdirectory;
class G4UIcmdWithAnInteger;
class G4UIcmdWithADoubleAndUnit;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class PhysListMessenger: public G4UImessenger
{
  public:
    PhysListMessenger(PhysList* pPhys);
    virtual ~PhysListMessenger();
 
    virtual void SetNewValue(G4UIcommand* command, G4String newValue);
 
  private:
    PhysList*  fPhysicsList;
 
    G4UIdirectory*        fPhysDir;
    //G4UIdirectory*        fOptPhPhysDir;

    G4UIcmdWithADoubleAndUnit* fGammaCutCmd;
    G4UIcmdWithADoubleAndUnit* fElectCutCmd;
    G4UIcmdWithADoubleAndUnit* fPositCutCmd;    
    G4UIcmdWithADoubleAndUnit* fAllCutCmd;    
    //G4UIcmdWithADoubleAndUnit* fMCutCmd;

    G4UIcmdWithAnInteger* fVerboseCmd;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
