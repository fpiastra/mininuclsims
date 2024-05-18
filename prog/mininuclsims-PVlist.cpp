//#include "PhysList.hh"
#include "DetConstr.hh"
//#include "AnalysisManager.hh"
//#include "PrimGenActionOptPh.hh"
//#include "StackingAction.hh"
//#include "RunAction.hh"
//#include "EventAction.hh"
//#include "SteppingAction.hh"

#include <string>
#include <fstream>
#include <sstream>
#include <unistd.h>

/*
#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh"
#else
#include "G4RunManager.hh"
#endif
*/

#include "globals.hh"
#include "G4String.hh"
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4UIterminal.hh"
#include "G4UItcsh.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"


using std::string;
using std::stringstream;
using std::ifstream;

void usage();

int main(int argc, char **argv)
{
	stringstream hStream;
	
	bool bGdmlFile = false;
	G4String hGdmlFileName;
	
	
	if(argc!=3){
		G4cout << "\nERROR: wrong number of arguments (2 required)!" << G4endl;
		usage();
	}
	
	hStream << argv[1];
	
	if(hStream.str()==string("-g")){
		hGdmlFileName = argv[2];
		ifstream gdmlfile(hGdmlFileName.c_str());
		if(gdmlfile){
			bGdmlFile = true;
		}else{
			G4cout << "\nERROR: cannot find/open file <" << hGdmlFileName.c_str() << ">" << G4endl;
		}
	}
	
	
	
	if(!bGdmlFile){
		G4cout << "\nERROR: the gdml file must be provided!" << G4endl;
		usage();
	}
	
	// create the run manager
/*
#ifdef G4MULTITHREADED
	G4RunManager *pRunManager = new G4RunManager;
#else
	G4MTRunManager* runManager = new G4MTRunManager;
	runManager->SetNumberOfThreads(1);
#endif
*/
	
	DetConstr *pDetGeom = new DetConstr(hGdmlFileName);
	
	pDetGeom->PrintListOfPhysVols();
	
	return 0;
}


void usage()
{
	G4cout << "\nUsage:" << G4endl;
	G4cout << "PVlist -g gdmlfile " << G4endl;
	exit(0);
}

