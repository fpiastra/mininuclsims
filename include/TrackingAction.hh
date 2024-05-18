#ifndef TRACK_ACTION_HH
#define TRACK_ACTION_HH

#include "G4UserTrackingAction.hh"
#include "AnalysisManager.hh"

class TrackAct: public G4UserTrackingAction
{
	AnalysisManager *fAnalysisManager;
	
public:
	TrackAct(AnalysisManager *pAnalysisManager):fAnalysisManager(pAnalysisManager){;};
	
	virtual ~TrackAct(){;};
	
	inline void PreUserTrackingAction(const G4Track* pTrack){
		if(fAnalysisManager) fAnalysisManager->PreUserTrackingAction(pTrack);
	};
	
};


#endif