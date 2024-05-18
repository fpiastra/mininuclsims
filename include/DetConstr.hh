#ifndef DET_CONSTR_HH
#define DET_CONSTR_HH

#include "OptPropManager.hh"

#include "globals.hh"
#include "G4VUserDetectorConstruction.hh"

#include "G4OpticalSurface.hh"
#include "G4GDMLParser.hh"
#include "G4Material.hh"

#include <vector>
#include <map>

using std::vector;
using std::map;
using std::string;


class G4String;
class G4Colour;
class G4LogicalVolume;
class G4VPhysicalVolume;
class G4SubtractionSolid;
class DetectorMessenger;
//class OptPropManager;

enum class DetVerbosity{kSilent, kInfo, kDetails, kDebug};


class DetConstr: public G4VUserDetectorConstruction
{
public:
	
	using PVmap = std::map<std::string, std::vector<G4VPhysicalVolume*> >;
	//typedef std::map<std::string, std::vector<G4VPhysicalVolume*> > PVmap;

private:
	DetectorMessenger *fDetectorMessenger;
	
	OptPropManager *fOptPropManager;
	
	G4GDMLParser *fGDMLParser;
	G4VPhysicalVolume *fWorld;
	
	DetVerbosity fVerbose;
	
	PVmap fPVolsMap;
	std::map<std::string, int > fPVolsRecour; //How many time it is found in the tree
	
	const G4SurfacePropertyTable *fOptSurfTab;
		
	G4double fTpbThick;
	
	
public:
	
	DetConstr(G4String gdmlfilename);
	virtual ~DetConstr();
	
	//Mandatory method to define. It is called at the initialisation of the run manager.
	G4VPhysicalVolume* Construct();
	
	
	const G4GDMLParser* GetGdmlParser() const {return fGDMLParser;};
	const G4VPhysicalVolume* GetWorldVolume() const {return fWorld;};
	
	
	inline void SetVerbosity(DetVerbosity verb){
		fVerbose=verb;
		fOptPropManager->SetVerbosity( static_cast<OptPropManager::verbosity>(verb) );
	};

	inline DetVerbosity GetVerbosity(){return fVerbose;};
	
	//As of now the TPB for optical photons simulations is deactivated.
	//These two methods do not have any effect
	inline void SetTpbThickness(G4double thick){fTpbThick = thick;};
	inline G4double GetTpbThickness(){return fTpbThick;}
	
	void SetStepTrackLimit(const G4String& vol, const G4double step_lim);
	
	const std::vector<G4VPhysicalVolume* >* GetPvList(G4String pvname) const;
	
	inline const PVmap* GetVolsMap() const {return (const PVmap*)(&fPVolsMap);}
	
	void PrintVolumeCoordinates(const G4String& VolName);
	void PrintVolumeInfo(const G4String& VolName);
	
	void PrintListOfPhysVols() const;
	void PrintListOfLogVols() const;
	
protected:
	virtual G4Material* FindMaterial(G4String matname);
	
	//These methods are only used at the startup as default (and to debug)
	//They should go away when the full user interface for optical setting
	//Here also all the optical surfaces are defined
	
	//As of now optical photons simulations and are deactivated so this 4 methods are not called anywhere
	virtual void BuildTPBlayer(); 
	virtual void BuildDefaultOptSurf();
	virtual void BuildDefaultLogSurfaces();
	virtual void SetDefaultOptProperties();
	
private:
	
	//Using a std::set in the map in order to avoid having more entries with same volumes pointers (like for a std::vector). This fills the fPVolsMap object.
	void ScanVols(G4VPhysicalVolume* mvol, std::map<G4String, std::set<G4VPhysicalVolume*> > *map=NULL);
	
};





#endif
