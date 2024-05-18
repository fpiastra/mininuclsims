#include "ParticleSource.hh"
#include "ParticleSourceMessenger.hh"

#include "G4UIcommand.hh"
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
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

#include "G4Geantino.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4OpticalPhoton.hh"
#include "G4SystemOfUnits.hh"

#include "G4ios.hh"

#include <fstream>
#include <iomanip>


PartSrcMessenger::PartSrcMessenger(PartSrc *pParticleSource):
fParticleSource(pParticleSource),
fParticleTable(nullptr),
fDirectory(nullptr),
fPrimNbCmd(nullptr),
fTypeCmd(nullptr),
fShapeCmd(nullptr),
fCenterCmd(nullptr),
fHalfxCmd(nullptr),
fHalfyCmd(nullptr),
fHalfzCmd(nullptr),
fRadiusCmd(nullptr),
fConfineCmd(nullptr),
fAngTypeCmd(nullptr),
fVerbosityCmd(nullptr),
fParticleCmd(nullptr),
fIonCmd(nullptr),
fPositionCmd(nullptr),
fDirectionCmd(nullptr),
fPolarCmd(nullptr),
fEnergyCmd(nullptr),
fGetDirectCmd(nullptr),
fGetPolarCmd(nullptr),
fGetPartCmd(nullptr),
fSurfNormCmd(nullptr),
fParticlesList(G4String("")),
fShootIon(false),
fAtomicNumber(0),
fAtomicMass(0),
fIonCharge(0),
fIonExciteEnergy(0.)
{
	fParticleTable = G4ParticleTable::GetParticleTable();
	
	MakeParticlesList();
	

	// create directory
	fDirectory = new G4UIdirectory("/minisims/gun/");
	fDirectory->SetGuidance("Particle source control commands for medipix detector.");

	// list available particles
	/*
	fListCmd = new G4UIcmdWithoutParameter("/xurich2/gun/List", this);
	fListCmd->SetGuidance("List available particles.");
	fListCmd->SetGuidance(" Invoke G4ParticleTable.");
	*/
	
	// set particle  
	fParticleCmd = new G4UIcmdWithAString("/minisims/gun/particle", this);
	fParticleCmd->SetGuidance("Set particle to be generated. Defaults to geantino.");
	fParticleCmd->SetGuidance("usage: /medipix/gun/particle Part_name");
	fParticleCmd->SetParameterName("particleName", true);
	fParticleCmd->SetDefaultValue("geantino");
	fParticleCmd->SetCandidates(fParticlesList);
	fParticleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	
	// set the ion 
	fIonCmd = new G4UIcommand("/minisims/gun/ion", this);
	fIonCmd->SetGuidance("Set properties of ion to be generated.");
	fIonCmd->SetGuidance("usage: /minisims/gun/ion Z A Q E");
	fIonCmd->SetGuidance("        Z:(int) AtomicNumber");
	fIonCmd->SetGuidance("        A:(int) AtomicMass");
	fIonCmd->SetGuidance("        Q:(int) Charge of Ion (in unit of e)");
	fIonCmd->SetGuidance("        E:(double) Excitation energy (in keV)");

	G4UIparameter *param;
	param = new G4UIparameter("Z", 'i', false);
	param->SetDefaultValue("1");
	fIonCmd->SetParameter(param);
	param = new G4UIparameter("A", 'i', false);
	param->SetDefaultValue("1");
	fIonCmd->SetParameter(param);
	param = new G4UIparameter("Q", 'i', true);
	param->SetDefaultValue("0");
	fIonCmd->SetParameter(param);
	param = new G4UIparameter("E", 'd', true);
	param->SetDefaultValue("0.0");
	fIonCmd->SetParameter(param);
	
	
	// number of primaries
	fPrimNbCmd = new G4UIcmdWithAnInteger("/minisims/gun/primaryNb", this);
	fPrimNbCmd->SetGuidance("Set number of primary particles (must be >0). Defaults to 1.");
	fPrimNbCmd->SetGuidance("usage: /medipix/gun/primaryNb Np");
	fPrimNbCmd->SetParameterName("PrimNb", true);
	fPrimNbCmd->SetDefaultValue(1);
	fPrimNbCmd->SetRange("PrimNb>0");
	fPrimNbCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// particle direction
	fDirectionCmd = new G4UIcmdWith3Vector("/minisims/gun/direction", this);
	fDirectionCmd->SetGuidance("Set momentum direction.");
	fDirectionCmd->SetGuidance("Direction needs not to be a unit vector.");
	fDirectionCmd->SetGuidance("usage: /minisims/gun/direction Px Py Pz");
	fDirectionCmd->SetParameterName("Px", "Py", "Pz", true, true);
	fDirectionCmd->SetRange("Px != 0 || Py != 0 || Pz != 0");
	fDirectionCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// set polarization
	fPolarCmd = new G4UIcmdWith3Vector("/minisims/gun/polarization", this);
	fPolarCmd->SetGuidance("Set polarization vector.");
	fPolarCmd->SetGuidance("Direction needs not to be a unit vector.");
	fPolarCmd->SetGuidance("usage: /minisims/gun/polarization ex ey ez");
	fPolarCmd->SetParameterName("ex", "ey", "ez", true, true);
	fPolarCmd->SetRange("ex != 0 || ey != 0 || ez != 0");
	fPolarCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// particle energy
	fEnergyCmd = new G4UIcmdWithADoubleAndUnit("/minisims/gun/energy", this);
	fEnergyCmd->SetGuidance("Set the particle energy (default unit is MeV).");
	fEnergyCmd->SetGuidance("usage: /minisims/gun/energy E [unit]");
	fEnergyCmd->SetParameterName("Energy", true, true);
	fEnergyCmd->SetDefaultUnit("MeV");
	fEnergyCmd->SetUnitCategory("Energy");
	fEnergyCmd->SetUnitCandidates("eV keV MeV GeV TeV");
	fEnergyCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	//Particle position
	fPositionCmd = new G4UIcmdWith3VectorAndUnit("/minisims/gun/position", this);
	fPositionCmd->SetGuidance("Set starting position of the particle(s) vertex (default unit is mm).");
	fPositionCmd->SetGuidance("usage: /minisims/gun/position X Y Z [unit]");
	fPositionCmd->SetParameterName("X", "Y", "Z", true, true);
	fPositionCmd->SetDefaultUnit("mm");
	fPositionCmd->SetUnitCategory("Length");
	fPositionCmd->SetUnitCandidates("nm mum mm cm m km");
	fPositionCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// source distribution type
	fTypeCmd = new G4UIcmdWithAString("/minisims/gun/sourceType", this);
	fTypeCmd->SetGuidance("Sets source distribution type.");
	fTypeCmd->SetGuidance("usage: /minisims/gun/sourceType DisType");
	fTypeCmd->SetGuidance("Candidates: Point, Volume, Surface");
	fTypeCmd->SetParameterName("DisType", true, true);
	fTypeCmd->SetDefaultValue("Point");
	fTypeCmd->SetCandidates("Point Volume Surface");
	fTypeCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// source shape
	fShapeCmd = new G4UIcmdWithAString("/minisims/gun/shape", this);
	fShapeCmd->SetGuidance("Sets source shape type.");
	fShapeCmd->SetGuidance("usage: /minisims/gun/shape Shape");
	fShapeCmd->SetGuidance("Candidates are: Sphere, Cylinder, Box, Disk, Rect");
	fShapeCmd->SetGuidance("    Sphere, Cylinder and Box shapes are used only with source of distribution type Volume.");
	fShapeCmd->SetGuidance("    Disk and Rect shapes are used only with source of distribution type Surface.");
	fShapeCmd->SetParameterName("Shape", true, true);
	fShapeCmd->SetDefaultValue("NULL");
	fShapeCmd->SetCandidates("Sphere Cylinder Box Disk Rect");
	fShapeCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// center coordinates
	fCenterCmd = new G4UIcmdWith3VectorAndUnit("/minisims/gun/center", this);
	fCenterCmd->SetGuidance("Set center coordinates of source distribution type.");
	fCenterCmd->SetGuidance("It correspond to the actual position in space of the Volume or Surface middle point (or baricentrum).");
	fCenterCmd->SetGuidance("usage: /minisims/gun/center X Y Z [unit]");
	fCenterCmd->SetParameterName("X", "Y", "Z", true, true);
	fCenterCmd->SetDefaultUnit("mm");
	fCenterCmd->SetUnitCategory("Length");
	fCenterCmd->SetUnitCandidates("nm mum mm cm m km");
	fCenterCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// half x of source(if source shape is Box)
	fHalfxCmd = new G4UIcmdWithADoubleAndUnit("/minisims/gun/halfx", this);
	fHalfxCmd->SetGuidance("Set x half length of source distribution type.");
	fHalfxCmd->SetGuidance("Used only with for Box and Rect shapes.");
	fHalfxCmd->SetGuidance("usage: /minisims/gun/halfx hx [unit]");
	fHalfxCmd->SetParameterName("Halfx", true, true);
	fHalfxCmd->SetDefaultUnit("mm");
	fHalfxCmd->SetUnitCategory("Length");
	fHalfxCmd->SetUnitCandidates("nm mum mm cm m km");
	fHalfxCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// half y of source (if source shape is Box)
	fHalfyCmd = new G4UIcmdWithADoubleAndUnit("/minisims/gun/halfy", this);
	fHalfyCmd->SetGuidance("Set y half length of source distribution type.");
	fHalfyCmd->SetGuidance("Used only with for Box and Rect shapes.");
	fHalfyCmd->SetGuidance("usage: /minisims/gun/halfy hy [unit]");
	fHalfyCmd->SetParameterName("Halfy", true, true);
	fHalfyCmd->SetDefaultUnit("mm");
	fHalfyCmd->SetUnitCategory("Length");
	fHalfyCmd->SetUnitCandidates("nm mum mm cm m km");
	fHalfyCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// half height of source
	fHalfzCmd = new G4UIcmdWithADoubleAndUnit("/minisims/gun/halfz", this);
	fHalfzCmd->SetGuidance("Set z half length of source distribution type.");
	fHalfzCmd->SetGuidance("Used only with for Box, Rect and Cylinder shapes.");
	fHalfzCmd->SetGuidance("usage: /minisims/gun/halfz hz [unit]");
	fHalfzCmd->SetParameterName("Halfz", true, true);
	fHalfzCmd->SetDefaultUnit("mm");
	fHalfzCmd->SetUnitCategory("Length");
	fHalfzCmd->SetUnitCandidates("nm mum mm cm m km");
	fHalfzCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// radius of source  
	fRadiusCmd = new G4UIcmdWithADoubleAndUnit("/minisims/gun/radius", this);
	fRadiusCmd->SetGuidance("Set radius of source distribution type.");
	fRadiusCmd->SetGuidance("Used only with for Cylinder and Disk shapes.");
	fRadiusCmd->SetGuidance("usage: /minisims/gun/radius r [unit]");
	fRadiusCmd->SetParameterName("Radius", true, true);
	fRadiusCmd->SetDefaultUnit("cm");
	fRadiusCmd->SetUnitCandidates("nm mum mm cm m km");
	fRadiusCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// confine to volume(s)
	fConfineCmd = new G4UIcmdWithAString("/minisims/gun/confine", this);
	fConfineCmd->SetGuidance("Confine source to volume(s) (NULL to unset).");
	fConfineCmd->SetGuidance("Is used only with the Volume source distribution type.");
	fConfineCmd->SetGuidance("usage: /minisims/gun/confine VolName1 VolName2 ...");
	fConfineCmd->SetParameterName("VolName", true, true);
	fConfineCmd->SetDefaultValue("NULL");
	fConfineCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	//Surface normal vector
	fSurfNormCmd = new G4UIcmdWith3Vector("/minisims/gun/surfNorm", this);
	fSurfNormCmd->SetGuidance("Set the normal vector to the Surface distribution type (Disk or Rect).");
	fSurfNormCmd->SetGuidance("Direction needs not to be a unit vector.");
	fSurfNormCmd->SetGuidance("usage: /minisims/gun/surfNorm nx ny nz");
	fSurfNormCmd->SetParameterName("nx", "ny", "nz", true, true);
	fSurfNormCmd->SetRange("nx != 0 || ny != 0 || nz != 0");
	fSurfNormCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// angular distribution
	fAngTypeCmd = new G4UIcmdWithAString("/minisims/gun/angtype", this);
	fAngTypeCmd->SetGuidance("Sets angular source distribution type");
	fAngTypeCmd->SetGuidance("Possible variables are: iso direction");
	fAngTypeCmd->SetParameterName("AngDis", true, true);
	fAngTypeCmd->SetDefaultValue("iso");
	fAngTypeCmd->SetCandidates("iso direction");
	fAngTypeCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// verbosity
	fVerbosityCmd = new G4UIcmdWithAnInteger("/minisims/gun/verbose", this);
	fVerbosityCmd->SetGuidance("Set Verbose level for gun");
	fVerbosityCmd->SetGuidance(" 0 : Silent");
	fVerbosityCmd->SetGuidance(" 1 : Limited information");
	fVerbosityCmd->SetGuidance(" 2 : Detailed information");
	fVerbosityCmd->SetGuidance(" 3 : Debug information");
	fVerbosityCmd->SetParameterName("level", false);
	fVerbosityCmd->SetRange("level>=0 && level <=3");
	fVerbosityCmd->AvailableForStates(G4State_PreInit, G4State_PreInit, G4State_Idle);
	
	// Get the particle type
	fGetPartCmd = new G4UIcmdWithoutParameter("/minisims/gun/getPartType", this);
	fGetPartCmd->SetGuidance("Prints the particle type selected");
	fGetPartCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// Get particle direction
	fGetDirectCmd = new G4UIcmdWithoutParameter("/minisims/gun/getDirection", this);
	fGetDirectCmd->SetGuidance("Prints the particle direction set");
	fGetDirectCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// Get particle polarization
	fGetPolarCmd = new G4UIcmdWithoutParameter("/minisims/gun/getPolar", this);
	fGetPolarCmd->SetGuidance("Prints the particle polarization set");
	fGetPolarCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
}

PartSrcMessenger::~PartSrcMessenger()
{
	if(fDirectory) delete fDirectory;
	
	if(fPrimNbCmd) delete fPrimNbCmd;
	if(fTypeCmd) delete fTypeCmd;
	if(fShapeCmd) delete fShapeCmd;
	if(fCenterCmd) delete fCenterCmd;
	if(fHalfxCmd) delete fHalfxCmd;
	if(fHalfyCmd) delete fHalfyCmd;
	if(fHalfzCmd) delete fHalfzCmd;
	if(fRadiusCmd) delete fRadiusCmd;
	if(fConfineCmd) delete fConfineCmd;
	if(fAngTypeCmd) delete fAngTypeCmd;
	if(fVerbosityCmd) delete fVerbosityCmd;
	if(fParticleCmd) delete fParticleCmd;
	if(fPositionCmd) delete fPositionCmd;
	if(fDirectionCmd) delete fDirectionCmd;
	if(fPolarCmd) delete fPolarCmd;
	if(fEnergyCmd) delete fEnergyCmd;
	if(fGetPartCmd) delete fGetPartCmd;
	if(fGetDirectCmd) delete fGetDirectCmd;
	if(fGetPolarCmd) delete fGetPolarCmd;
	if(fIonCmd) delete fIonCmd;
	if(fSurfNormCmd) delete fSurfNormCmd;
	
}


void PartSrcMessenger::MakeParticlesList()
{
	G4int nPtcl = fParticleTable->entries();

	for(G4int iPart = 0; iPart < nPtcl; iPart++){
		fParticlesList += fParticleTable->GetParticleName(iPart);
		fParticlesList += " ";
	}
	fParticlesList += "ion ";
	
}


void PartSrcMessenger::SetNewValue(G4UIcommand* command, G4String newValues)
{
	if(command == fPrimNbCmd){
		G4int newPrimNb = fPrimNbCmd->GetNewIntValue(newValues);
		G4cout << "\nSetting number of primaries to be generated to: " << newPrimNb << G4endl;
		fParticleSource->SetPrimNb( newPrimNb );
		return;
	}
	
	if(command == fParticleCmd){
		if(newValues == "ion"){
			fShootIon = true;
		}else{
			fShootIon = false;
			
			G4ParticleDefinition* pd = fParticleTable->FindParticle(newValues);
			
			if(pd){
				G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting primary particle to <" << pd->GetParticleName() << ">" << G4endl;
				fParticleSource->SetParticleDef(pd);
			}
		}
		
		return;
	}
	
	if(command == fIonCmd){
		if(fShootIon){
			ProcessIonCmd(newValues);
		}else{
			G4cout << "ERROR --> PartSrcMessenger::SetNewValue: Cannot set ion properties if the primary particle is not an ion! Issue the command \"/medipix/gun/particle ion\" before using the \"/medipix/gun/ion\"." << G4endl;
		}
		return;
	}
	
	if(command == fTypeCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting position distribution type to \"" << newValues << "\"" << G4endl;
		fParticleSource->SetPosDisType(newValues);
		return;
	}
	
	if(command == fShapeCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting position source shape to \"" << newValues << "\"" << G4endl;
		fParticleSource->SetPosDisShape(newValues);
		return;
	}
	
	if(command == fCenterCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting source center coordinates to " << fCenterCmd->GetNew3VectorValue(newValues) << G4endl;
		fParticleSource->SetCenterCoords(fCenterCmd->GetNew3VectorValue(newValues));
		return;
	}
	
	if(command == fHalfxCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting source half X dimension to " << fHalfxCmd->GetNewDoubleValue(newValues) << G4endl;
		fParticleSource->SetHalfX(fHalfxCmd->GetNewDoubleValue(newValues));
		return;
	}
	
	if(command == fHalfyCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting source half Y dimension to " << fHalfyCmd->GetNewDoubleValue(newValues) << G4endl;
		fParticleSource->SetHalfY(fHalfyCmd->GetNewDoubleValue(newValues));
		return;
	}
	
	if(command == fHalfzCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting source half Z dimension to " << fHalfzCmd->GetNewDoubleValue(newValues) << G4endl;
		fParticleSource->SetHalfZ(fHalfzCmd->GetNewDoubleValue(newValues));
		return;
	}
	
	if(command == fRadiusCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting source radius to " << fRadiusCmd->GetNewDoubleValue(newValues) << G4endl;
		fParticleSource->SetRadius(fRadiusCmd->GetNewDoubleValue(newValues));
		return;
	}
	
	if(command == fAngTypeCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting angular distribution type to \"" << newValues << "\"" << G4endl;
		fParticleSource->SetAngDistType(newValues);
		return;
	}
	
	if(command == fSurfNormCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting the normal to the surface to " << fDirectionCmd->GetNew3VectorValue(newValues) << G4endl;
		fParticleSource->SetSurfNormal(fDirectionCmd->GetNew3VectorValue(newValues));
		return;
	}
	
	if(command == fConfineCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Confining primary particle to volume \"" << newValues << "\"" << G4endl;
		fParticleSource->ConfineSourceToVolume(newValues);
		return;
	}
	
	if(command == fVerbosityCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting particle generator verbosity level to " << fVerbosityCmd->GetNewIntValue(newValues) << G4endl;
		fParticleSource->SetVerbosity((PartSrcVerbosity)fVerbosityCmd->GetNewIntValue(newValues));
		return;
	}
	
	if(command == fDirectionCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting primary particle direction to " << fDirectionCmd->GetNew3VectorValue(newValues) << G4endl;
		G4cout << "Info --> PartSrcMessenger::SetNewValue: Setting angular distribution type to \"direction\"" << G4endl;
		fParticleSource->SetAngDistType("direction");
		fParticleSource->SetDirection(fDirectionCmd->GetNew3VectorValue(newValues));
		return;
	}
	
	if(command==fPolarCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting primary particle polarization to " << fPolarCmd->GetNew3VectorValue(newValues) << G4endl;
		fParticleSource->SetPhotonPolar(fPolarCmd->GetNew3VectorValue(newValues));
		return;
	}
	
	if(command == fEnergyCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting primary particle energy to " << newValues << G4endl;
		G4cout << "Setting energy distribution type to \"Mono\"" << G4endl;
		fParticleSource->SetEnergyDisType("Mono");
		fParticleSource->SetKinEnergy(fEnergyCmd->GetNewDoubleValue(newValues));
		return;
	}
	
	if(command == fPositionCmd){
		G4cout << "\nInfo --> PartSrcMessenger::SetNewValue: Setting primary particle position to " << fPositionCmd->GetNew3VectorValue(newValues) << G4endl;
		G4cout << "Info --> PartSrcMessenger::SetNewValue: Setting position distribution type to \"Point\"" << G4endl;
		fParticleSource->SetPosDisType("Point");
		fParticleSource->SetCenterCoords(fPositionCmd->GetNew3VectorValue(newValues));
		return;
	}
	
	if(command == fGetPartCmd){
		fParticleSource->PrintParticle();
		return;
	}
	
	if(command == fGetDirectCmd){
		
		fParticleSource->PrintDirection();
		return;
	}
	
	if(command == fGetPolarCmd){
		fParticleSource->PrintPolar();
		return;
	}
	
	G4cerr << "\nERROR --> PartSrcMessenger::SetNewValue: command <" << command << "> not recognized." << G4endl;
}


void PartSrcMessenger::ProcessIonCmd(G4String newValues){
	G4Tokenizer next(newValues);

	// check argument
	fAtomicNumber = StoI(next());
	fAtomicMass = StoI(next());
	G4String sQ = next();

	if(sQ.isNull()){
		fIonCharge = fAtomicNumber;
	}else{
		fIonCharge = StoI(sQ);
		sQ = next();
		if(sQ.isNull()){
			fIonExciteEnergy = 0.0;
		}else{
			fIonExciteEnergy = StoD(sQ) * keV;
		}
	}

	G4ParticleDefinition *ion = G4IonTable::GetIonTable()->GetIon(fAtomicNumber, fAtomicMass, fIonExciteEnergy);
	if(!ion){
		G4cerr << "\nERROR --> PartSrcMessenger::ProcessIonCmq: Can't find ion in the particle table (not defined). Ion with:" << G4endl;
		G4cerr << "   Z="<< fAtomicNumber << G4endl;
		G4cerr << "   A=" << fAtomicMass << G4endl;
	}else{
		fParticleSource->SetParticleDef(ion);
		fParticleSource->SetParticleCharge(fIonCharge*eplus);
	}
}