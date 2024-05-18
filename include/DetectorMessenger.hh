#ifndef DET_MESSANGER_HH
#define DET_MESSANGER_HH

#include <G4UImessenger.hh>
#include <globals.hh>


class G4UIcommand;
class G4UIdirectory;
class G4UIcmdWithoutParameter;
class G4UIcmdWithAString;
class G4UIcmdWithADoubleAndUnit;
class G4UIcmdWith3Vector;
class G4UIcmdWith3VectorAndUnit;
class G4UIcmdWithAnInteger;
class G4UIcmdWithADouble;
class G4UIcmdWithABool;
class G4UIcmdWithoutParameter;

class DetConstr;

class DetectorMessenger: public G4UImessenger
{
public:
	DetectorMessenger(DetConstr *pDetector);
	~DetectorMessenger();

	void SetNewValue(G4UIcommand *pUIcommand, G4String hString);
	void ProcessTrackLimCmd(const G4UIcommand *cmd, const G4String& newValues);

private:
	DetConstr* fDetector;
	
	G4UIdirectory *fDetectorDir, *fDetectorOptDir;
	
	//G4UIcmdWithADoubleAndUnit *fLArAbsorbtionLengthCmd;
	//G4UIcmdWithADoubleAndUnit *fLArRayScatterLengthCmd;
	
	G4UIcmdWithAnInteger *fDetConstrVerb;
	
	G4UIcmdWithADoubleAndUnit *fTpbThicknCmd;
	
	G4UIcommand *fStepTrackLimCmd;
	
	G4UIcmdWithAString *fPhysVolCoordCmd;
	
	G4UIcmdWithAString *fPhysVolInfoCmd;
	
	G4UIcmdWithoutParameter *fPhysVolList;
	
	G4UIcmdWithAString *fLoadOpticalSettingsFile;
	
	G4UIcmdWithAnInteger *fOpticalSettingsVerb;
	
};

#endif

