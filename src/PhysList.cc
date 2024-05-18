/**
 * AUTHOR: 
 * CONTACT: 
 * FIRST SUBMISSION: 12-28-2010
 *
 * REVISION:
 *
 * mm-dd-yyyy, What is changed, Whoami
 *
 * 12-28-2010, the switching from Geant4.9.2 to Geant4.9.3
 *             is finished starting from e+, Xiang
 */

// ---------------------------------------------------------------------------

#include "PhysListMessenger.hh"
#include "PhysList.hh"


#include "globals.hh"

#include "G4PhysicsListHelper.hh"
#include "G4LossTableManager.hh"
#include "G4ios.hh"
#include "G4SystemOfUnits.hh"

#include "G4ParticleDefinition.hh"
#include "G4ParticleTypes.hh"
#include "G4ParticleTable.hh"

#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"


#include "G4EmStandardPhysics.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4EmExtraPhysics.hh"
#include "G4EmParameters.hh"
#include "G4DecayPhysics.hh"
#include "G4NuclideTable.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4StoppingPhysics.hh"

#include "G4HadronElasticPhysicsHP.hh"
#include "G4HadronPhysicsFTFP_BERT_HP.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4HadronInelasticQBBC.hh"
#include "G4HadronPhysicsINCLXX.hh"
#include "G4IonElasticPhysics.hh"
#include "G4IonPhysics.hh"
#include "G4IonINCLXXPhysics.hh"
#include "G4IonBinaryCascadePhysics.hh"

#include "G4BosonConstructor.hh"
#include "G4LeptonConstructor.hh"
#include "G4MesonConstructor.hh"
#include "G4BosonConstructor.hh"
#include "G4BaryonConstructor.hh"
#include "G4IonConstructor.hh"
#include "G4ShortLivedConstructor.hh"

#include "G4StepLimiter.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4NeutronTrackingCut.hh"
#include "G4ParticleWithCuts.hh"
#include "G4UserLimits.hh"

#include "G4Region.hh"


#include <iomanip>

PhysList::PhysList():
G4VModularPhysicsList(),
fMessenger(nullptr),
fVerboseLevel(PhysVerbosity::kInfo),
fDefaultCutValue(1.*mm),
fCutForGamma(fDefaultCutValue),
fCutForElectron(fDefaultCutValue),
fCutForPositron(fDefaultCutValue), 
fEmPhysicsList(nullptr),
fRaddecayList(nullptr),
fDecayPhysList(nullptr),
fHadPhysicsList(nullptr),
fNhadcomp(0),
fDetectorCuts(nullptr),
fStepLimiter(nullptr),
fPhysRegistered(false)
{
	G4LossTableManager::Instance();
	defaultCutValue =1.*mm;
	
	fMessenger = new PhysListMessenger(this);
	
	SetVerboseLevel((G4int)PhysVerbosity::kInfo);
	
	//add new units for radioActive decays
	//
	new G4UnitDefinition( "millielectronVolt", "meV", "Energy", 1.e-3*eV);   
	
	// 
  	const G4double minute = 60*second;
  	const G4double hour   = 60*minute;
  	const G4double day    = 24*hour;
  	const G4double year   = 365*day;
  	new G4UnitDefinition("minute", "min", "Time", minute);
  	new G4UnitDefinition("hour",   "h",   "Time", hour);
  	new G4UnitDefinition("day",    "d",   "Time", day);
  	new G4UnitDefinition("year",   "y",   "Time", year);


	
	
	// Mandatory for G4NuclideTable
	// Half-life threshold must be set small or many short-lived isomers 
  	// will not be assigned life times (default to 0) 
  	G4NuclideTable::GetInstance()->SetThresholdOfHalfLife(0.1*picosecond);
  	G4NuclideTable::GetInstance()->SetLevelTolerance(1.0*eV);
	
	
	// EM physics
	//fEmPhysicsList = new G4EmStandardPhysics();
	fEmPhysicsList = new G4EmStandardPhysics_option4((G4int)fVerboseLevel);
	fEmParams = G4EmParameters::Instance();


	//default physics
	fDecayPhysList = new G4DecayPhysics();
	
	//default physics
	fRaddecayList = new G4RadioactiveDecayPhysics();
	
	// Ion Elastic scattering
	fIonPhys.push_back(new G4IonElasticPhysics((G4int)fVerboseLevel));

	// Ion Inelastic scattering
	fIonPhys.push_back(new G4IonPhysics((G4int)fVerboseLevel));

	//Hadronic physics list builders
	//Add additional physics for hadrons
	fHadronPhys.push_back( new G4EmExtraPhysics((G4int)fVerboseLevel) );
	fHadronPhys.push_back( new G4HadronElasticPhysicsHP((G4int)fVerboseLevel) );
	fHadronPhys.push_back( new G4StoppingPhysics((G4int)fVerboseLevel) );
    fHadronPhys.push_back( new G4IonBinaryCascadePhysics((G4int)fVerboseLevel) );
    fHadronPhys.push_back( new G4NeutronTrackingCut((G4int)fVerboseLevel));
   
    fHadPhysicsList = new G4HadronPhysicsQGSP_BIC_HP((G4int)fVerboseLevel);
	
	
	
	fStepLimiter = new G4StepLimiterPhysics;
	
}


////////////////////////////////////////////////////////////////////

PhysList::~PhysList()
{
	if(fMessenger) delete fMessenger;
	if(fRaddecayList) delete fRaddecayList;
	if(fDecayPhysList) delete fDecayPhysList;
	if(fEmPhysicsList) delete fEmPhysicsList;
	if(fHadPhysicsList) delete fHadPhysicsList;
	if(fHadronPhys.size()>0) {
		for(auto& hadBuil: fHadronPhys) {
			if(hadBuil) delete hadBuil;
		}
	}
}


////////////////////////////////////////////////////////////////////

void PhysList::RegisterAllLists(){
	this->G4VModularPhysicsList::RegisterPhysics(fEmPhysicsList);
	this->G4VModularPhysicsList::RegisterPhysics(fDecayPhysList);
	this->G4VModularPhysicsList::RegisterPhysics(fRaddecayList);
	
	for(auto& physList: fIonPhys){
		this->G4VModularPhysicsList::RegisterPhysics(physList);
	}
	
	for(auto& physList: fHadronPhys){
		this->G4VModularPhysicsList::RegisterPhysics(physList);
	}
	
	this->G4VModularPhysicsList::RegisterPhysics(fHadPhysicsList);

	//auto oldverb = fVerboseLevel;
	//fVerboseLevel = PhysVerbosity::kDebug;
	this->G4VModularPhysicsList::RegisterPhysics(fStepLimiter);
	//fVerboseLevel = oldverb;

	fPhysRegistered = true;
}

////////////////////////////////////////////////////////////////////

void PhysList::ConstructParticle()
{
	if(!fPhysRegistered) RegisterAllLists();

	this->G4VModularPhysicsList::ConstructParticle();
	
	if(fVerboseLevel>=PhysVerbosity::kDetails)
		G4cout << "Detail --> PhysList::ConstructParticle is done" << G4endl;

	return;

	fEmPhysicsList->ConstructParticle();
	fDecayPhysList->ConstructParticle();
	fRaddecayList->ConstructParticle();

	for(auto& physList: fIonPhys){
		physList->ConstructParticle();
	}

	for(auto& physList: fHadronPhys){
		physList->ConstructParticle();
	}
	
	fHadPhysicsList->ConstructParticle();
}


////////////////////////////////////////////////////////////////////

void PhysList::ConstructProcess()
{
	if(!fPhysRegistered) RegisterAllLists();

	this->G4VModularPhysicsList::ConstructProcess();
	
	if(fVerboseLevel>=PhysVerbosity::kDetails)
		G4cout << "Detail --> PhysList::ConstructProcess is done" << G4endl;

}


////////////////////////////////////////////////////////////////////

void PhysList::SetCuts()
{
	SetCutValue(fCutForGamma, "gamma");
	SetCutValue(fCutForElectron, "e-");
	SetCutValue(fCutForPositron, "e+");
	
	if(fVerboseLevel>=PhysVerbosity::kInfo){
		G4cout << "\nInfo --> PhysList::SetCuts(): Global cuts are set" << G4endl;
	}
	

	//The stufff below should be eventually activated from UI command.
	//Deactivate for time being
	
	/*
	if( !fTargetCuts ) SetTargetCut(fCutForElectron);
	G4Region* region = (G4RegionStore::GetInstance())->GetRegion("Target");
	region->SetProductionCuts(fTargetCuts);
	if(fVerboseLevel>=PhysVerbosity::kInfo) G4cout << "Target cuts are set" << G4endl;
	
	if( !fDetectorCuts ){
		this->SetDetectorCut(fCutForElectron);
	}
	
	 
	G4Region *region = (G4RegionStore::GetInstance())->GetRegion("Detector");
	if(region){
		region->SetProductionCuts(fDetectorCuts);
		if(fVerboseLevel>=PhysVerbosity::kInfo){
			G4cout << "Info --> PhysList::SetCuts(): \"Detector\" region cuts are set." << G4endl;
		}
	}
	*/
	
	if (fVerboseLevel>=PhysVerbosity::kDetails) DumpCutValuesTable();
}


////////////////////////////////////////////////////////////////////

void PhysList::SetCutForGamma(G4double cut)
{
	fCutForGamma = cut;
	SetParticleCuts(fCutForGamma, G4Gamma::Gamma());
}


////////////////////////////////////////////////////////////////////

void PhysList::SetCutForElectron(G4double cut)
{
  fCutForElectron = cut;
  SetParticleCuts(fCutForElectron, G4Electron::Electron());
}


////////////////////////////////////////////////////////////////////

void PhysList::SetCutForPositron(G4double cut)
{
  fCutForPositron = cut;
  SetParticleCuts(fCutForPositron, G4Positron::Positron());
}

void PhysList::SetCutForAll(G4double cut){
	SetCutForGamma(cut);
	SetCutForElectron(cut);
	SetCutForPositron(cut);
}

////////////////////////////////////////////////////////////////////
/*
void PhysList::SetTargetCut(G4double cut)
{
  if( !fTargetCuts ) fTargetCuts = new G4ProductionCuts();

  fTargetCuts->SetProductionCut(cut, idxG4GammaCut);
  fTargetCuts->SetProductionCut(cut, idxG4ElectronCut);
  fTargetCuts->SetProductionCut(cut, idxG4PositronCut);

}


////////////////////////////////////////////////////////////////////

void PhysList::SetDetectorCut(G4double cut)
{
  if( !fDetectorCuts ) fDetectorCuts = new G4ProductionCuts();
  
  //The idxG4***Cut are enumerator values defined in the G4ProductionCuts.hh header
  
  fDetectorCuts->SetProductionCut(cut, idxG4GammaCut);
  fDetectorCuts->SetProductionCut(cut, idxG4ElectronCut);
  fDetectorCuts->SetProductionCut(cut, idxG4PositronCut);
}
*/
