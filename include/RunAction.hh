#ifndef RUN_ACTION_HH
#define RUN_ACTION_HH

#include <G4UserRunAction.hh>

class G4Run;

class AnalysisManager;

class RunAction: public G4UserRunAction
{
public:
	RunAction(AnalysisManager *pAnalysisManager=NULL);
	~RunAction();

public:
	void BeginOfRunAction(const G4Run *pRun);
	void EndOfRunAction(const G4Run *pRun);
	
	

private:
	AnalysisManager *fAnalysisManager;
};

#endif

