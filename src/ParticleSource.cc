#include "ParticleSource.hh"
#include "ParticleSourceMessenger.hh"
#include "PrimGenAction.hh"

#include <G4PrimaryParticle.hh>
#include <G4Event.hh>
#include <G4TransportationManager.hh>
#include <G4VPhysicalVolume.hh>
#include <G4PhysicalVolumeStore.hh>
#include <G4ParticleTable.hh>
#include <G4ParticleDefinition.hh>
#include <G4IonTable.hh>
#include <G4Ions.hh>
#include <G4TrackingManager.hh>
#include <G4Track.hh>
#include <Randomize.hh>
#include "G4SystemOfUnits.hh"

#include <iostream>
#include <sstream>
#include <cmath>

using std::stringstream;



PartSrc::PartSrc(PrimGenAction *primGenAct, G4int nPrim, PartSrcVerbosity verb):
fPrimGenAct(primGenAct),
fMessenger(nullptr),
fNavigator(nullptr),
fParticleDefinition(nullptr),
fPrimNb(nPrim),
fPartName(""),
fMass(0.),
fCharge(0.),
fTotEnergy(0.),
fKinEnergy(0.),
fMom(0.),
fSourcePosType(G4String("Point")),
fShape("NULL"),
fCenterCoords(G4ThreeVector(0.,0.,0.)),
fHalfx(0.),
fHalfy(0.),
fHalfz(0.),
fRadius(0.),
fConfine(false),
fAngDistType("iso"),
fEnergyDisType(G4String("Mono")),
fMaxConfineLoop(100000),
fVol(nullptr),
fTime(0.),
fVolPrim(nullptr),
fVerbosityLevel(verb)
{
	
	fMessenger = new PartSrcMessenger(this);
	
	
	fNavigator = G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();
	
	
	SetInitialValues();
	
	//Set the "geantino" as a default particle 
	fParticleDefinition = G4ParticleTable::GetParticleTable()->FindParticle("geantino");
	
	if(!fParticleDefinition){
		G4Exception("PartSrc::PartSrc","PartSrc000", FatalException,"Cannot set the geantino as default particle! Check your physics list.");
	}
	
	
	
	if(fVerbosityLevel>=PartSrcVerbosity::kDetails)
	G4cout << "Detail --> Particle source initialized." << G4endl;
}


PartSrc::~PartSrc()
{
	if(fMessenger) delete fMessenger;
}


void PartSrc::SetInitialValues()
{
	if(fParticleDefinition){
		fPartName = fParticleDefinition->GetParticleName();
		fCharge = fParticleDefinition->GetPDGCharge();
		fMass = fParticleDefinition->GetPDGMass();
	}
	
	
	fMomentumDirection = G4ParticleMomentum(0., 0., 1.);
	fTotEnergy = fMass + fKinEnergy;
	fMom = std::sqrt( fTotEnergy*fTotEnergy - fMass*fMass );
	
	
	fPosition = G4ThreeVector(0.,0.,0.);
	fPolarization=G4ThreeVector(1.,0.,0.);
	fCenterCoords=G4ThreeVector(0.,0.,0.);
	
	fVolumeNames.clear();
	
	fPrimPartVec.resize(fPrimNb);
	fEnPrim.resize(fPrimNb,fTotEnergy);
	fPosPrim = G4ThreeVector(0.,0.,0.);
	fMomPrim.resize(fPrimNb);
	fPolPrim.resize(fPrimNb);
	
	fPhysVolPrim.resize(fPrimNb);
}


//THIS IS THE CENTRAL FUNCTION OF THIS CLASS
void PartSrc::GeneratePrimaryVertex(G4Event* pEvt)
{
	if(!fParticleDefinition){
		G4cerr << "\nERROR --> PartSrc::GeneratePrimaryVertex: No particle is defined (null pointer)!" << G4endl;
		G4Exception("PartSrc::GeneratePrimaryVertex","PartSrc001",FatalException,"The \"fParticleDefinition\" is null!");
	}
	
	if(!pEvt){
		G4cerr << "\nERROR --> PartSrc::GeneratePrimaryVertex: No particle is defined (null pointer)!" << G4endl;
		G4Exception("PartSrc::GeneratePrimaryVertex","PartSrc002",FatalException,"The \"pEvt\" is null!");
	}
	
	
	// Position
	G4bool srcconf = false;
	
	G4int LoopCount;
	
	if(fSourcePosType == G4String("Point")){
		GeneratePointSource();
	}else if(fSourcePosType == G4String("Volume")){
		
		if(fConfine == true){
			
			LoopCount = 0;
			do{
				GeneratePointsInVolume();
				LoopCount++;
				
				if(LoopCount >= fMaxConfineLoop){
					G4cerr << "\nERROR --> PartSrc::GeneratePrimaryVertex:" << G4endl;
					G4cerr << "*********************************************" << G4endl;
					G4cerr << "LoopCount = " << fMaxConfineLoop << G4endl;
					G4cerr << "Either the source distribution >> confinement" << G4endl;
					G4cerr << "or any confining physical volume may not" << G4endl;
					G4cerr << "overlap with the source distribution or none" << G4endl;
					G4cerr << "of the confining volumes may not exist!" << G4endl;
					G4cerr << "If you have set confine then this will be" << G4endl;
					G4cerr << "ignored for this event." << G4endl;
					G4cerr << "*********************************************" << G4endl;
					break; //Avoids an infinite loop
				}
				
			}while( (!fVol) || (!IsSourceConfined()) );
			
		}else{
			LoopCount = 0;
			do{
				GeneratePointsInVolume();
				LoopCount++;
				
				if(LoopCount >= fMaxConfineLoop){
					G4cerr << "\nERROR --> PartSrc::GeneratePrimaryVertex:" << G4endl;
					G4cerr << "***************************************************" << G4endl;
					G4cerr << "LoopCount = " << fMaxConfineLoop << G4endl;
					G4cerr << "The source has been generated too many times"  << G4endl;
					G4cerr << "outside the world: the pointer to the physical" << G4endl;
					G4cerr << "volume is null! Setting the source point at the" << G4endl;
					G4cerr << "origin of the coordinates to avoid the application" << G4endl;
					G4cerr << "to crash. Not affidable simulation results!"  << G4endl;
					G4cerr << "***************************************************" << G4endl;
					fPosition = G4ThreeVector(0.,0.,0.);
					fVol = fNavigator->LocateGlobalPointAndSetup(fPosition, nullptr, true);
					break; //Avoids an infinite loop
				}
			}while(!fVol);
		}
	}else if(fSourcePosType == G4String("Surface")){
		LoopCount = 0;
		do{
			GeneratePointsOnSurface();
			
			LoopCount++;
			
			if(LoopCount >= fMaxConfineLoop){
				G4cerr << "\nERROR --> PartSrc::GeneratePrimaryVertex:" << G4endl;
				G4cerr << "**************************************************" << G4endl;
				G4cerr << "LoopCount = " << fMaxConfineLoop << G4endl;
				G4cerr << "The source has been generated too many times"  << G4endl;
				G4cerr << "outside the world: the pointer to the physical" << G4endl;
				G4cerr << "volume is null! Setting the source point at the" << G4endl;
				G4cerr << "origin of the coordinates to avoid the application" << G4endl;
				G4cerr << "to crash. Not affidable simulation results!"  << G4endl;
				G4cerr << "**************************************************" << G4endl;
				fPosition = G4ThreeVector(0.,0.,0.);
				fVol = fNavigator->LocateGlobalPointAndSetup(fPosition, nullptr, true);
				break; //Avoids an infinite loop
			}
		}while(!fVol);
	}
	
	
	if(!fVol){
		G4cerr << "\nERROR --> PartSrc::GeneratePrimaryVertex: No volume determined for the generated point!" << G4endl;
		G4Exception("PartSrc::GeneratePrimaryVertex","PartSrc003",FatalException,"The \"fVol\" is null!");
	}
	
	fPosPrim = fPosition;
	fVolPrim = fVol;
	
	// Angular stuff
	if(fAngDistType == G4String("direction")){
		SetDirection(fMomentumDirection);
	}else{
		if(fAngDistType != G4String("iso")){
			G4cerr << "\nERROR --> PartSrc::GeneratePrimaryVertex: AngDistType has unusual value: \"" << fAngDistType << "\"" << G4endl;
			return;
		}
	}
	
	// Energy stuff
	if(fEnergyDisType != G4String("Mono")){
		G4cerr << "\nERROR --> PartSrc::GeneratePrimaryVertex: EnergyDisType has unusual value: \"" << fEnergyDisType << "\"" << G4endl;
		return;
	}
	
	
	
	
	
	if(fVerbosityLevel >= PartSrcVerbosity::kDetails){
		G4cout << "\nDetail --> PartSrc::GeneratePrimaryVertex: " << G4endl;
		G4cout << "    Number of prymaries per event: " << fPrimNb << G4endl;
		G4cout << "                    Particle name: " << fParticleDefinition->GetParticleName() << G4endl;
		G4cout << "                             Mass: " << fMass/MeV << " MeV" << G4endl;
		G4cout << "                           Energy: " << fTotEnergy/keV << " keV" << G4endl;
		G4cout << "                             Time: " << fTime << G4endl;
		if(fAngDistType == G4String("direction")){
			G4cout << "                        Direction: " << fMomentumDirection << G4endl;
			G4cout << "                     Polarization: " << fPolarization << G4endl;
		}
		G4cout << "                         Position: " << fPosPrim << G4endl;
		G4cout << "                      Volume name: " << fVolPrim->GetName() << G4endl;
		
	}
	
	
	
	G4PrimaryVertex *vertex = new G4PrimaryVertex(fPosition, fTime);
	
#ifndef NDEBUG
	if(fVerbosityLevel >= PartSrcVerbosity::kDebug) G4cout << "\nDebug --> PartSrc::GeneratePrimaryVertex: vertex created." << G4endl;
#endif
	
	G4String PhysVolName = fVolPrim->GetName();
	
	// create a new vertex
#ifndef NDEBUG
	if(fVerbosityLevel >= PartSrcVerbosity::kDebug)
		G4cout << "\nDebug --> PartSrc::GeneratePrimaryVertex: Creating primaries and assigning to vertex." << G4endl;
#endif
	
	if((fVerbosityLevel >= PartSrcVerbosity::kDetails) && (fAngDistType == G4String("iso"))){
		G4cout << "\nDetail --> PartSrc::GeneratePrimaryVertex: Generating isotropic directions for "<< fPrimNb << " primaries:" << G4endl;
	}
	for(G4int iPart = 0; iPart < fPrimNb; iPart++)
	{
		if(fAngDistType == G4String("iso")){
			GenerateIsotropic();
			if(fVerbosityLevel >= PartSrcVerbosity::kDetails){
				G4cout << "          Direction ("<<iPart<<"): " << fMomentumDirection.unit() << G4endl;
				G4cout << "       Polarization ("<<iPart<<"): " << fPolarization << G4endl;
			}
		}
		
		G4PrimaryParticle *particle = new G4PrimaryParticle(fParticleDefinition);
		particle->SetMass(fMass);
		particle->SetMomentumDirection(fMomentumDirection.unit());
		particle->SetTotalEnergy(fTotEnergy);
		particle->SetCharge(fCharge);
		particle->SetPolarization(fPolarization.unit());
		vertex->SetPrimary(particle);
		
		fPrimPartVec.at(iPart) = particle;
		
		fEnPrim.at(iPart) = fTotEnergy;
		fMomPrim.at(iPart) = particle->GetMomentum();
		fPolPrim.at(iPart) = particle->GetPolarization();
	}
	pEvt->AddPrimaryVertex(vertex);
	
	if(fVerbosityLevel >= PartSrcVerbosity::kDetails) G4cout << "\nDetail --> PartSrc::GeneratePrimaryVertex: Primary Vertex generated with " << fMomPrim.size() << " particles." << G4endl;
	
}


void PartSrc::SetPrimNb(G4int nprim)
{
	fPrimNb = nprim;
	
	if(fPrimGenAct) fPrimGenAct->SetPrimNb(fPrimNb);
	
	fEnPrim.resize(fPrimNb,fTotEnergy);
	fMomPrim.resize(fPrimNb);
	fPolPrim.resize(fPrimNb);
	fPhysVolPrim.resize(fPrimNb);
	
	return;
}


void PartSrc::SetParticleDef(G4ParticleDefinition *aParticleDefinition)
{
	if(!aParticleDefinition){
		G4Exception("PartSrc::SetParticleDef(...)","PartSrc004",FatalException,"The \"aParticleDefinition\" pointer is null!");
	}
	fParticleDefinition = aParticleDefinition;
	fCharge = fParticleDefinition->GetPDGCharge();
	fMass = fParticleDefinition->GetPDGMass();
	fPartName = fParticleDefinition->GetParticleName();
}


void PartSrc::SetKinEnergy(G4double KinEnergy)
{
	fKinEnergy = KinEnergy;
	if(!fParticleDefinition){
		//Assuming zero mass
		fMass = 0;
		fTotEnergy = fKinEnergy;
		fMom = fTotEnergy;
		return;
	}
	fTotEnergy = fMass + fKinEnergy;
	fMom = std::sqrt( fTotEnergy*fTotEnergy - fMass*fMass );
}


void PartSrc::SetMomentum(G4double aMomentum)
{
	fMom = aMomentum;
	if(!fParticleDefinition){
		//Assuming zero mass
		fMass = 0;
		fTotEnergy = fMom;
		fKinEnergy = fTotEnergy;
		return;
	}
	
	fTotEnergy = std::sqrt( fMom*fMom + fMass*fMass );
	fKinEnergy = fTotEnergy - fMass;
}


void PartSrc::SetMomentum(G4ParticleMomentum aMomentum)
{
	fMomentumDirection = aMomentum.unit();
	SetMomentum(aMomentum.mag());
}


void PartSrc::ConfineSourceToVolume(G4String hVolumeList)
{
	stringstream hStream;
	hStream.str(hVolumeList);
	G4String hVolumeName;
	
	// store all the volume names
	while(!hStream.eof())
	{
		hStream >> hVolumeName;
		fVolumeNames.insert(hVolumeName);
	}

	// checks if the selected volumes exist and store all volumes that match
	G4PhysicalVolumeStore *PVStore = G4PhysicalVolumeStore::GetInstance();
	G4bool bFoundAll = true;

	set<G4String> hActualVolumeNames;
	for(set<G4String>::iterator pIt = fVolumeNames.begin(); pIt != fVolumeNames.end(); pIt++){
		G4String hRequiredVolumeName = *pIt;
		//G4bool bMatch = false;
		
		G4bool bMatch = (hRequiredVolumeName.last('*') != std::string::npos);

		if(bMatch) hRequiredVolumeName = hRequiredVolumeName.strip(G4String::trailing, '*');

		G4bool bFoundOne = false;
		for(G4int iIndex = 0; iIndex < (G4int) PVStore->size(); iIndex++)
		{
			G4String hName = (*PVStore)[iIndex]->GetName();

			if((bMatch && (hName.substr(0, hRequiredVolumeName.size())) == hRequiredVolumeName) || hName == hRequiredVolumeName)
			{
				hActualVolumeNames.insert(hName);
				bFoundOne = true;
			}
		}

		bFoundAll = bFoundAll && bFoundOne;
	}

	if(bFoundAll)
	{
		fVolumeNames = hActualVolumeNames;
		fConfine = true;

		if(fVerbosityLevel >= PartSrcVerbosity::kInfo)
			G4cout << "Info --> Source confined to volumes: " << hVolumeList << G4endl;

		if(fVerbosityLevel >= PartSrcVerbosity::kDetails)
		{
			G4cout << "Detail --> Volume list: " << G4endl;

			for(set<G4String>::iterator pIt = fVolumeNames.begin(); pIt != fVolumeNames.end(); pIt++)
				G4cout << *pIt << G4endl;
		}
	}
	else if(fVolumeNames.empty())
		fConfine = false;
	else
	{
		G4cerr << "ERROR --> PartSrc::ConfineSourceToVolume(...): One or more volumes do not exist! Ignoring confine condition." << G4endl;
		fVolumeNames.clear();
		fConfine = false;
	}
}


void PartSrc::GeneratePointSource()
{
	// Generates Points given the point source.
	if(fSourcePosType == "Point"){
		fPosition = fCenterCoords;
		G4ThreeVector nullvect(0., 0., 0.);
		G4ThreeVector *ptr = &nullvect;
		fVol = fNavigator->LocateGlobalPointAndSetup(fPosition, ptr, true);
	}else{
		G4cerr << "ERROR --> PartSrc::GeneratePointSource(): SourcePosType is not set to \"Point\"" << G4endl;
	}
}


void PartSrc::GeneratePointsInVolume()
{
#ifndef NDEBUG
	if(fVerbosityLevel>=PartSrcVerbosity::kDebug) G4cout << "\nDebug --> PartSrc::GeneratePointsInVolume: Start of the method." << G4endl;
#endif
	
	G4double x = 0., y = 0., z = 0.;
	
	G4double r = 0.;
	G4double phi = 0.;
	G4double cosTheta, sinTheta = 0.;

	if( fSourcePosType != G4String("Volume") )
		G4cerr << "\nWARNING --> PartSrc::GeneratePointsInVolume: SourcePosType is not \"Volume\"!" << G4endl;

	if(fShape == G4String("Sphere"))
	{
		phi = CLHEP::twopi * G4UniformRand();
		cosTheta = 2*(G4UniformRand()-0.5);
		sinTheta = std::sqrt(1.-cosTheta*cosTheta);
		r = std::cbrt(G4UniformRand()) * fRadius;
		
		x = r * std::cos(phi) * sinTheta;
		y = r * std::sin(phi) * sinTheta;
		z = r * cosTheta;
		
	}
	else if(fShape == G4String("Cylinder"))
	{
		phi = CLHEP::twopi * G4UniformRand();
		r = std::sqrt( G4UniformRand() )*fRadius;
		
		x = r*std::cos(phi);
		y = r*std::sin(phi);
		z = (2*G4UniformRand() - 1)*fHalfz;
	}
	else if(fShape == G4String("Box"))
	{
		x = 2*(G4UniformRand()-0.5)*fHalfx;
		y = 2*(G4UniformRand()-0.5)*fHalfy;
		z = 2*(G4UniformRand()-0.5)*fHalfz;
	}
	else
	{
		G4cerr << "\nWARNING --> PartSrc::GeneratePointsInVolume(): Volume shape \"" << fShape << "\" does not exist or is not allowed for this point generator method!" << G4endl;
	}
	
	
	fPosition = fCenterCoords + G4ThreeVector(x,y,z);
	
	G4ThreeVector nullvect(0., 0., 0.);
	fVol = fNavigator->LocateGlobalPointAndSetup(fPosition, nullptr, true);
#ifndef NDEBUG
	if(fVerbosityLevel>=PartSrcVerbosity::kDebug) G4cout << "\nDebug --> PartSrc::GeneratePointsInVolume: End of the method." << G4endl;
#endif
}


G4bool PartSrc::IsSourceConfined()
{
	//This method is private and should be used only after the GeneratePointsInVolume method!
#ifndef NDEBUG
	if(fVerbosityLevel>=PartSrcVerbosity::kDebug) G4cout << "\nDebug --> PartSrc::IsSourceConfined: Start of the method." << G4endl;
#endif
	
	// Method to check point is within the volume specified
	if(fConfine == false){
		G4cerr << "\nERROR --> PartSrc::IsSourceConfined: \"fConfine\" is false!" << G4endl;
		return true;
	}
	
	// Check fParticlePosition is within a volume in our list
	G4String theVolName("");
	if(fVol) theVolName = fVol->GetName();
	
	set<G4String>::iterator pIt;
	if((pIt = fVolumeNames.find(theVolName)) != fVolumeNames.end())
	{
		if(fVerbosityLevel >= PartSrcVerbosity::kDetails)
			G4cout << "Detail --> PartSrc::IsSourceConfined: Particle is in volume " << *pIt << G4endl;
		return true;
	}else{
		return false;
	}
#ifndef NDEBUG
	if(fVerbosityLevel>=PartSrcVerbosity::kDebug) G4cout << "\nDebug --> PartSrc::IsSourceConfined: End of the method." << G4endl;
#endif
}


void PartSrc::GeneratePointsOnSurface()
{
	if( fSourcePosType != G4String("Surface") ){
		G4cerr << "\nWARNING --> PartSrc::GeneratePointsOnSurface(): SourcePosType is not \"Surface\"!" << G4endl;
		return;
	}
	
	G4bool vertical = false;
	
	if( (fPrimSurfNormal.getX()==0.) && (fPrimSurfNormal.getY()==0.) ){
		vertical = true;
	}
	
	
	G4double theta = fPrimSurfNormal.theta();
	
	G4double phi = fPrimSurfNormal.phi();
	
	
	
	G4double x = 0., y = 0.;
	
	if(fShape == G4String("Disk")){
#ifndef NDEBUG
		if(fVerbosityLevel >= PartSrcVerbosity::kDebug){
			G4cout << "\nDebug --> PartSrc::GeneratePointsOnSurface: Disk radius " << fRadius << " mm" << G4endl;
		}
#endif
		G4double alpha = CLHEP::twopi * G4UniformRand();
		G4double r = std::sqrt( G4UniformRand() )*fRadius;
		
		x = r*std::cos(alpha);
		y = r*std::sin(alpha);
		
	}else if(fShape == G4String("Rect")){
#ifndef NDEBUG
		if(fVerbosityLevel >= PartSrcVerbosity::kDebug){
			G4cout << "\nDebug --> PartSrc::GeneratePointsOnSurface: Rectangle dimensions: (" << fHalfx << ", " <<  fHalfy << ") mm" << G4endl;
		}
#endif
		x = 2*(G4UniformRand()-0.5)*fHalfx;
		y = 2*(G4UniformRand()-0.5)*fHalfy;
		
	}else{
		G4cerr << "\nWARNING --> PartSrc::GeneratePointsOnSurface(): Surface shape \"" << fShape << "\" does not exist or is not allowed for this point generator!" << G4endl;
		return;
	}
	
	
	G4ThreeVector pos(x, y, 0);
	
#ifndef NDEBUG
	if(fVerbosityLevel >= PartSrcVerbosity::kDebug){
		G4cout << "Debug --> PartSrc::GeneratePointsOnSurface: Relative position of the primary vertex respect to <" << fShape << "> center (before rotation): " << pos << G4endl;
	}
#endif
	
	if(!vertical){
#ifndef NDEBUG
		if(fVerbosityLevel >= PartSrcVerbosity::kDebug){
			G4cout << "Debug --> PartSrc::GeneratePointsOnSurface: Rotation Theta: " << -theta << " rads; Rotation Phi: " << phi << " rads" << G4endl;
		}
#endif
		pos = pos.rotateY(theta);
		pos = pos.rotateZ(phi);
	}
	
#ifndef NDEBUG
	if(fVerbosityLevel >= PartSrcVerbosity::kDebug){
		G4cout << "Debug --> PartSrc::GeneratePointsOnSurface: Relative position of the primary vertex respect to <" << fShape << "> center (after rotation): " << pos << G4endl;
	}
#endif
	fPosition = pos + fCenterCoords;
	
	if(fVerbosityLevel >= PartSrcVerbosity::kDetails){
		G4cout << "\nDetail --> PartSrc::GeneratePointsOnSurface: Vertex generated at " << fPosition << " mm" << G4endl;
	}
	
	fVol = fNavigator->LocateGlobalPointAndSetup(fPosition, nullptr, true);
}


void PartSrc::GenerateIsotropic()
{
	G4double px, py, pz, polx, poly, polz;

	G4double sintheta, sinphi, costheta, cosphi;

	costheta = 1 - 2*G4UniformRand();
	sintheta = std::sqrt(1. - std::pow(costheta,2));

	G4double phi = CLHEP::twopi * G4UniformRand();
	sinphi = std::sin(phi);
	cosphi = std::cos(phi);

	px = sintheta * cosphi;
	py = sintheta * sinphi;
	pz = costheta;

	fMomentumDirection= G4ThreeVector(px,py,pz).unit();
	
	//Check if the particle integer spin or not
	
	if( (fPartName=="gamma") || (fPartName=="opticalphoton") ){
		
		//Generate the random polarization as a 2pi around the momentum direction
		G4double alpha = CLHEP::twopi * G4UniformRand();
	
		G4double sinalpha = std::sin(alpha);
		G4double cosalpha = std::cos(alpha);
	
		//This is always orthogonal to the momentum direction (try to make the scalar product)
		polx = cosalpha*costheta*cosphi - sinalpha*sinphi;
		poly = cosalpha*costheta*sinphi + sinalpha*cosphi;
		polz = -cosalpha*sintheta;
	
		fPolarization = G4ThreeVector(polx,poly,polz).unit();
	}else{
		fPolarization = G4ThreeVector(0.,0.,0.);
	}
	
	// m_hParticleMomentumDirection now holds unit momentum vector.
#ifndef NDEBUG
	if(fVerbosityLevel >= PartSrcVerbosity::kDebug) G4cout << "\nDebug --> PartSrc::GenerateIsotropic: Generated isotropic vector: " << fMomentumDirection << G4endl;
#endif
}


void PartSrc::PrintParticle(){ 
	G4cout << "\nSelected particle: " << fParticleDefinition->GetParticleName() << G4endl;
	G4cout << "PDG encoding: " << fParticleDefinition->GetPDGEncoding() << G4endl;
	//std::cout << "\nSelected particle: " << fParticleDefinition->GetParticleName() << std::endl;
}


void PartSrc::PrintDirection()
{
	if(GetAngDistrType()!=G4String("iso")){
		G4cout << "\nParticle direction: " << fMomentumDirection.unit() << G4endl;
	}else{
		G4cout << "\nParticle direction: isotropic" << G4endl;
	}
}


void PartSrc::PrintPolar()
{
	if(GetAngDistrType()!=G4String("iso")){
		G4cout << "\nParticle polarization: " << fPolarization.unit() << G4endl;
	}else{
		G4cout << "\nParticle polarization: random" << G4endl;
	}
}


