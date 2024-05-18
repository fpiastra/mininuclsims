#ifndef ANALYSIS_MANAGER_HH
#define ANALYSIS_MANAGER_HH

#include "globals.hh"
#include "G4ProcessTable.hh"

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TParameter.h"
#include "TDirectory.h"


#include <set>
#include <vector>
#include <map>


class G4Run;
class G4Event;
class G4Step;
class G4SteppingManager;

class EventData;
class PrimGenAction;
class PartSrc;
class AnalysisMessenger;
class G4Navigator;


enum class DatasaveLevel
{
	kOff, //No saving
	kLUT, //Only save hit data (no primary info)
	kHits, //Only stuff useful for LUT (hits, and primary info)
	kHitsExt, //Hits extended informations
	kSdSteps, //All the steps in the physical volumes defined by the user
	kAll //All the steps everywhere in any physical volume: same data as kSdSteps but everywhere
};

enum class AnalysisVerbosity{
	kSilent,
	kInfo,
	kDetails,
	kDebug
};


class AnalysisManager
{
	
	G4int fRunSeed;
	
	//Handles of necessary objects 
	PrimGenAction *fPrimaryGeneratorAction; //Class only passed. //This should NEVER be instanced or removed within this class
	const PartSrc *fPartSrc;
	AnalysisMessenger *fMessenger; //Created here
	EventData *fEventData; //Created here
	
	G4Navigator* fNav;
	//G4TouchableHandle fTouchableHandle;
	
	
	//Verbosity stuff
	AnalysisVerbosity fVerbose;
	G4int fPrintModulo;
	G4bool fStepsDebug; //This is actually used only to see the behaviour before and after a volume boundary
	
	
	//Data storing stuff
	TFile *fTreeFile;
	TTree *fTree;
	Long64_t fAutoSaveEvs, fAutoFlushEvs;
	
	DatasaveLevel fSave;
	
	G4String fDataFilename;
	
	

	//--------------------------------------------------//
	// Private variables mainly for the stepping action //
	// and for saving of the quantities at end of event //
	//--------------------------------------------------//
	
	G4int fCurrentEvent;
	
	
	//Stuff used to reduce the number of calls at stepping level
	G4int fCurrentTrackId;
	G4Track *fCurrentTrack;
	G4int fLastTrackId; //Track ID of the last step
	G4int fLastPartType;
	G4VPhysicalVolume* fLastPhysVol; //Pointer of the physical volume seen at the last post-step point.
	G4VPhysicalVolume* fLastPrePhysVol; //Pointer of the physical volume seen at the last pre-step point.
	
	int fLastVolIdx;
	int fLastCopyNum;
	std::string fLastVolGlobalCopy; //This is now a string that looks like an IP address
	
	
	
	
	G4int fNbEventsToSimulate, fNbPrim; //Taken from the Primary Generator Action Instance
	
	G4ProcessTable *fProcTable; //This should NEVER be instanced or removed within this class
	G4ProcessVector *fProcVec;  //This should NEVER be instanced or removed within this class
	G4int fNprocs;
	G4ParticleTable *fParticlesTable; //This should NEVER be instanced or removed within this class
	
	std::map<G4int, G4String> fParticlesMap; //This is for the json formatted dictionary
	std::map<const G4ParticleDefinition*, G4int> fParticlesDefsMap; //This is used at step for saving the kind of particle
	G4int fGenIonId;
	
	
	Long64_t fNbTotHits;
	
	
	std::map<G4VPhysicalVolume*, int> fPhysVolMap; //Map of all the existing physics volumes
	std::map<int, G4String> fPhysVolNamesMap; //Map of all the existing physics volumes: link their number to their name
	std::map<G4VPhysicalVolume*, int> fPhysVolUniqueMap; //This is to save in the TTree a unique index corresponding to a phys vol name
	std::map<int, G4String> fPhysVolUniqueNamesMap; //This is to make the correspondence of the volume index saved in the TTree to the name of phys volume in the json dictionary
	
	std::map<G4VPhysicalVolume*, int> fPhysVolCpnmMap; //Map of all the existing physics volumes by copy number
	std::map<G4VPhysicalVolume*, int> fSenDetVolPtrsMap; //Index of each physical volume
	
	
	std::set<G4VPhysicalVolume*> fSenDetVolPtrs; //Sensitive volume where the tracking quantities are saved

	
	
	
	std::map<int, G4String> fPhysVolsMap;
	
	std::map<int, G4String> fProcessesMap;
	
	
	
	std::set<int> fTrackIDs; //Storage of the tracks IDs seen during an event when they are saved

	std::map<int, int> fTrackParentIDsMap; //Here for each track ID (first entry, key) there is the ID of the parent track

	std::map<int, int> fTrackGenerationMap; //Here for each track ID (first entry, key) there is the track generation level

	std::map<int, int> fFirstParentIDMap; //Here for each track ID (first entry, key) there is the track ID of the first parent (0 in case it is a primary track)

	std::map<int, int> fTrackCreatProc; //Here for each track ID (first entry, key) there is the process ID of the creation process (0 in casse of primary track)
	

	//---------------------------------------------------//
	// Variables and containers only for optical photons //
	//---------------------------------------------------//
	//Mostly used to debug what happens at the boundaries
	G4bool fWasAtBoundary;
	std::set<G4VPhysicalVolume*> fOptPhAbsVolPtrs; //Volumes where the optical photon tracks get forcely absorbed (killed)


	//------------------------------------//
	// Private methods (for internal use) //
	//------------------------------------//
	
	//This function takes care of making the look-up tables for physical volumes at the start of the run
	void MakeVolMaps();

	//This function takes care of making the look-up tables for particle definitions at the start of the run
	void MakeParticlesMaps();
	
	//This function tries to make physical volumes with consecutive indexing if at the same tree level
	//It behaves very differently from the same function in the detector construction
	void ScanVols(const G4LogicalVolume* LogVol, int& volindex);
	
	int FindVolumeIndex( const G4VPhysicalVolume* aVolume );
	
	//This method should determine the unique touchable copy corresponding to a physical volume by means of the touchable history (the geometry tree).
	// At the moment I can do this only by saving a dot separated list (it is a string like an IP address) with the copy numbers of ghe volumes in the current branch of the geometry tree
	std::string FindVolGlobalCopy(const G4TouchableHandle& touch);
	
	
	//This actually returns the process position in the table of processes (it is a vector)
	int FindProcessIndex( const G4VProcess* aProcess );
	
	//Makes the list of all processes and produces a json file like dictionary usable in analysis phase. Method executed at the start of the run.
	std::string BuildProcsDict();

	//Makes the list of all particles definitions and produces a json file like dictionary usable in analysis phase. Method executed at the start of the run.
	std::string BuildParticlesDict();
	
	//Makes the list of all physical volumes and produces a json file like dictionary usable in analysis phase. Method executed at the start of the run. Only the volumes defined by the user are taken into account if the saving level is high enough. In the case that kAll is the defined saving mode the entire list of physical volumes is considered
	std::string BuildPhysVolDict();
	
	//The same as before but only for the volumes that are sensitive volumes
	std::string BuildSDvolDict();
	
	
public:
	AnalysisManager(PrimGenAction *pPrimaryGeneratorAction);
	
	virtual ~AnalysisManager();
	
	
	//-----------------------------------------//
	// Wrapping functions for the user actions //
	//-----------------------------------------//
	
	void BeginOfRun(const G4Run *pRun); 
	void EndOfRun(const G4Run *pRun); 
	void BeginOfEvent(const G4Event *pEvent); 
	void EndOfEvent(const G4Event *pEvent);
	void PreUserTrackingAction(const G4Track*);
	void Step(const G4Step *pStep, const G4SteppingManager* pStepMan);	
	
	
	//---------//
	// Setters //
	//---------//
	
	inline void SetRunSeed(G4int hRanSeed) { fRunSeed = hRanSeed; };
	inline void SetSaveData(DatasaveLevel save){ fSave=save; };
	inline void SetStepsDebug(G4bool dbg=true){ fStepsDebug=dbg; };
	inline void SetDataFilename(const G4String &hFilename) { fDataFilename = hFilename; };
	inline void SetNbEventsToSimulate(G4int iNbEventsToSimulate) { fNbEventsToSimulate = iNbEventsToSimulate; };
	inline void SetNbOfPrimariesPerEvent(G4int nPrim){ fNbPrim = nPrim; };
	inline void SetVerbosity(AnalysisVerbosity verb){fVerbose=verb;};
	inline void SetPrintModulo(G4int modulo){fPrintModulo=modulo;};
	inline void SetAutoFlush(Long64_t autof){fAutoFlushEvs=autof;};
	inline void SetAutoSave(Long64_t autos){fAutoSaveEvs=autos;};
	void DefineOptPhSensDet(G4String volList);
	void DefineOptPhAbsVols(G4String volList); //Useful only with optical photons (as of now it is irrelevant)
	
	
	//---------//
	// Getters //
	//---------//
	
	inline G4String GetDataFilename() const { return fDataFilename; };
	inline G4int GetNbEventsToSimulate() const { return fNbEventsToSimulate; };
	inline G4int GetNbOfPrimariesPerEvent() const { return fNbPrim; };
	inline DatasaveLevel GetSaveStatus() const {return fSave;};
	

	
};

#endif

