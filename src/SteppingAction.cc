#include "AnalysisManager.hh"
#include "SteppingAction.hh"
#include "DetConstr.hh"

#include <G4ios.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleTypes.hh>
#include <G4Track.hh>
#include <G4Event.hh>
#include <G4VProcess.hh>
#include <G4StackManager.hh>
#include <G4TrackStatus.hh>


#include <G4Step.hh>


SteppingAction::SteppingAction(AnalysisManager *pAnalysisManager)
{
	fAnalysisManager = pAnalysisManager;
}

SteppingAction::~SteppingAction()
{
}

void SteppingAction::UserSteppingAction(const G4Step* theStep) 
{
	if(!theStep) return;
	if(fAnalysisManager){
		fAnalysisManager->Step(theStep, fpSteppingManager);
	}
}









