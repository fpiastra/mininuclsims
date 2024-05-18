#include "EventData.hh"

#include "G4Threading.hh"
#include "G4AutoLock.hh"

G4Mutex mutex_ev_data = G4MUTEX_INITIALIZER;

EventData::EventData()
{
	fEventId = -1;
	//fPrimaryVolume = new vector<int>;
	
	fPrimPartName = new vector<string>;
	fPrimPartType = new vector<int>;

	fPrimEn = new vector<Double_t>;
	fPrimWaveLength = new vector<Double_t>;
	
	fPrimaryVolumeIndex=-1;
	fPrimaryVolumeCopyNum=-1;
	fPrimaryVolumeGlobCp = new string;
	
	fPrimary_Xmom = new vector<Double_t>;
	fPrimary_Ymom = new vector<Double_t>;
	fPrimary_Zmom = new vector<Double_t>;
	
	fPrimary_Xpol = new vector<Double_t>;
	fPrimary_Ypol = new vector<Double_t>;
	fPrimary_Zpol = new vector<Double_t>;
	
	
	fNbTotHits = -1;
	
	fVolIndex = new vector<Int_t>;
	fHitVolCopyNum = new vector<Int_t>;
	fHitVolGlobCp = new vector<string>;
	fTime = new vector<Double_t>;
	
	fPartName = new vector<string>;
	fPartType = new vector<int>;
	fTrackId = new vector<Int_t>;
	fPrimary_Id = new vector<Int_t>;
	fPartGener = new vector<Int_t>;
	
	fParentId = new vector<Int_t>;
	
	fEkin = new vector<Double_t>;
	fPhWaveLength = new vector<Double_t>;
	
	fXpos = new vector<Double_t>;
	fYpos = new vector<Double_t>;
	fZpos = new vector<Double_t>;
	
	fXmom = new vector<Double_t>;
	fYmom = new vector<Double_t>;
	fZmom = new vector<Double_t>;
	
	fXpol = new vector<Double_t>;
	fYpol = new vector<Double_t>;
	fZpol = new vector<Double_t>;
	
	fCreatProc = new vector<Int_t>;
	fFirstParentId = new vector<Int_t>;
	fDepProc = new vector<Int_t>;

	fEdep = new vector<Double_t>;
	fDeltaTime = new vector<Double_t>;
}


EventData::~EventData()
{
	//delete fPrimaryVolume;
	delete fPrimaryVolumeGlobCp;
	
	delete fPrimPartName;
	delete fPrimPartType;

	delete fPrimEn;
	delete fPrimWaveLength;
	
	delete fPrimary_Xmom;
	delete fPrimary_Ymom;
	delete fPrimary_Zmom;
	
	delete fPrimary_Xpol;
	delete fPrimary_Ypol;
	delete fPrimary_Zpol;
	
	delete fVolIndex;
	delete fHitVolCopyNum;
	
	delete fHitVolGlobCp;
	delete fTime;
	
	delete fPartName;
	delete fPartType;
	delete fTrackId;
	delete fPrimary_Id;
	
	delete fPartGener;
	
	delete fParentId;
	
	delete fEkin;
	delete fPhWaveLength;
	
	delete fXpos;
	delete fYpos;
	delete fZpos;
	
	delete fXmom;
	delete fYmom;
	delete fZmom;
	
	delete fXpol;
	delete fYpol;
	delete fZpol;
	
	delete fCreatProc;
	delete fFirstParentId;
	delete fDepProc;

	delete fEdep;
	delete fDeltaTime;
}


void EventData::Reset()
{
	G4AutoLock l(&mutex_ev_data);
	l.lock();

	fEventId = -1;
	
	fPrimPartName->resize(0);
	fPrimPartType->resize(0);

	fPrimaryVolumeGlobCp->resize(0);
	
	fPrimEn->resize(0);
	fPrimWaveLength->resize(0);
	
	fPrimaryVolumeIndex=-1;
	fPrimaryVolumeCopyNum=-1;
	
	fPrimary_Xmom->resize(0);
	fPrimary_Ymom->resize(0);
	fPrimary_Zmom->resize(0);
	
	fPrimary_Xpol->resize(0);
	fPrimary_Ypol->resize(0);
	fPrimary_Zpol->resize(0);
	
	fPrimary_Id->resize(0);
	
	fNbTotHits = 0;
	
	fVolIndex->resize(0);
	fHitVolCopyNum->resize(0);
	fHitVolGlobCp->resize(0);
	fTime->resize(0);
	
	fPartName->resize(0);
	fPartType->resize(0);
	fTrackId->resize(0);
	
	fPartGener->resize(0);
	fParentId->resize(0);
	
	fEkin->resize(0);
	fPhWaveLength->resize(0);
	
	fXpos->resize(0);
	fYpos->resize(0);
	fZpos->resize(0);
	
	fXmom->resize(0);
	fYmom->resize(0);
	fZmom->resize(0);
	
	fXpol->resize(0);
	fYpol->resize(0);
	fZpol->resize(0);
	
	fCreatProc->resize(0);
	fFirstParentId->resize(0);
	fDepProc->resize(0);

	fEdep->resize(0);
	fDeltaTime->resize(0);

	l.unlock();
}

