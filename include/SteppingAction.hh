#ifndef STEPPING_ACTION_HH
#define STEPPING_ACTION_HH

#include <globals.hh>
#include <G4UserSteppingAction.hh>


class AnalysisManager;

class SteppingAction: public G4UserSteppingAction
{
public:
	SteppingAction(AnalysisManager *pAnalysisManager=0);
	virtual ~SteppingAction();
	
	void UserSteppingAction(const G4Step* aStep);

private:
	AnalysisManager *fAnalysisManager;
};

#endif

