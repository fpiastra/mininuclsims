#ifndef PARTICLE_SOURCE_HH
#define PARTICLE_SOURCE_HH


#include "globals.hh"
#include "G4VPrimaryGenerator.hh"
#include "G4Navigator.hh"
#include "G4ParticleMomentum.hh"
#include "G4ParticleDefinition.hh"
#include "G4Track.hh"


#include <set>
#include <vector>

using std::set;
using std::vector;


class G4VPhysicalVolume;
class PartSrcMessenger;
class PrimGenAction;


enum class PartSrcVerbosity{kSilent, kInfo, kDetails, kDebug};

class PartSrc: public G4VPrimaryGenerator
{
	PrimGenAction *fPrimGenAct;
	PartSrcMessenger *fMessenger;
	G4Navigator *fNavigator;
	
protected:
	virtual void SetInitialValues();
	
	G4ParticleDefinition *fParticleDefinition;
	
	G4int fPrimNb;
	G4String fPartName;
	G4double fMass, fCharge;
	G4double fTotEnergy, fKinEnergy, fMom;
	G4ParticleMomentum fMomentumDirection;
	G4ThreeVector fPolarization;
	
	G4String fSourcePosType;
	G4String fShape;
	G4ThreeVector fCenterCoords;
	G4double fHalfx; //Used also for the "Rect" shape
	G4double fHalfy; //Used also for the "Rect" shape
	G4double fHalfz;
	G4double fRadius; //This is for Sphere, Cylinder and Disk
	G4ThreeVector fPrimSurfNormal; //This is for the generation on a surface
	G4bool fConfine;
	set<G4String> fVolumeNames;
	G4String fAngDistType;
	G4String fEnergyDisType;
	G4int fMaxConfineLoop;
	
	//Stuff to be used internally for generating the primaries
	G4VPhysicalVolume *fVol;
	G4ThreeVector fPosition;
	G4double fTime;
	
	//Stuff that is given to the outside world from the class interface (e.g. made for the analysis manager)
	vector<const G4PrimaryParticle*> fPrimPartVec;
	vector<G4double> fEnPrim;
	G4VPhysicalVolume *fVolPrim; //This should never ever be deleted inside this class, only reassigned when needed
	G4ThreeVector fPosPrim;
	vector<G4ThreeVector> fPolPrim;
	vector<G4ParticleMomentum> fMomPrim;
	vector<G4String> fPhysVolPrim;
	
	PartSrcVerbosity fVerbosityLevel;
	
	
	
	
public:
	PartSrc(PrimGenAction *primGenAct, G4int nPrim=1, PartSrcVerbosity verb=PartSrcVerbosity::kSilent);
	
	virtual ~PartSrc();
	
	
	//THIS IS THE CENTRAL FUNCTION OF ALL THE CLASS
	void GeneratePrimaryVertex(G4Event *pEvent);
	
	//-------------------------------------//
	//Methods used by the UI (the messenger)
	
	//Set primary particle and their properties
	void SetPrimNb(G4int nprim);
	void SetParticleDef(G4ParticleDefinition* aParticleDefinition);
	inline void SetParticleCharge(G4double charge){fCharge = charge;};
	void SetKinEnergy(G4double KinEnergy);
	void SetMomentum(G4double aMomentum);
	void SetMomentum(G4ParticleMomentum aMomentum);
	inline void SetDirection(G4ThreeVector aDirection){fMomentumDirection = aDirection.unit();};
	
	//Set where and how to generate
	inline void SetMaxConfineLoop(G4int _max){fMaxConfineLoop = _max;};
	inline void SetPosDisType(G4String hSourcePosType) { fSourcePosType = hSourcePosType; }
	inline void SetPosDisShape(G4String hShape) { fShape = hShape; }
	inline void SetCenterCoords(G4ThreeVector hCenterCoords) { fCenterCoords = hCenterCoords; }
	inline void SetSurfNormal(G4ThreeVector hDirVect){fPrimSurfNormal = hDirVect.unit();};

	inline void SetHalfX(G4double dHalfx) { fHalfx = dHalfx; }
	inline void SetHalfY(G4double dHalfy) { fHalfy = dHalfy; }
	inline void SetHalfZ(G4double dHalfz) { fHalfz = dHalfz; }
	inline void SetRadius(G4double dRadius) { fRadius = dRadius; }
	
	inline void SetVerbosity(PartSrcVerbosity iVerbosityLevel) { fVerbosityLevel = iVerbosityLevel; };
	inline void SetAngDistType(G4String hAngDistType) { fAngDistType = hAngDistType; }
	inline void SetPhotonPolar(G4ThreeVector hPol) { fPolarization = hPol.unit(); }
	inline void SetEnergyDisType(G4String type){fEnergyDisType=type;};
	void ConfineSourceToVolume(G4String);
	
	//Help message methods
	void PrintParticle();
	void PrintDirection();
	void PrintPolar();
	
	
	//--------------//
	//Getter methods
	inline G4int GetPrimNb() const {return fPrimNb;};
	
	inline const G4VPhysicalVolume* GetPrimVol(){return fVolPrim;};
	inline const G4ThreeVector& GetPrimPos() const {return fPosPrim;};
	inline const vector<G4ParticleMomentum>& GetPrimMom() const {return fMomPrim;};
	inline const vector<G4ThreeVector>& GetPrimPol() const {return fPolPrim;};
	inline const vector<const G4PrimaryParticle*>* GetPrimPartVec() const {return &fPrimPartVec;};
	inline const G4String& GetParticleType() const { return fParticleDefinition->GetParticleName(); };
	inline G4double GetTotalEnergy() const { return fTotEnergy; }
	inline G4double GetKinEnergy() const { return fKinEnergy; }
	inline G4double GetMomentum() const { return fMom; }
	inline const G4ThreeVector& GetDirection() const { return fMomentumDirection; }
	inline const G4ThreeVector& GetPosition() const { return fPosition; }
	inline const G4ThreeVector& GetPolarization() const { return fPolarization; }
	inline const G4String& GetPosDisType() const { return fSourcePosType; };
	inline const G4String& GetAngDistrType() const {return fAngDistType;};
	
	
private:
	//--------------//
	//Functions effectively used by the main function to generate the primaries (the setters must have been used before!)
	void GeneratePointSource();
	void GeneratePointsInVolume();
	void GeneratePointsOnSurface();
	G4bool IsSourceConfined();
	void GenerateIsotropic();
	
};


#endif

