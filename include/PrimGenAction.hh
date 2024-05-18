#ifndef PRIM_GEN_ACTION_HH
#define PRIM_GEN_ACTION_HH

#include "ParticleSource.hh"

#include "globals.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ThreeVector.hh"



class G4Event;

class PrimGenAction: public G4VUserPrimaryGeneratorAction
{
	long fSeeds[2];
	G4int fPrimNb;
	G4String fParticleTypeOfPrimary;
	G4double fEnergyOfPrimary;
	G4ThreeVector fPositionOfPrimary;

	PartSrc *fPartSrc;
	
	
	//Do not allow to have the assign operator and the copy constructor
	PrimGenAction & operator=(const PrimGenAction &right);
	PrimGenAction(const PrimGenAction&);
	
public:
	PrimGenAction();
	virtual ~PrimGenAction();
	
	
	const PartSrc* GetParticleSource() const {return fPartSrc;};
	
	const long *GetEventSeeds() const { return fSeeds; };
	const G4String &GetParticleTypeOfPrimary() const { return fParticleTypeOfPrimary; };
	G4double GetEnergyOfPrimary() const { return fEnergyOfPrimary; };
	G4ThreeVector GetPositionOfPrimary() const { return fPositionOfPrimary; };
	
	
	//This is not a simple getter
	G4int GetPrimNb();
	
	//This controls the Particle source settings to match the fPrimNb
	void SetPrimaries();
	
	//This should be used only by the Particle source (otherwise it is very unsafe)
	void SetPrimNb(G4int primNb){fPrimNb=primNb;};
	
	
	
	const G4ThreeVector& GetPrimPos()const{return fPartSrc->GetPrimPos();};
	const vector<G4ParticleMomentum>& GetPrimMom()const{return fPartSrc->GetPrimMom();};
	const vector<G4ThreeVector>& GetPrimPol()const{return fPartSrc->GetPrimPol();};
	
	
	
	virtual void GeneratePrimaries(G4Event *pEvent);

private:
	
};

#endif
