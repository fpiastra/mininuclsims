#include "AnalysisMessenger.hh"
#include "AnalysisManager.hh"

#include <set>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>


AnalysisMessenger::AnalysisMessenger(AnalysisManager *pAnManager):
fAnManager(pAnManager)
{ 
	std::stringstream txt; txt.str("");
	
	fAnalysisDir = new G4UIdirectory("/minisims/analysis/");
	fAnalysisDir->SetGuidance("ArgonCube analysis manager settings.");
	
	
	fVerboseCmd = new G4UIcmdWithAnInteger("/minisims/analysis/verbose",this);
	fVerboseCmd->SetGuidance("Set verbosity of the analysis manager");
	txt.str(""); txt << " Default " << static_cast<G4int>(AnalysisVerbosity::kInfo) << ".";
	fVerboseCmd->SetGuidance(txt.str().c_str());
	fVerboseCmd->SetParameterName("Verb", false);
	fVerboseCmd->SetDefaultValue(static_cast<G4int>(AnalysisVerbosity::kInfo));
	txt.str(""); txt << "Verb>=" << static_cast<G4int>(AnalysisVerbosity::kSilent) << " && Verb<=" << static_cast<G4int>(AnalysisVerbosity::kDebug);
	fVerboseCmd->SetRange( txt.str().c_str() );
	fVerboseCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	fPrintModuloCmd = new G4UIcmdWithAnInteger("/minisims/analysis/PrintModulo",this);
	fPrintModuloCmd->SetGuidance("Prints the start of event every \"PrMod\"");
	fPrintModuloCmd->SetGuidance("Usage: /minisims/analysis/PrintModulo <intnum>");
	fPrintModuloCmd->SetParameterName("PrMod", false);
	fPrintModuloCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

	fStepDebugCmd = new G4UIcmdWithABool("/minisims/analysis/stepsDebug", this);
	fStepDebugCmd->SetGuidance("Activates debugging controls and messages at step level for particles arriving to or coming from a volumes boundary (super heavy and super slow). Useful for debugging with optical photons.");
	fStepDebugCmd->SetGuidance("Usage: /minisims/analysis/stepsDebug <true|false>");
	fStepDebugCmd->SetGuidance("       /minisims/analysis/stepsDebug <1|0>");
	fStepDebugCmd->SetParameterName("DebugSteps", false);
	fStepDebugCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	fSaveDataCmd = new G4UIcmdWithAnInteger("/minisims/analysis/SaveData", this);
	fSaveDataCmd->SetGuidance("Usage: /minisims/analysis/SaveData SaveLev");
	fSaveDataCmd->SetGuidance("Control for data tree saving:");
	txt.str(""); txt << (int)DatasaveLevel::kOff << " to not save;";
	fSaveDataCmd->SetGuidance(txt.str().c_str());
	txt.str(""); txt << (int)DatasaveLevel::kLUT << " save hit variables in defined sensitive volumes with limited primary info (low memeory usage);";
	fSaveDataCmd->SetGuidance(txt.str().c_str());
	txt.str(""); txt << (int)DatasaveLevel::kHits << " save hit variables in defined sensitive volumes (default);";
	fSaveDataCmd->SetGuidance(txt.str().c_str());
	txt.str(""); txt << (int)DatasaveLevel::kHitsExt << " save extended hit variables in defined sensitive volumes;";
	fSaveDataCmd->SetGuidance(txt.str().c_str());
	txt.str(""); txt << (int)DatasaveLevel::kSdSteps << " save stepping variables in defined sensitive volumes (heavy);";
	fSaveDataCmd->SetGuidance(txt.str().c_str());
	txt.str(""); txt << (int)DatasaveLevel::kAll << " save stepping variables in every volume (very heavy);";
	fSaveDataCmd->SetGuidance(txt.str().c_str());
	fSaveDataCmd->SetParameterName("SaveLev", false);
	txt.str(""); txt << "SaveLev>=" << static_cast<G4int>(DatasaveLevel::kOff) << " && SaveLev<=" << static_cast<G4int>(DatasaveLevel::kAll);
	fSaveDataCmd->SetRange( txt.str().c_str() );
	fSaveDataCmd->SetDefaultValue(static_cast<G4int>(DatasaveLevel::kHits));
	fSaveDataCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	fFileNameCmd = new G4UIcmdWithAString("/minisims/analysis/FileName",this);
	fFileNameCmd->SetGuidance("Usage: /minisims/analysis/FileName [/path/]<filename.root>");
	fFileNameCmd->SetGuidance("Set the file name where data tree will be saved. The path can be a relative path.");
	fFileNameCmd->SetParameterName("filename", false);
	fFileNameCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// Define sensitive volumes
	fDefOptSDCmd = new G4UIcmdWithAString("/minisims/analysis/DefOptSD", this);
	fDefOptSDCmd->SetGuidance("Defines a list of physical volume as sensitive (NULL to unset).");
	fDefOptSDCmd->SetGuidance("Note that in hit mode the optical photon is killed also at its first step in the volume.");
	fDefOptSDCmd->SetGuidance("Usage: /minisims/analysis/DefOptSD VolName1 VolName2 ...");
	fDefOptSDCmd->SetParameterName("VolName", true, true);
	fDefOptSDCmd->SetDefaultValue("NULL");
	fDefOptSDCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// Define sensitive volumes
	fDefAbsVolCmd = new G4UIcmdWithAString("/minisims/analysis/DefAbsVols", this);
	fDefAbsVolCmd->SetGuidance("Defines a list of physical volume where the optical photons will be absorbed (killed) as soon as they enter in (NULL to unset).");
	fDefAbsVolCmd->SetGuidance("DefAbsVols: detvol VolName1 VolName2 ...");
	fDefAbsVolCmd->SetParameterName("AbsVolNames", true, true);
	fDefAbsVolCmd->SetDefaultValue("NULL");
	fDefAbsVolCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	// Autoflush and autosave for data tree
	fAutoFlushCmd = new G4UIcmdWithAnInteger("/minisims/analysis/SetAutoFlush",this);
	fAutoFlushCmd->SetGuidance("Autoflush settings of the data TTree (see ROOT reference guide for more information).");
	fAutoFlushCmd->SetParameterName("AutoFlush", false, false);
	fAutoFlushCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	fAutoSaveCmd = new G4UIcmdWithAnInteger("/minisims/analysis/SetAutoSave",this);
	fAutoSaveCmd->SetGuidance("Autosave settings of the data TTree (see ROOT reference guide for more information).");
	fAutoSaveCmd->SetParameterName("AutoSave", false, false);
	fAutoSaveCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
	
	fRandSeedCmd = new G4UIcmdWithAnInteger("/minisims/analysis/SetRandSeed",this);
	fRandSeedCmd->SetGuidance("Manual set of the random seed. 0 is taken from machine time (default).");
	fRandSeedCmd->SetParameterName("RandSeed", true);
	fRandSeedCmd->SetDefaultValue(0);
	fRandSeedCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}


AnalysisMessenger::~AnalysisMessenger()
{
	delete fVerboseCmd;
	delete fPrintModuloCmd;
	delete fStepDebugCmd;
	delete fSaveDataCmd;
	delete fFileNameCmd;
	delete fDefOptSDCmd;
	delete fDefAbsVolCmd;
	
	delete fAnalysisDir;
	delete fAutoSaveCmd;
	delete fAutoFlushCmd;
	delete fRandSeedCmd;
}


void AnalysisMessenger::SetNewValue(G4UIcommand *pUIcommand, G4String hNewValue)
{
	
	if(pUIcommand == fVerboseCmd){
		std::cout << "Info --> AnalysisMessenger::SetNewValue(...): called command fVerboseCmd" << std::endl;
		fAnManager->SetVerbosity(static_cast<AnalysisVerbosity>(fVerboseCmd->GetNewIntValue(hNewValue)));
		return;
	}
	
	if(pUIcommand == fPrintModuloCmd){
		std::cout << "Info --> AnalysisMessenger::SetNewValue(...): called command fPrintModuloCmd" << std::endl;
		fAnManager->SetPrintModulo(fPrintModuloCmd->GetNewIntValue(hNewValue));
		return;
	}
	
	if(pUIcommand == fStepDebugCmd){
		std::cout << "Info --> AnalysisMessenger::SetNewValue(...): called command fVerboseCmd. Setting the analysis manager steps debug flag to: " << G4UIcommand::ConvertToString(fStepDebugCmd->GetNewBoolValue(hNewValue)) << std::endl;
		fAnManager->SetStepsDebug(fStepDebugCmd->GetNewBoolValue(hNewValue));
		return;
	}
	
	if(pUIcommand == fSaveDataCmd){
		std::cout << "Info --> AnalysisMessenger::SetNewValue(...): called command fSaveDataCmd. Setting the analysis manager saving flag to: " << fSaveDataCmd->ConvertToString(fSaveDataCmd->GetNewIntValue(hNewValue)) << std::endl;
		fAnManager->SetSaveData(static_cast<DatasaveLevel>(fSaveDataCmd->GetNewIntValue(hNewValue)));
		return;
	}
	
	if(pUIcommand == fFileNameCmd){
		std::cout << "Info --> AnalysisMessenger::SetNewValue(...): called command fFileNameCmd. Setting the tree file name to: " << hNewValue << std::endl;
		if(fAnManager->GetSaveStatus()>DatasaveLevel::kOff){
			fAnManager->SetDataFilename(hNewValue);
		}
		return;
	}
	
	if(pUIcommand == fDefOptSDCmd){
		std::cout << "Info --> AnalysisMessenger::SetNewValue(...): called command fDefOptSDCmd. Setting the optical photons sensitive volumes file name to: " << hNewValue << std::endl;
		fAnManager->DefineOptPhSensDet(hNewValue);
		return;
	}
	
	
	if(pUIcommand == fDefAbsVolCmd){
		std::cout << "Info --> AnalysisMessenger::SetNewValue(...): called command fDefAbsVolCmd. Setting the optical photons detection volumes file name to: " << hNewValue << std::endl;
		fAnManager->DefineOptPhAbsVols(hNewValue);
		return;
	}
	
	
	if(pUIcommand == fAutoFlushCmd){
		std::cout << "Info --> AnalysisMessenger::SetNewValue(...): called command fAutoFlushCmd. Setting the data TTree autoflush to " << hNewValue << std::endl;
		fAnManager->SetAutoFlush( std::stoll(hNewValue) );
		return;
	}
	
	if(pUIcommand == fAutoSaveCmd){
		std::cout << "Info --> AnalysisMessenger::SetNewValue(...): called command fAutoSaveCmd. Setting the data TTree autosave to " << hNewValue << std::endl;
		fAnManager->SetAutoSave( std::stoll(hNewValue) );
		return;
	}
	
	if(pUIcommand == fRandSeedCmd){
		std::cout << "Info --> AnalysisMessenger::SetNewValue(...): called command fRandSeedCmd. Setting the random seed to " << hNewValue << std::endl;
		fAnManager->SetRunSeed( std::stoi(hNewValue) );
		return;
	}
	
	
	std::cout << "\nERROR ---> AnalysisMessenger::SetNewValue(...): not recognized command!" << std::endl;

}


