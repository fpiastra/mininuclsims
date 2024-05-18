
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "PhysListMessenger.hh"
#include "PhysList.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAString.hh"
#include <G4UIcmdWithoutParameter.hh>

#include <G4UIcmdWith3Vector.hh>
#include <G4UIcmdWith3VectorAndUnit.hh>

#include <G4UIcmdWithADouble.hh>
#include <G4UIcmdWithABool.hh>


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PhysListMessenger::PhysListMessenger(PhysList* pPhys):
G4UImessenger(),
fPhysicsList(pPhys)
{
	fPhysDir = new G4UIdirectory("/minisims/physics/");
	fPhysDir->SetGuidance("UI commands for ArgonCube physics list setup");
	
	/*
	fOptPhPhysDir = new G4UIdirectory("/minisims/physics/optical/");
	fOptPhPhysDir->SetGuidance("Optical Physics List control ");
	
	fVerboseCmd = new G4UIcmdWithAnInteger("/minisims/physics/optical/verbose",this);
	fVerboseCmd->SetGuidance("Set verbosi for optical physics processes");
	fVerboseCmd->SetParameterName("verbose",true);
	fVerboseCmd->SetDefaultValue(1);
	fVerboseCmd->SetRange("verbose>=0");
	fVerboseCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	*/

	fGammaCutCmd = new G4UIcmdWithADoubleAndUnit("/minisims/physics/setGammaCut",this);
	fGammaCutCmd->SetGuidance("Set gamma cut.");
	fGammaCutCmd->SetParameterName("Gcut",false);
	fGammaCutCmd->SetUnitCategory("Length");
	fGammaCutCmd->SetRange("Gcut>0.0");
	fGammaCutCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
	
	fElectCutCmd = new G4UIcmdWithADoubleAndUnit("/minisims/physics/setElectCut",this);
	fElectCutCmd->SetGuidance("Set electron cut.");
	fElectCutCmd->SetParameterName("Ecut",false);
	fElectCutCmd->SetUnitCategory("Length");
	fElectCutCmd->SetRange("Ecut>0.0");
	fElectCutCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
	
	fPositCutCmd = new G4UIcmdWithADoubleAndUnit("/minisims/physics/setPositCut",this);
	fPositCutCmd->SetGuidance("Set positron cut.");
	fPositCutCmd->SetParameterName("Pcut",false);
	fPositCutCmd->SetUnitCategory("Length");
	fPositCutCmd->SetRange("Pcut>0.0");
	fPositCutCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
	
	fAllCutCmd = new G4UIcmdWithADoubleAndUnit("/minisims/physics/setCuts",this);
	fAllCutCmd->SetGuidance("Set cut for all.");
	fAllCutCmd->SetParameterName("cut",false);
	fAllCutCmd->SetUnitCategory("Length");
	fAllCutCmd->SetRange("cut>0.0");
	fAllCutCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
	
	/*
	fMCutCmd = new G4UIcmdWithADoubleAndUnit("/minisims/physics/setDetectorCuts",this);
	fMCutCmd->SetGuidance("Set cuts for the Detector");
	fMCutCmd->SetParameterName("Ecut",false);
	fMCutCmd->SetUnitCategory("Length");
	fMCutCmd->SetRange("Ecut>0.0");
	fMCutCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
	
	fPListCmd = new G4UIcmdWithAString("/minisims/physics/SelectPhysics",this);
	fPListCmd->SetGuidance("Select modula physics list.");
	fPListCmd->SetParameterName("PList",false);
	fPListCmd->AvailableForStates(G4State_PreInit);
	*/
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PhysListMessenger::~PhysListMessenger()
{
	if(fVerboseCmd) delete fVerboseCmd;
	
	if(fGammaCutCmd) delete fGammaCutCmd;
	if(fElectCutCmd) delete fElectCutCmd;
	if(fPositCutCmd) delete fPositCutCmd;
	if(fAllCutCmd) delete fAllCutCmd;
	//if(fMCutCmd) delete fMCutCmd;

	//if(fOptPhPhysDir) delete fOptPhPhysDir;
	if(fPhysDir) delete fPhysDir;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PhysListMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
	if( command == fVerboseCmd ){
		fPhysicsList->SetVerbose(static_cast<PhysVerbosity>(fVerboseCmd->GetNewIntValue(newValue)));
	}
	if( command == fGammaCutCmd ) fPhysicsList->SetCutForGamma(fGammaCutCmd->GetNewDoubleValue(newValue));
	
	if( command == fElectCutCmd ) fPhysicsList->SetCutForElectron(fElectCutCmd->GetNewDoubleValue(newValue));
	
	if( command == fPositCutCmd ) fPhysicsList->SetCutForPositron(fPositCutCmd->GetNewDoubleValue(newValue));
	
	if( command == fAllCutCmd ){
		G4double cut = fAllCutCmd->GetNewDoubleValue(newValue);
		fPhysicsList->SetCutForGamma(cut);
		fPhysicsList->SetCutForElectron(cut);
		fPhysicsList->SetCutForPositron(cut);
	}
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
