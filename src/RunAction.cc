#include "AnalysisManager.hh"
#include "RunAction.hh"





RunAction::RunAction(AnalysisManager *pAnalysisManager)
{
	fAnalysisManager = pAnalysisManager;
}

RunAction::~RunAction()
{

}

void RunAction::BeginOfRunAction(const G4Run *pRun)
{
	if(fAnalysisManager) fAnalysisManager->BeginOfRun(pRun);
}

void RunAction::EndOfRunAction(const G4Run *pRun)
{
	if(fAnalysisManager) fAnalysisManager->EndOfRun(pRun);
}

