#ifndef ANALISYS_MESSENGER_HH
#define ANALISYS_MESSENGER_HH


#include "G4UIcommand.hh"
#include "G4ThreeVector.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWith3Vector.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithABool.hh"
#include "G4Tokenizer.hh"

#include "G4ios.hh"

#include "G4UImessenger.hh"
#include "globals.hh"




class AnalysisManager;

class AnalysisMessenger: public G4UImessenger
{
public:
	AnalysisMessenger(AnalysisManager *pAnManager);
	virtual ~AnalysisMessenger();

	void SetNewValue(G4UIcommand *pUIcommand, G4String hNewValue);

private:
	AnalysisManager* fAnManager;

	G4UIdirectory *fAnalysisDir;
	
	G4UIcmdWithAnInteger *fRandSeedCmd;
	G4UIcmdWithAnInteger *fVerboseCmd;
	G4UIcmdWithAnInteger *fPrintModuloCmd;
	G4UIcmdWithABool *fStepDebugCmd;
	G4UIcmdWithAnInteger *fSaveDataCmd;
	G4UIcmdWithAString *fFileNameCmd;
	G4UIcmdWithAString *fDefOptSDCmd;
	G4UIcmdWithAString *fDefAbsVolCmd;
	
	G4UIcmdWithAnInteger *fAutoFlushCmd;
	G4UIcmdWithAnInteger *fAutoSaveCmd;
	
	
	
};

#endif

