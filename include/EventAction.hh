#ifndef EVENT_ACTION_HH
#define EVENT_ACTION_HH

#include <G4UserEventAction.hh>

#include "AnalysisManager.hh"

class G4Event;

class EventAction : public G4UserEventAction
{
public:
	EventAction(AnalysisManager *pAnalysisManager = NULL);
	virtual ~EventAction();

public:
	void BeginOfEventAction(const G4Event *pEvent);
	void EndOfEventAction(const G4Event *pEvent);

private:
	AnalysisManager *fAnalysisManager;
};

#endif

