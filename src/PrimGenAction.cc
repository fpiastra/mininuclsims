#include "G4RunManagerKernel.hh"
#include "G4Event.hh"
#include "Randomize.hh"

#include "PrimGenAction.hh"

PrimGenAction::PrimGenAction():
fPrimNb(1),
fPartSrc(nullptr)
{
	fPartSrc = new PartSrc(this);
	
	fSeeds[0] = -1;
	fSeeds[1] = -1;
}

PrimGenAction::~PrimGenAction()
{
	if(fPartSrc) delete fPartSrc;
}

void PrimGenAction::SetPrimaries(){
	if(fPartSrc){
		fPrimNb = fPartSrc->GetPrimNb();
	}
}

G4int PrimGenAction::GetPrimNb(){
	SetPrimaries();
	return fPrimNb;
}

void PrimGenAction::GeneratePrimaries(G4Event *pEvent)
{
	fSeeds[0] = *(CLHEP::HepRandom::getTheSeeds());
	fSeeds[1] = *(CLHEP::HepRandom::getTheSeeds()+1);
	
	/*
	G4cout << "------------------------------------------------------" << G4endl;
	G4cout << "\nXurich2PrimaryGeneratorActionOptPh::GeneratePrimaries:" << G4endl;
	fPartSrc->PrintParticle();
	fPartSrc->PrintDirection();
	fPartSrc->PrintPolar();
	G4cout << "\nNumber of primaries per event: " << fPartSrc->GetPrimNb() << G4endl;
	*/
	
	fPartSrc->GeneratePrimaryVertex(pEvent);
}

