#include "DetConstr.hh"
#include "PrimGenAction.hh"
#include "AnalysisManager.hh"
#include "AnalysisMessenger.hh"
//#include "OptPhHit.hh"
#include "EventData.hh"

#include "TNamed.h"

#include "G4String.hh"
#include "Randomize.hh"
#include "G4SDManager.hh"
#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4HCofThisEvent.hh"
#include "G4TrackStatus.hh"
#include "G4StepStatus.hh"
#include "G4SteppingManager.hh"
#include "G4VProcess.hh"
#include "G4PhysicalVolumeStore.hh"

#include "CLHEP/Units/SystemOfUnits.h"
#include "CLHEP/Units/PhysicalConstants.h"


#include "nlohmann/json.hpp"


#include <sys/time.h>
#include <numeric>
#include <fstream>
#include <sstream>


using std::vector;
using std::stringstream;
using std::set;
using std::ofstream;

using namespace CLHEP;

using json = nlohmann::json;


G4Mutex mutex_an_man = G4MUTEX_INITIALIZER;


AnalysisManager::AnalysisManager(PrimGenAction *pPrimaryGeneratorAction):
fPrimaryGeneratorAction(pPrimaryGeneratorAction),
fPartSrc(fPrimaryGeneratorAction->GetParticleSource()),
fRunSeed(0),
fNav(nullptr),
fVerbose(AnalysisVerbosity::kSilent),
fPrintModulo(0),
fCurrentEvent(-1),
fCurrentTrackId(-1),
fCurrentTrack(nullptr),
fSave(DatasaveLevel::kOff),
fStepsDebug(false),
fWasAtBoundary(false),
fProcTable(nullptr),
fProcVec(nullptr),
fNprocs(0),
fParticlesTable(nullptr),
fDataFilename("events.root"),
fTreeFile(nullptr),
fTree(nullptr),
fAutoSaveEvs(100),
fAutoFlushEvs(100),
fLastTrackId(-1),
fLastPhysVol(nullptr),
fLastPrePhysVol(nullptr)
{
	fMessenger = new AnalysisMessenger(this);
	
	//fEventData = new EventDataOptPh();
	fEventData = new EventData();
}

AnalysisManager::~AnalysisManager()
{
	if(fEventData) delete fEventData;
	if(fMessenger) delete fMessenger;
}



void AnalysisManager::BeginOfRun(const G4Run *pRun)
{
#ifndef NDEBUG
	if(fVerbose>=AnalysisVerbosity::kDebug)  G4cout << "\nDebug --> Entering into AnalysisManager::BeginOfRun" << G4endl;
#endif
	G4int randseed;
	
	if(fRunSeed > 0){
		CLHEP::HepRandom::setTheEngine(new CLHEP::RanecuEngine);
		CLHEP::HepRandom::setTheSeed(fRunSeed);
		randseed = fRunSeed;
	}else{
		// initialize with time.....
		struct timeval hTimeValue;
		gettimeofday(&hTimeValue, nullptr);
		CLHEP::HepRandom::setTheSeed(hTimeValue.tv_usec);
		randseed = hTimeValue.tv_usec;
	}
	
	if(fVerbose>=AnalysisVerbosity::kInfo) std::cout << "\nInfo --> AnalysisManager::BeginOfRun: Random numbers generator initialized with seed " << randseed << std::endl;
	
	
	fProcTable = G4ProcessTable::GetProcessTable();
	
	fCurrentEvent = -1;
	
	fParticlesTable = G4ParticleTable::GetParticleTable();
	
	MakeVolMaps();
	MakeParticlesMaps();
	if( (fParticlesMap.size()==0) || (fParticlesDefsMap.size()==0) ){
		G4cerr << "\nERROR --> AnalysisManager::BeginOfRun: The book-keeping maps for the particle definitions are empty!" << G4endl;
	}
	
	std::string vol_dict = BuildPhysVolDict();
	std::string sd_vol_dict = BuildSDvolDict();
	std::string proc_dict = BuildProcsDict();
	std::string partdef_dict = BuildParticlesDict();
	
#ifndef NDEBUG	
	if(fVerbose>=AnalysisVerbosity::kDebug){
		G4cout << "\nDebug --> AnalysisManager::BeginOfRun: Content of the \"partdef_dict\":" <<  G4endl;
		G4cout << "          " << partdef_dict << G4endl;
	}
#endif
	
	
	if(fSave<=DatasaveLevel::kOff){
		G4cout << "\nWARNING --> AnalysisManager::BeginOfRun: Data will not be saved." << G4endl;
		return;
	}
	
	fTreeFile = TFile::Open(fDataFilename.c_str(), "RECREATE", "File containing event data of optical photon simulations of ArgonCube");
	
	
	if(!fTreeFile){//Here there is a problem
		fSave = DatasaveLevel::kOff; //Do this to save from application crashing
		return;
	}
	
	
	//All the global level data that has to saved as string in a ROOT 
	//file will be saved inside a TNamed class and all the numerical 
	//parameters will be saved by using a TParameter class

	TNamed *tn_vol_dict = new TNamed("vol_dict", vol_dict.c_str());
	
	TNamed *tn_sdvol_dict = nullptr;
	if(fSenDetVolPtrs.size()>0) tn_sdvol_dict = new TNamed("sdvol_dict", sd_vol_dict.c_str());
	
	TNamed *tn_proc_dict = new TNamed("proc_dict", proc_dict.c_str());
	
	TNamed *tn_partdef_dict = new TNamed("partdef_dict", partdef_dict.c_str());

	TParameter<int>* ptTParNbEventsToSimulate = new TParameter<int>("EventsNb", fNbEventsToSimulate);
	
	fNbPrim = fPrimaryGeneratorAction->GetPrimNb();
	TParameter<int>* ptTParNbPrimariesPerEvent = new TParameter<int>("PrimNb", fNbPrim);
	
	TParameter<G4int>* ptTParRandSeed = new TParameter<G4int>("RandSeed", randseed);
	
	if(fTreeFile){
		fTreeFile->WriteTObject(tn_vol_dict, 0, "overwrite");
		if(tn_sdvol_dict) fTreeFile->WriteTObject(tn_sdvol_dict, 0, "overwrite");
		fTreeFile->WriteTObject(tn_proc_dict, 0, "overwrite");
		fTreeFile->WriteTObject(tn_partdef_dict, 0, "overwrite");
		fTreeFile->WriteTObject(ptTParNbEventsToSimulate, 0, "overwrite");
		fTreeFile->WriteTObject(ptTParNbPrimariesPerEvent, 0, "overwrite");
		fTreeFile->WriteTObject(ptTParRandSeed, 0, "overwrite");
	}
	
	//Clean up the memory from stuff already written on disk
	delete tn_vol_dict;
	if(tn_sdvol_dict) delete tn_sdvol_dict;
	delete tn_proc_dict;
	delete tn_partdef_dict;
	delete ptTParNbEventsToSimulate;
	delete ptTParNbPrimariesPerEvent;
	delete ptTParRandSeed;
	
	
	
	fTree = new TTree("t1", "Tree containing event data for ArgonCube simulations.");
	
	//Lines needed to be able to store std containers in the TTree
	gROOT->ProcessLine("#include <vector>");
	gROOT->ProcessLine("#include <string>");

	fTree->Branch("EvId", &fEventData->fEventId, "eventid/I");
	
	//Primary particle savings
	fTree->Branch("prim_type", "vector<Int_t>", &fEventData->fPrimPartType);
	
	fTree->Branch("prim_vol_index", &fEventData->fPrimaryVolumeIndex, "prim_vol_idx/I");//Fill at start of tracking stage only once
	fTree->Branch("prim_vol_cpnm", &fEventData->fPrimaryVolumeCopyNum, "prim_vol_cpnm/I");//Fill at start of tracking stage only once
	fTree->Branch("prim_vol_globcp", "string", &fEventData->fPrimaryVolumeGlobCp);//Fill at start of tracking stage only once
	fTree->Branch("prim_Xpos", &fEventData->fPrimary_Xpos, "prim_Xpos/D");//Fill at start of tracking stage only once
	fTree->Branch("prim_Ypos", &fEventData->fPrimary_Ypos, "prim_Ypos/D");//Fill at start of tracking stage only once
	fTree->Branch("prim_Zpos", &fEventData->fPrimary_Zpos, "prim_Zpos/D");//Fill at start of tracking stage only once
	fTree->Branch("prim_id", "vector<Int_t>", &fEventData->fPrimary_Id);//Fill at start of tracking stage only once
	fTree->Branch("prim_en", "vector<Double_t>", &fEventData->fPrimEn);//Fill at step stage only once

	//ONLY FOR OPTICAL PHOTONS
	//fTree->Branch("prim_wavelen", "vector<Double_t>", &fEventData->fPrimWaveLength);//This is the wavelength of the optical photon (in nm units), taken from the Ekin. //Fill start of tracking stage only once
	
	if(fSave > DatasaveLevel::kLUT) fTree->Branch("prim_Xmom", "vector<Double_t>", &fEventData->fPrimary_Xmom);//Fill at start of tracking stage only once
	if(fSave > DatasaveLevel::kLUT) fTree->Branch("prim_Ymom", "vector<Double_t>", &fEventData->fPrimary_Ymom);//Fill at start of tracking stage only once
	if(fSave > DatasaveLevel::kLUT) fTree->Branch("prim_Zmom", "vector<Double_t>", &fEventData->fPrimary_Zmom);//Fill at start of tracking stage only once
	if(fSave > DatasaveLevel::kLUT) fTree->Branch("prim_Xpol", "vector<Double_t>", &fEventData->fPrimary_Xpol);//Fill at start of tracking stage only once
	if(fSave > DatasaveLevel::kLUT) fTree->Branch("prim_Ypol", "vector<Double_t>", &fEventData->fPrimary_Ypol);//Fill at start of tracking stage only once
	if(fSave > DatasaveLevel::kLUT) fTree->Branch("prim_Zpol", "vector<Double_t>", &fEventData->fPrimary_Zpol);//Fill at start of tracking stage only once
	
	
	
	//Hits related data
	if(fSave < DatasaveLevel::kSdSteps) fTree->Branch("totalhits", &fEventData->fNbTotHits, "totalhits/L");
	if(fSave < DatasaveLevel::kSdSteps) fTree->Branch("hit_vol_index", "vector<Int_t>", &fEventData->fVolIndex);//ID of the physical volume (it is a whish!). //Fill at step stage
	if(fSave < DatasaveLevel::kSdSteps) fTree->Branch("hit_vol_copy", "vector<Int_t>", &fEventData->fHitVolCopyNum);//This is the copy number of a specific physics volume. //Fill at step stage
	if(fSave < DatasaveLevel::kSdSteps) fTree->Branch("hit_vol_globcp", "vector<string>", &fEventData->fHitVolGlobCp);//This MUST become the unique ID of the touchable volume. //Fill at step stage
	if(fSave < DatasaveLevel::kSdSteps) fTree->Branch("hit_time", "vector<Double_t>", &fEventData->fTime);//Fill at step stage
	//if(fSave < DatasaveLevel::kSdSteps) fTree->Branch("hit_trackid", "vector<Int_t>", &fEventData->fTrackId);//Fill at step stage
	if(fSave < DatasaveLevel::kSdSteps) fTree->Branch("hit_firstparentid", "vector<Int_t>", &fEventData->fFirstParentId); //Fill at step stage
	if(fSave < DatasaveLevel::kSdSteps) fTree->Branch("hit_edep", "vector<Double_t>", &fEventData->fEdep);//Fill at step stage
	
	//ONLY FOR OPTICAL PHOTONS
	//if(fSave < DatasaveLevel::kSdSteps) fTree->Branch("hit_phot_wavelen", "vector<Double_t>", &fEventData->fPhWaveLength); //This is the wavelength of the optical photon (in nm units), taken from the Ekin. //Fill at step stage 
	
	
	//Extended information of hits related data
	if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_part_name", "vector<string>", &fEventData->fPartName);//Fill at stepping stage
	if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_part_type", "vector<Int_t>", &fEventData->fPartType);//Fill at stepping stage
	if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_trackid", "vector<Int_t>", &fEventData->fTrackId);//Fill at step stage
	if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_partgen", "vector<Int_t>", &fEventData->fPartGener);//Fill at step stage

	if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_ekin", "vector<Double_t>", &fEventData->fEkin);//Fill at step stage
	
	//ONLY FOR OPTICAL PHOTONS
	//if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_phot_wvlen", "vector<Double_t>", &fEventData->fPhWaveLength); //This is the wavelength of the optical photon (in nm units), taken from the Ekin. /Fill at step stage.

	if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_xpos", "vector<Double_t>", &fEventData->fXpos);//Fill at step stage
	if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_ypos", "vector<Double_t>", &fEventData->fYpos);//Fill at step stage
	if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_zpos", "vector<Double_t>", &fEventData->fZpos);//Fill at step stage
	
	if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_xmom", "vector<Double_t>", &fEventData->fXmom);//Fill at step stage
	if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_ymom", "vector<Double_t>", &fEventData->fYmom);//Fill at step stage
	if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_zmom", "vector<Double_t>", &fEventData->fZmom);//Fill at step stage
	
	//ONLY FOR OPTICAL PHOTONS
	//if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_xpol", "vector<Double_t>", &fEventData->fXpol);//Fill at step stage
	//if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_ypol", "vector<Double_t>", &fEventData->fYpol);//Fill at step stage
	//if(fSave == DatasaveLevel::kHitsExt) fTree->Branch("hit_zpol", "vector<Double_t>", &fEventData->fZpol);//Fill at step stage
	
	
	//Full step mode
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("totsteps", &fEventData->fNbTotHits, "totsteps/I");//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("part_name", "vector<string>", &fEventData->fPartName);//Fill at stepping stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("part_type", "vector<Int_t>", &fEventData->fPartType);
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("vol_index", "vector<Int_t>", &fEventData->fVolIndex);//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("vol_copy", "vector<Int_t>", &fEventData->fHitVolCopyNum);//ID of the touchable volume//Fill at step stage
	//if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("vol_id", "vector<Long64_t>", &fEventData->fHitVolId);//ID of the touchable volume//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("vol_globcp", "vector<string>", &fEventData->fHitVolGlobCp);//ID of the touchable volume//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("time", "vector<Double_t>", &fEventData->fTime);//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("step_dt", "vector<Double_t>", &fEventData->fDeltaTime);//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("trackid", "vector<Int_t>", &fEventData->fTrackId);//Fill at end of Event
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("partgener", "vector<Int_t>", &fEventData->fPartGener);//Fill at end of Event
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("parentid", "vector<Int_t>", &fEventData->fParentId);//Fill at end of Event
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("firstparentid", "vector<Int_t>", &fEventData->fFirstParentId);//Fill at end of Event
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("creatproc", "vector<Int_t>", &fEventData->fCreatProc);//Fill at end of Event
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("deposproc", "vector<Int_t>", &fEventData->fDepProc);//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("edep", "vector<Double_t>", &fEventData->fEdep);//Fill at step stage
	
	//ONLY FOR OPTICAL PHOTONS
	//if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("wavelen", "vector<Double_t>", &fEventData->fPhWaveLength);//This is the wavelength of the optical photon (in nm units), taken from the Ekin. //Fill at step stage
	
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("xpos", "vector<Double_t>", &fEventData->fXpos);//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("ypos", "vector<Double_t>", &fEventData->fYpos);//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("zpos", "vector<Double_t>", &fEventData->fZpos);//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("xmom", "vector<Double_t>", &fEventData->fXmom);//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("ymom", "vector<Double_t>", &fEventData->fYmom);//Fill at step stage
	if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("zmom", "vector<Double_t>", &fEventData->fZmom);//Fill at step stage
	
	//ONLY FOR OPTICAL PHOTONS
	//if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("xpol", "vector<Double_t>", &fEventData->fXpol);//Fill at step stage
	//if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("ypol", "vector<Double_t>", &fEventData->fYpol);//Fill at step stage
	//if(fSave >= DatasaveLevel::kSdSteps) fTree->Branch("zpol", "vector<Double_t>", &fEventData->fZpol);//Fill at step stage
	
	
	fEventData->fPrimaryVolumeIndex = -1;
	
	//These assignment are for preallocation of memory.
	//The vectors will be resized to 0 at start of event
	fEventData->fPrimEn->assign(fNbPrim,0);

	//ONLY FOR OPTICAL PHOTONS
	//fEventData->fPrimWaveLength->assign(fNbPrim,0);
	
	fEventData->fPrimary_Xmom->assign(fNbPrim,0);
	fEventData->fPrimary_Ymom->assign(fNbPrim,0);
	fEventData->fPrimary_Zmom->assign(fNbPrim,0);
	
	fEventData->fPrimary_Xpol->assign(fNbPrim,0);
	fEventData->fPrimary_Ypol->assign(fNbPrim,0);
	fEventData->fPrimary_Zpol->assign(fNbPrim,0);
	
	//fTree->SetMaxTreeSize((int)1e6);
	fTree->SetAutoFlush(fAutoFlushEvs);
	fTree->SetAutoSave(fAutoSaveEvs);
	
	fTrackIDs.clear();
	fTrackParentIDsMap.clear();
	fTrackGenerationMap.clear();
	fFirstParentIDMap.clear();
	
	
#ifndef NDEBUG
	if(fVerbose>=AnalysisVerbosity::kDebug) G4cout << "\nDebug --> Exiting from AnalysisManager::BeginOfRun" << G4endl;
#endif
}


void AnalysisManager::EndOfRun(const G4Run *pRun)
{
	fProcTable = nullptr;
	fProcVec = nullptr;
	
	if(fSave==DatasaveLevel::kOff) return;
	
	if(fTreeFile){
		if(fTree){
			fTreeFile->WriteTObject(fTree, 0, "overwrite");
			delete fTreeFile; //This deletes also the TTree owned by the TFile
			fTree=nullptr;
		}
		fTreeFile=nullptr;
	}
}


void AnalysisManager::BeginOfEvent(const G4Event *pEvent)
{
	// grab event ID
	fCurrentEvent = pEvent->GetEventID();
	
	// print this information event by event (modulo n)  	
	if( (fVerbose>=AnalysisVerbosity::kInfo) && (fPrintModulo>0) && (fCurrentEvent%fPrintModulo == 0) ){
		G4cout << "\nInfo --> AnalysisManager::BeginOfEvent: Begin of event: " << fCurrentEvent  << G4endl;
	}
	
	//ONLY OPTICAL PHOTONS
	fWasAtBoundary = false;
	
	//These initialisations are needed at the start of event
	fLastTrackId = -1;
	fLastPhysVol = nullptr;
	fLastVolIdx = -1;
	fLastCopyNum = -1;
	fLastVolGlobalCopy = std::string("");

	fEventData->Reset();
	
	fTrackIDs.clear();
	fTrackParentIDsMap.clear();
	fTrackGenerationMap.clear(); 
	fFirstParentIDMap.clear();
	
	
	//Primary particles information
	//Volume id.......
	G4ThreeVector posVec = fPrimaryGeneratorAction->GetPrimPos();
	
	//Here I can still use the tracking navigator as the tracking has not started yet
	if(!fNav) fNav = G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();
	G4VPhysicalVolume *fPrimVol = fNav->LocateGlobalPointAndSetup(posVec, nullptr, true);
	G4TouchableHandle touch = fNav->CreateTouchableHistory();
	
	
	if(fSave>DatasaveLevel::kOff){
		const vector<const G4PrimaryParticle*> *pPrimPartVec = fPartSrc->GetPrimPartVec();
		if(pPrimPartVec->size()==0){
			G4cerr << "\nERROR --> AnalysisManager::BeginOfEvent: The vector of primary particles is empty for Event ID " << fCurrentEvent << "!" << G4endl;
		}

		fEventData->fEventId = fCurrentEvent;
		
		fEventData->fPrimaryVolumeIndex = FindVolumeIndex(fPrimVol);
		fEventData->fPrimaryVolumeCopyNum = touch->GetCopyNumber();
		(*fEventData->fPrimaryVolumeGlobCp) = FindVolGlobalCopy(touch);
		
		fEventData->fPrimary_Xpos = posVec.x();
		fEventData->fPrimary_Ypos = posVec.y();
		fEventData->fPrimary_Zpos = posVec.z();
	}
	
	
#ifndef NDEBUG
	if(fVerbose>=AnalysisVerbosity::kDebug){
		G4cout << "Debug --> AnalysisManager::BeginOfEvent: EventID: " << fCurrentEvent << "; Primary volume: " << fPrimVol->GetName() << "; Copy number: " << touch->GetCopyNumber() << G4endl;
	}
#endif
}


void AnalysisManager::EndOfEvent(const G4Event *pEvent)
{
	if(fSave>DatasaveLevel::kOff){
		G4bool save = true;
		if(fEventData->fPrimPartType->size()==0){
			G4cerr << "\nERROR --> AnalysisManager::EndOfEvent: \"fEventData->fPrimPartType\" vector is empty for Event ID " << fCurrentEvent << "!" << G4endl;
			save = false;
		}
		if(fEventData->fPrimary_Id->size()==0){
			G4cerr << "\nERROR --> AnalysisManager::EndOfEvent: \"fEventData->fPrimary_Id\" vector is empty for Event ID " << fCurrentEvent << "!" << G4endl;
			save = false;
		}
		if(fEventData->fPrimEn->size()==0){
			G4cerr << "\nERROR --> AnalysisManager::EndOfEvent: \"fEventData->fPrimEn\" vector is empty for Event ID " << fCurrentEvent << "!" << G4endl;
			save = false;
		}
		if(fEventData->fPrimary_Xmom->size()==0){
			G4cerr << "\nERROR --> AnalysisManager::EndOfEvent: \"fEventData->fPrimary_Xmom\" vector is empty for Event ID " << fCurrentEvent << "!" << G4endl;
			save = false;
		}
		if(fEventData->fPrimary_Ymom->size()==0){
			G4cerr << "\nERROR --> AnalysisManager::EndOfEvent: \"fEventData->fPrimary_Ymom\" vector is empty for Event ID " << fCurrentEvent << "!" << G4endl;
			save = false;
		}
		if(fEventData->fPrimary_Zmom->size()==0){
			G4cerr << "\nERROR --> AnalysisManager::EndOfEvent: \"fEventData->fPrimary_Zmom\" vector is empty for Event ID " << fCurrentEvent << "!" << G4endl;
			save = false;
		}

		if(fTree && save) fTree->Fill();
	}
}


///////////////////////////////////////////////////////

G4Mutex mutex_an_man_tracking = G4MUTEX_INITIALIZER;
void AnalysisManager::PreUserTrackingAction(const G4Track* pTrack){
	
	G4AutoLock l(&mutex_an_man_tracking);
	l.lock();
	
	fCurrentTrack = (G4Track*)pTrack;
	G4int fCurrentTrackId = fCurrentTrack->GetTrackID();
	G4int parentid = fCurrentTrack->GetParentID();
	G4bool isPrimary = (!fCurrentTrack->GetCreatorProcess()) || (parentid<=0);
	
	//Check if the track is known and if it is a primary track
	//This book-keeping code must be executed whatever the saving options are
	if( fTrackIDs.find(fCurrentTrackId)==fTrackIDs.end() ){
		fTrackParentIDsMap[fCurrentTrackId] = parentid;
		if(isPrimary){ //It is a primary track at its very first step (in the primary volume)
			fTrackGenerationMap[fCurrentTrackId] = 1;
			fFirstParentIDMap[fCurrentTrackId] = fCurrentTrackId;
			
			if(fSave>DatasaveLevel::kOff){
				//If data will be saved the status of the primary track must be saved
				if(fCurrentTrack->GetParticleDefinition()->IsGeneralIon()){
					fEventData->fPrimPartType->push_back(fGenIonId);
				}else{
					fEventData->fPrimPartType->push_back(fParticlesDefsMap[fCurrentTrack->GetParticleDefinition()]);
				}
				
				fEventData->fPrimPartName->push_back(fCurrentTrack->GetParticleDefinition()->GetParticleName());
				fEventData->fPrimary_Id->push_back(fCurrentTrackId);
				
				fEventData->fPrimEn->push_back( fCurrentTrack->GetVertexKineticEnergy() );

				//FOR OPTICAL PHOTONS
				//fEventData->fPrimWaveLength->push_back( h_Planck*c_light/(fCurrentTrack->GetVertexKineticEnergy())/nm );
				
				const G4ThreeVector& momdir = fCurrentTrack->GetVertexMomentumDirection();
				
				fEventData->fPrimary_Xmom->push_back( momdir.x() );
				fEventData->fPrimary_Ymom->push_back( momdir.y() );
				fEventData->fPrimary_Zmom->push_back( momdir.z() );
				
				//There is no a G4Track::GetVertexPolarizationDirection() interface and must use this way
				if(fCurrentTrack->GetCurrentStepNumber()==0){
					//This is a redundant control but useful anyway
					const G4ThreeVector& poldir = fCurrentTrack->GetPolarization();
					
					fEventData->fPrimary_Xpol->push_back( poldir.x() );
					fEventData->fPrimary_Ypol->push_back( poldir.y() );
					fEventData->fPrimary_Zpol->push_back( poldir.z() );
				}
				
			}
		}else{
			fTrackGenerationMap[fCurrentTrackId] = fTrackGenerationMap[parentid]+1;
			fFirstParentIDMap[fCurrentTrackId] = fFirstParentIDMap[parentid];
		}
		
		
		if( fProcTable && (fSave>=DatasaveLevel::kSdSteps) ){
			if(isPrimary){//It's a primary track
				fTrackCreatProc[fCurrentTrackId] = 0;
			}else{
				int retcode = FindProcessIndex(fCurrentTrack->GetCreatorProcess()); //This is the index if the process is found (>=0), otherwise a negative return code is returned
				if(retcode>=0){
					fTrackCreatProc[fCurrentTrackId] = retcode+1; //Add 1 to the index as 0 is reserved for primary tracks (no creator process)
				}else{
					fTrackCreatProc[fCurrentTrackId] = retcode; //This is a negative number and indicates for problems
				}
			}
		}
		
		fTrackIDs.insert(fCurrentTrackId);
	}
	l.unlock();
}


void AnalysisManager::Step(const G4Step *pStep, const G4SteppingManager* pStepMan)
{
#ifndef NDEBUG
	if(fVerbose>=AnalysisVerbosity::kDebug) G4cout << "\nDebug --> Entering in AnalysisManager::Step\n" << G4endl;
#endif
	//if( (fVerbose<3) && (fOptPhSenDetVolNames.empty()) )return;
	if(!pStep) return; //This just avoids problems, although would be a big problem to be fixed
	
	G4AutoLock l(&mutex_an_man);
	l.lock();
	
	if(fCurrentTrack!=pStep->GetTrack()){
		G4cerr << "\nERROR --> AnalysisManager::Step: The track pointer from the G4Step is different from the \"fCurrentTrack\" pointer for Event ID: " << fCurrentEvent << "." << G4endl;
	}
	//G4Track *track = pStep->GetTrack();
	
	G4StepPoint *preStepPoint = pStep->GetPreStepPoint();
	
	G4StepPoint *postStepPoint = pStep->GetPostStepPoint();
	
	G4TouchableHandle touch = postStepPoint->GetTouchableHandle();
	G4VPhysicalVolume *Vol = touch->GetVolume();
	
	G4TouchableHandle touch_pre = preStepPoint->GetTouchableHandle();
	G4VPhysicalVolume *Vol_pre = preStepPoint->GetTouchableHandle()->GetVolume();
	
	G4String partName = fCurrentTrack->GetParticleDefinition()->GetParticleName();
	
	//This is the step point from where the stuff is saved. It changes to preStepPoint only in saving mode and in the case the track is absorbed in the physical volume soon after it went through the boundary surface.
	G4StepPoint *saveStepPoint = postStepPoint;
	G4VPhysicalVolume *saveVol = Vol;
	G4TouchableHandle *saveTouch = &touch; //This is from where I get the current quantities
	G4TouchableHandle *preTouch = &touch_pre; //This defines where the step occurred
	
	if(!Vol) return;
	
	
	G4TrackStatus trstatus = fCurrentTrack->GetTrackStatus();
	
	//Volume printouts
	if(fVerbose>=AnalysisVerbosity::kDetails || fStepsDebug){
		G4String TrackStat = "";
		
		if(trstatus==fAlive) TrackStat = "Alive";
		if(trstatus==fStopButAlive) TrackStat = "StopButAlive";
		if(trstatus==fStopAndKill) TrackStat = "StopAndKill";
		if(trstatus==fKillTrackAndSecondaries) TrackStat = "KillTrackAndSecondaries";
		if(trstatus==fSuspend) TrackStat = "Suspend";
		if(trstatus==fPostponeToNextEvent) TrackStat = "PostponeToNextEvent";
		
		
		
		if((postStepPoint->GetStepStatus()==fGeomBoundary) || fWasAtBoundary){
#ifndef NDEBUG
			if(fStepsDebug){
				if(!fWasAtBoundary){
					G4cout << "\nStepDebug --> " << "Event " << fCurrentEvent << ", trackID: " << fCurrentTrack->GetTrackID() << ", Name: " << partName << ". Particle at volumes boundary:" << G4endl;
					G4cout << "               Volume 1: <" << Vol_pre->GetName() << ">, copy num: " << touch_pre->GetCopyNumber() << G4endl; 
					G4cout << "               Volume 2: <" << Vol->GetName() << ">, copy num:" << touch->GetCopyNumber() << G4endl;
					G4cout << "           Track status: " << TrackStat << G4endl;
					G4cout << "              Sel. proc: " << postStepPoint->GetProcessDefinedStep()->GetProcessName() << G4endl;
					G4cout << "   First step in volume: " << pStep->IsFirstStepInVolume() << G4endl;
					G4cout << "    Last step in volume: " << pStep->IsLastStepInVolume() << G4endl;
					G4cout << "            Step length: " << pStep->GetStepLength() << G4endl;
					G4cout << "       Deposited energy: " << pStep->GetTotalEnergyDeposit() << G4endl;
					//fWasAtBoundary = true;
				}else{
					G4cout << "\nStepDebug --> " << "Event " << fCurrentEvent << ", trackID: " << fCurrentTrack->GetTrackID() << ", Name: " << partName << ". Particle after volumes boundary:" << G4endl;
					G4cout << "               Volume 1: <" << Vol_pre->GetName() << ">, copy num: " << touch_pre->GetCopyNumber() << G4endl; 
					G4cout << "               Volume 2: <" << Vol->GetName() << ">, copy num:" << touch->GetCopyNumber() << G4endl;
					G4cout << "           Track status: " << TrackStat << G4endl;
					G4cout << "              Sel. proc: " << postStepPoint->GetProcessDefinedStep()->GetProcessName() << G4endl;
					G4cout << "   First step in volume: " << pStep->IsFirstStepInVolume() << G4endl;
					G4cout << "    Last step in volume: " << pStep->IsLastStepInVolume() << G4endl;
					G4cout << "            Step length: " << pStep->GetStepLength() << G4endl;
					G4cout << "       Deposited energy: " << pStep->GetTotalEnergyDeposit() << G4endl;
					//fWasAtBoundary = false;
				}
				//std::string foo;
				//G4cout << "Press a enter to continue..."; std::cin >> foo;
			}else{
			
				if( (fSave==DatasaveLevel::kAll) || (fSenDetVolPtrs.find(Vol)!=fSenDetVolPtrs.end()) ){
					G4cout << "Detail ---> AnalysisManager::Step:\n" << "     Event " << fCurrentEvent << ", trackID: " << fCurrentTrack->GetTrackID() << ", Name: " << partName << ". Particle at volumes boundary"<< G4endl;
					G4cout << "               Volume 1: <" << Vol_pre->GetName() << ">, copy num: " << touch_pre->GetCopyNumber() << G4endl; 
					G4cout << "               Volume 2: <" << Vol->GetName() << ">, copy num:" << touch->GetCopyNumber() << G4endl;
					G4cout << "           Track status: " << TrackStat << G4endl;
					G4cout << "               Sel proc: " << postStepPoint->GetProcessDefinedStep()->GetProcessName() << G4endl;
					G4cout << "   First step in volume: " << pStep->IsFirstStepInVolume() << G4endl;
					G4cout << "    Last step in volume: " << pStep->IsLastStepInVolume() << G4endl;
					G4cout << "            Step length: " << pStep->GetStepLength() << G4endl;
					G4cout << "       Deposited energy: " << pStep->GetTotalEnergyDeposit() << G4endl;
				}
			}
#endif
		}else{
			//Print out for non-boundary conditions
			if( ((fSave==DatasaveLevel::kAll) || (fSenDetVolPtrs.find(Vol)!=fSenDetVolPtrs.end())) ){
				G4cout << "Detail ---> AnalysisManager::Step:\n" << "     Event " << fCurrentEvent << ", trackID: " << fCurrentTrack->GetTrackID() << ", Name: " << partName << "."<< G4endl;
				G4cout << "               Volume 1: <" << Vol_pre->GetName() << ">, copy num: " << touch_pre->GetCopyNumber() << G4endl; 
				G4cout << "               Volume 2: <" << Vol->GetName() << ">, copy num:" << touch->GetCopyNumber() << G4endl;
				G4cout << "           Track status: " << TrackStat << G4endl;
				G4cout << "               Sel proc: " << postStepPoint->GetProcessDefinedStep()->GetProcessName() << G4endl;
				G4cout << "   First step in volume: " << pStep->IsFirstStepInVolume() << G4endl;
				G4cout << "    Last step in volume: " << pStep->IsLastStepInVolume() << G4endl;
				G4cout << "            Step length: " << pStep->GetStepLength() << G4endl;
				G4cout << "       Deposited energy: " << pStep->GetTotalEnergyDeposit() << G4endl;
			}
		}
	} //Closes the "if(fVerbose>=AnalysisVerbosity::kDetails || fStepsDebug)" block
	
	
	if((postStepPoint->GetStepStatus()==fGeomBoundary) && (!fWasAtBoundary)){
		fWasAtBoundary = true;
		fLastPrePhysVol = Vol_pre;
	}else{
		//For optical photons it might happen that in successive steps they go from a boundary to another
		if(postStepPoint->GetStepStatus()!=fGeomBoundary) fWasAtBoundary = false;
	}
	
	
	if( fSave==DatasaveLevel::kOff ){
#ifndef NDEBUG
		if(fVerbose>=AnalysisVerbosity::kDebug) G4cout << "\nDebug --> AnalysisManager::Step: Exiting from method without saving.\n" << G4endl;
#endif
		return;
	}
	
	
	
	//For saves modes lower than kAll check whether the particle is in one of the sensitive volumes defined by the user
	if( (fSave<DatasaveLevel::kAll) ){
		//Here the mode is either "SD stepping mode" or one of the "hits modes"
		
		if( fSenDetVolPtrs.find(Vol_pre)==fSenDetVolPtrs.end() ){
			//The step occurred in a sens vol if the pre_step point physical volume is a sensitive volume
			//In all other saving modes I am interested only in hits or steps in specific physical volumes (sensitive volumes)
#ifndef NDEBUG
			if(fVerbose>=AnalysisVerbosity::kDebug){
				G4cout << "\nDebug --> AnalysisManager::Step: Hit in <" << Vol_pre->GetName() << "> volume. Exiting the function." << G4endl;
			}
#endif
			return;
		}
	}

#ifndef NDEBUG	
	if(fVerbose>=AnalysisVerbosity::kDebug){
		G4cout << "\nDebug --> AnalysisManager::Step: Hit in <" << Vol_pre->GetName() << "> volume. Saving the hit." << G4endl;
	}
#endif

	//Here start to get stuff to be saved
	fEventData->fNbTotHits += 1;//This is the number of the total recorded steps in "stepping mode" or the number energy releases of the particle is in a SD volume when in "hits mode"
	
	
	//Recalculate the volume id (recursive process) only if the volume pointer is different from before
	if( (Vol_pre!=fLastPhysVol) || (fCurrentTrackId!=fLastTrackId) )
	{
		fLastTrackId = fCurrentTrackId;
		if(fCurrentTrack->GetParticleDefinition()->IsGeneralIon()){
			fLastPartType = fGenIonId;
		}else{
			fLastPartType = fParticlesDefsMap[fCurrentTrack->GetParticleDefinition()];
		}
		fLastPhysVol = Vol_pre;
		fLastVolIdx = fPhysVolUniqueMap[Vol_pre];
		fLastCopyNum = (*preTouch)->GetCopyNumber();
		fLastVolGlobalCopy = FindVolGlobalCopy(*preTouch);
	}
	
	
	//The physical volume where the step occurred is the one taken from the "pre-step point", but the physical quantities are taken from the post step point.
	//In this way I will miss the physical quantities, like Ekin, Mom, Direct., polar, of the particle at the moment of entering in the sensitive volume.
	
	fEventData->fVolIndex->push_back( fLastVolIdx ); //Index of the physical volume (from a std::map)
	fEventData->fHitVolCopyNum->push_back( fLastCopyNum ); //Copy number of the physical volume
	fEventData->fHitVolGlobCp->push_back( fLastVolGlobalCopy ); //Global copy number of the PV considering the entire geometry tree
	fEventData->fFirstParentId->push_back( fFirstParentIDMap[fLastTrackId] );
	fEventData->fTime->push_back( saveStepPoint->GetGlobalTime() );
	fEventData->fEdep->push_back( pStep->GetTotalEnergyDeposit() );
	
	
	if(fSave>=DatasaveLevel::kHitsExt){
		
		fEventData->fTrackId->push_back( fLastTrackId );
		fEventData->fPartType->push_back(fLastPartType);
		fEventData->fPartName->push_back(fCurrentTrack->GetParticleDefinition()->GetParticleName());
		fEventData->fEkin->push_back( saveStepPoint->GetKineticEnergy() );
		fEventData->fXpos->push_back( (saveStepPoint->GetPosition()).x() );
		fEventData->fYpos->push_back( (saveStepPoint->GetPosition()).y() );
		fEventData->fZpos->push_back( (saveStepPoint->GetPosition()).z() );
		fEventData->fXmom->push_back( (saveStepPoint->GetMomentumDirection()).x() );
		fEventData->fYmom->push_back( (saveStepPoint->GetMomentumDirection()).y() );
		fEventData->fZmom->push_back( (saveStepPoint->GetMomentumDirection()).z() );
		//fEventData->fXpol->push_back( (saveStepPoint->GetPolarization()).x() );
		//fEventData->fYpol->push_back( (saveStepPoint->GetPolarization()).y() );
		//fEventData->fZpol->push_back( (saveStepPoint->GetPolarization()).z() );
		
		if(fSave>=DatasaveLevel::kSdSteps){
			fEventData->fDeltaTime->push_back(pStep->GetDeltaTime());
			if(fProcTable){
				
				if(!saveStepPoint->GetProcessDefinedStep()){
					fEventData->fDepProc->push_back( 0 );//This is a primary track!
				}else{
					int retcode = FindProcessIndex(saveStepPoint->GetProcessDefinedStep()); //This is the index if the process is found (>=0), otherwise a negative return code is returned
					if(retcode>=0){
						fEventData->fDepProc->push_back( retcode+1 );//Add 1 to the index as 0 is reserved for primary tracks
					}else{
						fEventData->fDepProc->push_back( retcode );//This is a negative number and indicates for problems
					}
				}
			}
			
		} // if(fSave>=kSdSteps)...
	} // if(fSave>=kHitsExt)...
	
	l.unlock();
	
#ifndef NDEBUG
	if(fVerbose>=AnalysisVerbosity::kDebug || fStepsDebug) G4cout << "\nDebug --> AnalysisManager::Step: Exiting from method.\n" << G4endl;
#endif
}


//Service methods
void AnalysisManager::DefineOptPhSensDet(G4String volList)
{
	fSenDetVolPtrs.clear();
	
	if(volList == G4String("NULL")){
		return;
	}
	
	G4PhysicalVolumeStore *pPhysVolStore = G4PhysicalVolumeStore::GetInstance();
	
	G4int nVols = pPhysVolStore->size();
	
	if(nVols <= 0) return;
	
	stringstream hStream;
	hStream.str(volList);
	G4String hVolumeName;
	
	
	// store all the volume names
	std::set<G4String> candidatevolnames;
	while(!hStream.eof()){
		hStream >> hVolumeName;
		candidatevolnames.insert(hVolumeName);
		if(!hStream) continue;
	}
	
	
	for(set<G4String>::iterator pIt = candidatevolnames.begin(); pIt != candidatevolnames.end(); pIt++){
		G4String hRequiredVolumeName = *pIt;
		G4bool bMatch = (hRequiredVolumeName.last('*') != std::string::npos);
		
		if(bMatch) hRequiredVolumeName = hRequiredVolumeName.strip(G4String::trailing, '*');
		
		for(G4int iVol=0; iVol<nVols; iVol++){
			G4String hName = pPhysVolStore->at(iVol)->GetName();
			
			if( (hName == hRequiredVolumeName) || (bMatch && (hName.substr(0, hRequiredVolumeName.size())) == hRequiredVolumeName) ){
				fSenDetVolPtrs.insert(pPhysVolStore->at(iVol));
			}
		}
	}
}


void AnalysisManager::DefineOptPhAbsVols(G4String volList)
{
	fOptPhAbsVolPtrs.clear();
	
	
	if(volList == G4String("NULL")){
		return;
	}
	
	G4PhysicalVolumeStore *pPhysVolStore = G4PhysicalVolumeStore::GetInstance();
	
	G4int nVols = pPhysVolStore->size();
	
	if(nVols <= 0) return;
	
	
	stringstream hStream;
	hStream.str(volList);
	G4String hVolumeName;
	
	
	// store all the volume names
	std::set<G4String> candidatevolnames;
	while(!hStream.eof()){
		hStream >> hVolumeName;
		candidatevolnames.insert(hVolumeName);
		if(!hStream) continue;
	}
	
	
	for(set<G4String>::iterator pIt = candidatevolnames.begin(); pIt != candidatevolnames.end(); pIt++){
		G4String hRequiredVolumeName = *pIt;
		G4bool bMatch = (hRequiredVolumeName.last('*') != std::string::npos);
		
		if(bMatch) hRequiredVolumeName = hRequiredVolumeName.strip(G4String::trailing, '*');
		
		for(G4int iVol=0; iVol<nVols; iVol++){
			G4String hName = pPhysVolStore->at(iVol)->GetName();
			
			if( (hName == hRequiredVolumeName) || (bMatch && (hName.substr(0, hRequiredVolumeName.size())) == hRequiredVolumeName) ){
				fOptPhAbsVolPtrs.insert(pPhysVolStore->at(iVol));
			}
		}
	}
}


///////////////////////////////////////////////////////

void AnalysisManager::MakeVolMaps()
{
	fPhysVolMap.clear();
	fPhysVolNamesMap.clear();
	fPhysVolUniqueNamesMap.clear();
	fPhysVolCpnmMap.clear();
	
	const G4VPhysicalVolume* worldPhysVol = (dynamic_cast<const DetConstr*> (G4RunManager::GetRunManager()->GetUserDetectorConstruction()) )->GetWorldVolume();
	
	if(worldPhysVol->GetMotherLogical()){
		//This is not the world volume!!!
		return;
	}
	
	
	int volindex = 0;
	fPhysVolMap[(G4VPhysicalVolume*)worldPhysVol] = volindex;
	fPhysVolNamesMap[volindex] = worldPhysVol->GetName();
	fPhysVolCpnmMap[(G4VPhysicalVolume*)worldPhysVol] = worldPhysVol->GetCopyNo();
	
	ScanVols(worldPhysVol->GetLogicalVolume(), volindex);
	
	
	int idx=0;
	std::map<G4VPhysicalVolume*, int>::iterator it;
	std::map<G4String, int> names_map;
	for(it=fPhysVolMap.begin(); it!=fPhysVolMap.end(); it++){
		if( names_map.find(it->first->GetName())==names_map.end() ){
			fPhysVolUniqueNamesMap[idx] = it->first->GetName();
			names_map[it->first->GetName()] = idx;
			idx++;
		}
		fPhysVolUniqueMap[it->first] = names_map[it->first->GetName()];
	}
}


///////////////////////////////////////////////////////

void AnalysisManager::ScanVols(const G4LogicalVolume* LogVol, int& volindex)
{
	G4int nDaught= LogVol->GetNoDaughters();
	
	G4VPhysicalVolume* PhysVol;
	
	for(G4int iDtr=0; iDtr<nDaught; iDtr++){
		PhysVol = LogVol->GetDaughter(iDtr);
		if( fPhysVolMap.find( PhysVol )==fPhysVolMap.end() ){
			volindex++;
			fPhysVolMap[PhysVol] = volindex;
			fPhysVolNamesMap[volindex] = PhysVol->GetName();
			fPhysVolCpnmMap[PhysVol] = PhysVol->GetCopyNo();
		}
	}
	
	
	for(G4int iDtr=0; iDtr<nDaught; iDtr++){
		PhysVol = LogVol->GetDaughter(iDtr);
		ScanVols(PhysVol->GetLogicalVolume(), volindex);
	}
}


///////////////////////////////////////////////////////

int AnalysisManager::FindProcessIndex( const G4VProcess* aProcess )
{
	if(!aProcess) return -1;//This is a return code
	
	if(!fProcVec){
		if(!fProcTable){
			fProcTable = G4ProcessTable::GetProcessTable();
			if(!fProcTable) return -2; //This is a problem!
		}
		fProcVec = fProcTable->FindProcesses();
		if(!fProcVec) return -3;
		fNprocs = fProcVec->size();
	}
	
	for (int iProc = 0; iProc<fNprocs; iProc++) {
		if((*fProcVec)(iProc)==aProcess){
			return iProc;
		}
	}
	
	return -4;//This should not happen at this stage as the process is not found in the list of all processes
	
}


///////////////////////////////////////////////////////

#include "nlohmann/json.hpp"

int AnalysisManager::FindVolumeIndex( const G4VPhysicalVolume* aVolume )
{
	if(!aVolume) return -1;//This is an error return code
	
	if(fPhysVolUniqueMap.size()==0){
		return -2;
	}
	
	return fPhysVolUniqueMap[(G4VPhysicalVolume*)aVolume];
}


///////////////////////////////////////////////////////

std::string AnalysisManager::BuildProcsDict()
{
	fProcessesMap.clear();
	
	std::string dictstr("");
	
	if(!fProcVec){
		if(!fProcTable){
			fProcTable = G4ProcessTable::GetProcessTable();
			if(!fProcTable) return dictstr; //This is a problem!
		}
		fProcVec = fProcTable->FindProcesses();
		if(!fProcVec) return dictstr;
	}
	
	fNprocs = fProcVec->size();
	
	for(int iProc=0; iProc<fNprocs; iProc++) {
		fProcessesMap[iProc] = (*fProcVec)(iProc)->GetProcessName();
	}
	
	if(fProcessesMap.size()!=0){
		//Make the json dict
		json obj;
		
		stringstream ss_tmp; ss_tmp.str("");
		std::map<int, G4String>::iterator it;
		for(it=fProcessesMap.begin(); it!=fProcessesMap.end(); it++){
			ss_tmp << it->first;
			//obj[ std::stoi(it->first).c_str() ] = it->second;
			obj[ ss_tmp.str().c_str() ] = it->second;
			ss_tmp.str("");
		}
		
		dictstr = obj.dump();
	}
}


///////////////////////////////////////////////////////

void AnalysisManager::MakeParticlesMaps()
{
#ifndef NDEBUG
	if(fVerbose>=AnalysisVerbosity::kDebug){
		G4cout << "\nDebug --> AnalysisManager::MakeParticlesMaps: Start of the method." << G4endl;
	}
#endif
	
	fParticlesMap.clear();
	fParticlesDefsMap.clear();
	
	if(!fParticlesTable){
		fParticlesTable = G4ParticleTable::GetParticleTable();
		if(!fParticlesTable){
			G4cerr << "\nERROR --> AnalysisManager::MakeParticlesMaps: Null pointer to the particle table!" << G4endl;
			return; //This is a problem!
		}
	}
	
	
	G4int pnum = 0;
	
	auto piter = fParticlesTable->GetIterator();
	
	piter -> reset();
	if(fVerbose>=AnalysisVerbosity::kDetails){
		G4cout << "\nDetail --> AnalysisManager::MakeParticlesMaps: Iterating over the particle table:" << G4endl;
	}
	while( (*piter)() ){
		if(fVerbose>=AnalysisVerbosity::kDetails){
			G4cout << "    \t" << piter->value() << ";\t " << pnum << ";\t " << piter->value()->GetParticleName() << G4endl;
		}
		fParticlesDefsMap[piter->value()] = pnum;
		fParticlesMap[pnum] = piter->value()->GetParticleName();
		pnum++;
	}
	//Ions are not included. Put a manual entry only in the dictionary for the analysis
	fGenIonId = pnum;
	fParticlesMap[fGenIonId] = G4String("ion");
	
	if(fVerbose>=AnalysisVerbosity::kDetails){
		G4cout << "\nInfo --> AnalysisManager::MakeParticlesMaps: Found " << fParticlesMap.size() << " particle definitions including the general class of ions." << G4endl;
	}
	
#ifndef NDEBUG
	if(fVerbose>=AnalysisVerbosity::kDebug){
		G4cout << "\nDebug --> AnalysisManager::MakeParticlesMaps: End of the method." << G4endl;
	}
#endif
}


///////////////////////////////////////////////////////

std::string AnalysisManager::BuildParticlesDict()
{
#ifndef NDEBUG
	if(fVerbose>=AnalysisVerbosity::kDebug){
		G4cout << "\nDebug --> AnalysisManager::BuildParticlesDict: Start of the method." << G4endl;
	}
#endif
	
	std::string dictstr = "";
	
	if(!fParticlesTable){
		fParticlesTable = G4ParticleTable::GetParticleTable();
		if(!fParticlesTable) return dictstr; //This is a problem!
		MakeParticlesMaps();
	}
	
	if(fParticlesMap.size()==0){
		G4cerr << "\nWARNING --> AnalysisManager::BuildParticlesDict: The map of the particle definition is empty!" << G4endl;
		return dictstr;
	}
	
	//Make the json dict
	json obj;
	
	stringstream ss_tmp; ss_tmp.str("");
	std::map<int, G4String>::iterator it;
	
#ifndef NDEBUG
	if(fVerbose>=AnalysisVerbosity::kDebug){
		G4cout << "\nDebug --> AnalysisManager::BuildParticlesDict: Scansion of the \"fParticlesMap\":" <<  G4endl;
	}
#endif
	
	for(auto iT=fParticlesMap.begin(); iT!=fParticlesMap.end(); ++iT){
		ss_tmp.str(""); ss_tmp << iT->first;
		//obj[ std::stoi(it->first).c_str() ] = it->second;
		obj[ ss_tmp.str().c_str() ] = iT->second;
#ifndef NDEBUG
		if(fVerbose>=AnalysisVerbosity::kDebug){
			G4cout << "    " << ss_tmp.str().c_str() << ":" << iT->second <<  G4endl;
		}
#endif
	}
	
	dictstr = obj.dump();
	
#ifndef NDEBUG
	if(fVerbose>=AnalysisVerbosity::kDebug){
		G4cout << "\nDebug --> AnalysisManager::BuildParticlesDict: End of the method." << G4endl;
	}
#endif
	return dictstr;
}


///////////////////////////////////////////////////////

std::string AnalysisManager::BuildPhysVolDict()
{
	std::string dictstr("");
	
	if(fPhysVolUniqueNamesMap.size()>0){
		//Make the json dict
		json obj;
		
		stringstream ss_tmp; ss_tmp.str("");
		std::map<int, G4String>::iterator it;
		for(it=fPhysVolUniqueNamesMap.begin(); it!=fPhysVolUniqueNamesMap.end(); it++){
			ss_tmp << it->first;
			//obj[ std::stoi(it->first).c_str() ] = it->second;
			obj[ ss_tmp.str().c_str() ] = it->second;
			ss_tmp.str("");
		}
		
		dictstr = obj.dump();
	}
	
	return dictstr;
}


///////////////////////////////////////////////////////

std::string AnalysisManager::BuildSDvolDict()
{
	std::string dictstr("");
	
	if(fSenDetVolPtrs.size()>0){
		//Make the json dict
		json obj;
		
		stringstream ss_tmp; ss_tmp.str("");
		std::set<G4VPhysicalVolume*>::iterator it;
		for(it=fSenDetVolPtrs.begin(); it!=fSenDetVolPtrs.end(); it++){
			ss_tmp << fPhysVolUniqueMap[*it];
			//obj[ std::stoi(it->first).c_str() ] = it->second;
			obj[ ss_tmp.str().c_str() ] = fPhysVolUniqueNamesMap[fPhysVolUniqueMap[*it]];
			ss_tmp.str("");
		}
		
		dictstr = obj.dump();
	}
	
	return dictstr;
}


///////////////////////////////////////////////////////

std::string AnalysisManager::FindVolGlobalCopy(const G4TouchableHandle& touch)
{
	G4int nLevs = touch->GetHistoryDepth();
	
	vector<Long64_t> copyNumVec(nLevs+1);
	
	for(int iLev=0; iLev<=nLevs; iLev++){
		//pCpNumVec->at(nLevs-iLev) = touch->GetCopyNumber(iLev);
		copyNumVec.at(nLevs-iLev) = touch->GetCopyNumber(iLev);
		
		if((touch->GetCopyNumber(iLev))<0){
			std::cout << " copynumber: " << touch->GetCopyNumber(iLev) << ", volname: " << touch->GetVolume(iLev)->GetName() << std::endl;
		}
		
	}
	
	stringstream ss_tmp; ss_tmp.str("");
	for(unsigned iCp=0; iCp<copyNumVec.size(); iCp++){
		if(iCp==0){
			ss_tmp << copyNumVec.at(iCp);
		}else{
			ss_tmp << "." << copyNumVec.at(iCp);
		}
	}
	
	return ss_tmp.str();
}
