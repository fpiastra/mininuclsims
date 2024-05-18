#include "EventAction.hh"


#include "G4Event.hh"
#include "G4SDManager.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4HCofThisEvent.hh"
#include "G4TrackStatus.hh"
#include "G4StepStatus.hh"
#include "G4VProcess.hh"



EventAction::EventAction(AnalysisManager *pAnalysisManager)
{
	fAnalysisManager = pAnalysisManager;
}

EventAction::~EventAction()
{
}

void EventAction::BeginOfEventAction(const G4Event *pEvent)
{
	if(fAnalysisManager) fAnalysisManager->BeginOfEvent(pEvent);
}

void EventAction::EndOfEventAction(const G4Event *pEvent)
{
	if(fAnalysisManager) fAnalysisManager->EndOfEvent(pEvent);
}


