#ifndef PHYS_LIST_HH
#define PHYS_LIST_HH

#include <G4VUserPhysicsList.hh>
#include "G4VModularPhysicsList.hh"
#include <globals.hh>


//Forward declarations
class G4StepLimiter;
class G4EmParameters;
class G4ProductionCuts;
class G4VPhysicsConstructor;
class G4StepLimiterPhysics;
class PhysListMessenger;


enum class PhysVerbosity{kSilent, kInfo, kDetails, kDebug};

class PhysList: public G4VModularPhysicsList
{
	PhysListMessenger* fMessenger;
	
	//All the static declaration here are made for speed and low memory consumption in addition they are thread safe using the G4 internal functionalities
	//Note: All this static members are not shared between different threads but shared between different instances of the class in the same thread
	PhysVerbosity fVerboseLevel;
	
	G4EmParameters* fEmParams;

	const G4double fDefaultCutValue;
	G4double fCutForGamma;
	G4double fCutForElectron;
	G4double fCutForPositron;
	
	G4VPhysicsConstructor*  fEmPhysicsList;
	G4VPhysicsConstructor*  fRaddecayList;
	G4VPhysicsConstructor*  fDecayPhysList;
	G4VPhysicsConstructor*  fHadPhysicsList;
	
	std::vector<G4VPhysicsConstructor*>  fHadronPhys, fIonPhys;
	G4int fNhadcomp;
	
	G4ProductionCuts* fDetectorCuts;
	
	G4StepLimiterPhysics *fStepLimiter;
	
	G4bool fPhysRegistered;

	//Do not allow to have the assign operator and the copy constructor
	PhysList & operator=(const PhysList &right);
	PhysList(const PhysList&);
	
	
	
protected:
	// Method used to delay the registration of the lists after the construction 
	// of this class and the initialisation of the run manager, 
	// when it is still possible setting up the physics options
	void RegisterAllLists();
	
public:
	PhysList();
	virtual ~PhysList();
	
	virtual void ConstructParticle();
	virtual void ConstructProcess();
	virtual void SetCuts();
	
	void SetCutForGamma(G4double);
	void SetCutForElectron(G4double);
	void SetCutForPositron(G4double);
	void SetCutForAll(G4double cut);
	
	void SelectPhysicsList(const G4String& name){;}; //Dummy for the moment
	
	
	void SetTargetCut(G4double val){;}; //Dummy for the moment
	void SetDetectorCut(G4double val){;}; //Dummy for the moment
	
	//for the Messenger
	void SetVerbose(PhysVerbosity verb){fVerboseLevel=verb;};

};


#endif

