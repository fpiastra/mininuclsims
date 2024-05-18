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
#include "G4SystemOfUnits.hh"
#include "G4String.hh"
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4UIterminal.hh"
#include "G4UItcsh.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"

using std::string;
using std::stringstream;
using std::ifstream;

void usage();

int main(int argc, char **argv)
{
	// switches
	int c = 0;
	
	bool bMassInfo = false;
	bool bVolumeName = false;
	bool bGdmlFile = false;
	
	G4String hCommand, hVolName, hGdmlFileName;
	
	// parse switches
	while((c = getopt(argc,argv,"v:g:m")) != -1){
		switch(c){
			case 'm':
				bMassInfo = true;
				break;
			
			case 'v':
				bVolumeName = true;
				hVolName = optarg;
				break;
			
			case 'g':
				{
					hGdmlFileName = optarg;
					ifstream gdmlfile(hGdmlFileName.c_str());
					if(gdmlfile){
						bGdmlFile = true;
					}else{
						G4cout << "\nERROR: cannot find/open the geometry file <" << hGdmlFileName.c_str() << ">" << G4endl;
						exit(1);
					}
					break;
				}
			default:
				usage();
		}
	}
	
	
	if(!bGdmlFile){
		G4cout << "ERROR::: Gdml file of geometry not set!!!\n" << G4endl;
		usage();
	}
	
	if(!bVolumeName){
		G4cout << "ERROR::: Physical volume name not set!!!\n" << G4endl;
		usage();
	}
	
	
	// Detector Construction (here doesn't play any role)
	DetConstr *detector = new DetConstr(hGdmlFileName); 
	detector->Construct();
	
	// geometry IO
	//G4UImanager* pUImanager = G4UImanager::GetUIpointer();
	
	
	
	G4PhysicalVolumeStore* pPhysVolStore = G4PhysicalVolumeStore::GetInstance();
	
	G4int Nvolumes = pPhysVolStore->size();
	
	//Take the physical volume pointer you want
	std::vector<G4VPhysicalVolume *> vPhysVols;
	for(G4int i=0; i<Nvolumes; i++){
		G4String tmp_name = pPhysVolStore->at(i)->GetName();
		if(tmp_name==hVolName){
			vPhysVols.push_back(pPhysVolStore->at(i));
		}
	}
	
	if(vPhysVols.size()==0){
		G4cout << "Physical Volume \"" << hVolName << "\" not found!!!" << G4endl;
		return(0);
	}else{
		G4cout << "There are " << vPhysVols.size() << " instances of the physical volume \"" << hVolName << "\"" << G4endl;
	}
	
	G4cout << G4endl << G4endl;
	
	if(bMassInfo){
		G4double mass = (vPhysVols.at(0)->GetLogicalVolume()->GetMass(false,false))/kg;
		G4double density = (vPhysVols.at(0)->GetLogicalVolume()->GetMaterial()->GetDensity())/(kg/m3);
		G4double volume = mass/density;
		G4cout << "Mass of physical volume \"" << vPhysVols.at(0)->GetName() << "\" = " << mass << " kg" << G4endl;
		G4cout << "Volume of physical volume \"" << vPhysVols.at(0)->GetName() << "\" = " << volume << " m^3" << G4endl;
		G4cout << G4endl << G4endl;
	}
	
	for(G4int iVol=0; iVol<vPhysVols.size(); iVol++){
		G4cout << "\n\nInstance " << iVol << ":" << G4endl;
		
		G4VPhysicalVolume *pPhysVol = vPhysVols.at(iVol);
		
		G4ThreeVector ShiftGlob = pPhysVol->GetTranslation();
		
		while( pPhysVol->GetMotherLogical() ){
			//Find the mother phys volume
			G4VPhysicalVolume *pAncPhysVol;
			
			for(G4int i=0; i<Nvolumes; i++){
				if(pPhysVolStore->at(i)->GetLogicalVolume()->IsDaughter(pPhysVol)){
					pAncPhysVol = pPhysVolStore->at(i);
				}
			}
			G4cout << "Shift of physical volume \"" << pPhysVol->GetName() << "\" with respect to \"" << pAncPhysVol->GetName() << "\" =  " << pPhysVol->GetTranslation()/mm << " mm" << G4endl;
			pPhysVol = pAncPhysVol;
			ShiftGlob = ShiftGlob + pPhysVol->GetTranslation();
		}
		
		G4cout << "\nGlobal shift of physical volume \"" << hVolName << "\" (inst. " << iVol << ") =  " << ShiftGlob/mm << " mm" << G4endl;
		
	}
	
	
	return 0;
}

void usage(){
	G4cout << "\nUsage:" << G4endl;
	G4cout << "GeomInfo <-g gdmlfile> <-v physvolname> [-m] " << G4endl;
	G4cout << "\n\nOptions:" << G4endl;
	G4cout << "-g gdmlfile          Set the gdml file with the geometry description of the detector (mandatory)\n" << G4endl;
	G4cout << "-v physvolname       Name of the physical volume to search geometry info for (mandagtory)\n" << G4endl;
	G4cout << "-m                   Prints out the m ass of the specific volume (optional)\n" << G4endl;
	exit(0);
}

