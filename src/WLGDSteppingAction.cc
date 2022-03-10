//
// Created by moritz on 3/4/21.
//

#include <iostream>

using namespace std;
#include "WLGDRunAction.hh"
#include "WLGDSteppingAction.hh"
#include "WLGDTrackingAction.hh"

#include "G4SystemOfUnits.hh"

#include "G4RunManager.hh"

WLGDSteppingAction::WLGDSteppingAction(WLGDEventAction* event, WLGDRunAction* run,
                                       WLGDDetectorConstruction* det)
{
  fEventAction          = event;
  fRunAction            = run;
  fDetectorConstruction = det;
  DefineCommands();
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void WLGDSteppingAction::UserSteppingAction(const G4Step* aStep)
{
#define MostOuterRadiusTracking 0
  // Edit: 2021/03/05 by Moritz Neuberger
  // Adding tracking of amount of neutrons crossing the detectors
#if MostOuterRadiusTracking == 1
  if(aStep->GetTrack()->GetParticleDefinition()->GetParticleName() == "neutron")
  {
    G4double tmp_x = aStep->GetTrack()->GetPosition().getX() / m;
    G4double tmp_y = aStep->GetTrack()->GetPosition().getY() / m;
    fEventAction->UpdateMostOuterRadius(sqrt(tmp_x * tmp_x + tmp_y * tmp_y));
    if(aStep->GetTrack()->GetNextVolume())
    {
      auto physVol1 = aStep->GetTrack()->GetVolume();
      auto physVol2 = aStep->GetTrack()->GetNextVolume();
      if(physVol1->GetName() != "Ge_phys" && physVol2->GetName() == "Ge_phys")
      {
        if(fRunAction->getWriteOutGeneralNeutronInfo() == 1)
          fRunAction->increaseNumberOfCrossingNeutrons();
      }
    }
  }
#endif

  // Edit: 2022/02/08 by Michele Morella
  // Adding target nucleus of neutron capture
  G4SteppingManager*  steppingManager = fpSteppingManager;
  
  // check if it is alive
  if(aStep->GetTrack()->GetTrackStatus() == fAlive) { return; }

  // Retrieve the secondary particles
  auto fSecondary = steppingManager -> GetfSecondary();

  for(size_t lp1=0;lp1<(*fSecondary).size(); lp1++)
  {
    // Retrieve the info about the generation of secondary particles 
    G4int encoding = (*fSecondary)[lp1] -> GetParticleDefinition() -> GetPDGEncoding();
    // G4int charge = (*fSecondary)[lp1] -> GetParticleDefinition() -> GetPDGCharge();
    // G4String process = (*fSecondary)[lp1]-> GetCreatorProcess()-> GetProcessName();   // process creating it
    // G4int AA = (*fSecondary)[lp1] -> GetDynamicParticle() -> GetDefinition() -> GetBaryonNumber();
    // G4String volumeName = (*fSecondary)[lp1] -> GetVolume() -> GetLogicalVolume() -> GetName(); // volume where secondary was generated 
    // G4String secondaryParticleName =  (*fSecondary)[lp1]->GetDefinition() -> GetParticleName();  // name of the secondary
    if(encoding == 1000320771 || encoding == 1000320770)
    {
      aStep->GetTrack()->SetTrackStatus(fKillTrackAndSecondaries);
    }
  }

  // Edit: 2021/04/07 by Moritz Neuberger
  // Adding total energy deposition inside LAr

  if(fDepositionInfo == 1)
  {
    if(aStep->GetTrack()->GetVolume()->GetName() == "World_phys")
      return;
    if((aStep->GetPostStepPoint()->GetTouchable()->GetVolume(0)->GetLogicalVolume()->GetName() == "Lar_log") ||
        aStep->GetPostStepPoint()->GetTouchable()->GetVolume(0)->GetLogicalVolume()->GetName() == "ULar_log" ||
        aStep->GetPostStepPoint()->GetTouchable()->GetVolume(0)->GetLogicalVolume()->GetName() == "Ge_log"   ||
        aStep->GetPostStepPoint()->GetTouchable()->GetVolume(0)->GetLogicalVolume()->GetName() == "Water_log" )
    {
      if(aStep->GetTotalEnergyDeposit() > 0)
      {
        // get position of deposition
        G4double tmp_x = aStep->GetTrack()->GetPosition().getX();
        G4double tmp_y = aStep->GetTrack()->GetPosition().getY();
        G4double tmp_z = aStep->GetTrack()->GetPosition().getZ();

        // get id of which reentrance tube
        G4int whichReentranceTube;
        if(abs(tmp_x) > abs(tmp_y) && tmp_x > 0)
          whichReentranceTube = 0;
        if(abs(tmp_x) > abs(tmp_y) && tmp_x < 0)
          whichReentranceTube = 2;
        if(abs(tmp_x) < abs(tmp_y) && tmp_y > 0)
          whichReentranceTube = 1;
        if(abs(tmp_x) < abs(tmp_y) && tmp_y < 0)
          whichReentranceTube = 3;

        if(fDetectorConstruction->GetGeometryName() == "hallA" ||
           aStep->GetPostStepPoint()->GetTouchable()->GetVolume(0)->GetLogicalVolume()->GetName() == "Lar_log" ||
           aStep->GetPostStepPoint()->GetTouchable()->GetVolume(0)->GetLogicalVolume()->GetName() == "Water_log")
          whichReentranceTube = 0;

        // calculate total energy deposition in water tank for muon veto
        if(aStep->GetPostStepPoint()->GetTouchable()->GetVolume(0)->GetLogicalVolume()->GetName() == "Water_log")
        {
          fEventAction->IncreaseEdepWater_prompt(aStep->GetTotalEnergyDeposit() / eV);
          return;
        }

        // LAr veto

        G4int whichVolume = -1;
        if(aStep->GetPostStepPoint()->GetTouchable()->GetVolume(0)->GetLogicalVolume()->GetName() == "ULar_log" ||
           (fDetectorConstruction->GetGeometryName() == "hallA" && aStep->GetTrack()->GetLogicalVolumeAtVertex()->GetName() == "Lar_log"))
        {
          whichVolume = 0;
          if(aStep->GetPostStepPoint()->GetGlobalTime() / us < 10.)
          {
            fEventAction->IncreaseLArEnergyDeposition(aStep->GetTotalEnergyDeposit() / eV, whichReentranceTube);
          }  // prompt
          else
          {
            if(aStep->GetPostStepPoint()->GetGlobalTime() / ms < 1.)
            {
              fEventAction->IncreaseLArEnergyDeposition_delayed(aStep->GetTotalEnergyDeposit() / eV, whichReentranceTube);
            }  // delayed
            if(aStep->GetPostStepPoint()->GetGlobalTime() / s < 1. && fRunAction->getWriteOutAdvancedMultiplicity())
            {
              fEventAction->IncreaseLArEnergyDeposition_delayed_long(aStep->GetTotalEnergyDeposit() / eV, whichReentranceTube);
            }  // long delayed
          }
          if(aStep->GetPostStepPoint()->GetGlobalTime() / s > 1. && fRunAction->getWriteOutAdvancedMultiplicity())
          {
            fEventAction->IncreaseLArEnergyDeposition_after_delayed(aStep->GetTotalEnergyDeposit() / eV, whichReentranceTube);
          }  // after delayed
        }

        // Ge energy

        if(aStep->GetPostStepPoint()->GetTouchable()->GetVolume(0)->GetLogicalVolume()->GetName() == "Ge_log")
        {
          whichVolume = 1;
          if(aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetLogicalVolume()->GetName() == "Layer_log")
          {
            fEventAction->IncreaseEdepPerDetector(aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() + whichReentranceTube * 96, aStep->GetTotalEnergyDeposit() / keV);
            //commented out by Michele (could be useless)
            /*
            if(fRunAction->getWriteOutAdvancedMultiplicity())
            {
              if(fEventAction->GetIDListOfGdSiblingParticles().count(aStep->GetTrack()->GetParentID()))
                fEventAction->IncreaseEdepPerDetector_prompt_onlyGd(aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() + whichReentranceTube * 96, aStep->GetTotalEnergyDeposit() / eV);
              else
                fEventAction->IncreaseEdepPerDetector_prompt_woGd(aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() + whichReentranceTube * 96, aStep->GetTotalEnergyDeposit() / eV);
            } */ // w/ and w/o Gd info (redundant)
            }
            else
              G4cout
              << "Trying to access Layer_log for the prompt multiplicity but it is "
              << aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetLogicalVolume()->GetName()
              << G4endl;
          // total energy released in Ge detectors
         
          if(aStep->GetPostStepPoint()->GetGlobalTime() / us < 10.)
          {
            if(aStep->GetPostStepPoint()
                 ->GetTouchable()
                 ->GetVolume(1)
                 ->GetLogicalVolume()
                 ->GetName() == "Layer_log")
            {
              fEventAction->IncreaseEdepPerDetector_prompt(
                aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() +
                  whichReentranceTube * 96,
                aStep->GetTotalEnergyDeposit() / eV);
              if(fRunAction->getWriteOutAdvancedMultiplicity())
              {
                if(fEventAction->GetIDListOfGdSiblingParticles().count(
                     aStep->GetTrack()->GetParentID()))
                  fEventAction->IncreaseEdepPerDetector_prompt_onlyGd(
                    aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() +
                      whichReentranceTube * 96,
                    aStep->GetTotalEnergyDeposit() / eV);
                else
                  fEventAction->IncreaseEdepPerDetector_prompt_woGd(
                    aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() +
                      whichReentranceTube * 96,
                    aStep->GetTotalEnergyDeposit() / eV);
              }  // w/ and w/o Gd info (redundant)
            }
            else
              G4cout
                << "Trying to access Layer_log for the prompt multiplicity but it is "
                << aStep->GetPostStepPoint()
                     ->GetTouchable()
                     ->GetVolume(1)
                     ->GetLogicalVolume()
                     ->GetName()
                << G4endl;
          }  // prompt
          else
          {
            if(aStep->GetPostStepPoint()->GetGlobalTime() / ms < 1.)
            {
              if(aStep->GetPostStepPoint()
                   ->GetTouchable()
                   ->GetVolume(1)
                   ->GetLogicalVolume()
                   ->GetName() == "Layer_log")
              {
                fEventAction->IncreaseEdepPerDetector_delayed(
                  aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() +
                    whichReentranceTube * 96,
                  aStep->GetTotalEnergyDeposit() / eV);

                if(fRunAction->getWriteOutAdvancedMultiplicity())
                {
                  if(fEventAction->GetIDListOfGdSiblingParticles().count(
                       aStep->GetTrack()->GetParentID()))
                    fEventAction->IncreaseEdepPerDetector_delayed_onlyGd(
                      aStep->GetPostStepPoint()
                          ->GetTouchable()
                          ->GetVolume(1)
                          ->GetCopyNo() +
                        whichReentranceTube * 96,
                      aStep->GetTotalEnergyDeposit() / eV);
                  else
                    fEventAction->IncreaseEdepPerDetector_delayed_woGd(
                      aStep->GetPostStepPoint()
                          ->GetTouchable()
                          ->GetVolume(1)
                          ->GetCopyNo() +
                        whichReentranceTube * 96,
                      aStep->GetTotalEnergyDeposit() / eV);
                }
              }
              else
                G4cout
                  << "Trying to access Layer_log for the delayed multiplicity but it is "
                  << aStep->GetPostStepPoint()
                       ->GetTouchable()
                       ->GetVolume(1)
                       ->GetLogicalVolume()
                       ->GetName()
                  << G4endl;
            }  // delayed
            if(aStep->GetPostStepPoint()->GetGlobalTime() / s < 1. &&
               fRunAction->getWriteOutAdvancedMultiplicity())
            {
              fEventAction->IncreaseEdepPerDetector_delayed_long(
                aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() +
                  whichReentranceTube * 96,
                aStep->GetTotalEnergyDeposit() / eV);
            }  // long delayed
          }
          if(aStep->GetPostStepPoint()->GetGlobalTime() / s > 1. &&
             fRunAction->getWriteOutAdvancedMultiplicity())
          {
            fEventAction->IncreaseGeEnergyDeposition_after_delayed(
              aStep->GetTotalEnergyDeposit() / eV, whichReentranceTube);
          } // after delayed
        }

        if(fRunAction->getIndividualGeDepositionInfo())
        {
          if(fEventAction->GetIDListOfGe77().count(aStep->GetTrack()->GetParentID()))
          {
            fEventAction->AddGe77mGammaEmission_timing(
              aStep->GetPostStepPoint()->GetGlobalTime() / s);
            fEventAction->AddGe77mGammaEmission_x(
              aStep->GetPostStepPoint()->GetGlobalTime() / m);
            fEventAction->AddGe77mGammaEmission_y(
              aStep->GetPostStepPoint()->GetGlobalTime() / m);
            fEventAction->AddGe77mGammaEmission_z(
              aStep->GetPostStepPoint()->GetGlobalTime() / m);
            fEventAction->AddGe77mGammaEmission_edep(aStep->GetTotalEnergyDeposit() / eV);
            fEventAction->AddGe77mGammaEmission_id(aStep->GetTrack()->GetTrackID());
            fEventAction->AddGe77mGammaEmission_type(
              aStep->GetTrack()->GetParticleDefinition()->GetPDGEncoding());
            fEventAction->AddGe77mGammaEmission_whichGe77(
              aStep->GetTrack()->GetParentID());
            int whichVolume = -3;
            if(aStep->GetPostStepPoint()
                 ->GetTouchable()
                 ->GetVolume()
                 ->GetLogicalVolume()
                 ->GetName() == "Lar_log")
              whichVolume = -2;
            if(aStep->GetPostStepPoint()
                 ->GetTouchable()
                 ->GetVolume(0)
                 ->GetLogicalVolume()
                 ->GetName() == "ULar_log")
              whichVolume = -1;
            if(aStep->GetPostStepPoint()
                 ->GetTouchable()
                 ->GetVolume(0)
                 ->GetLogicalVolume()
                 ->GetName() == "Ge_log")
              whichVolume =
                aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() +
                whichReentranceTube * 96;
            fEventAction->AddGe77mGammaEmission_whichVolume(whichVolume);
          }

          if(fEventAction->GetIDListOfGe77SiblingParticles().count(
               aStep->GetTrack()->GetParentID()))
          {
            fEventAction->AddGe77Siblings_timing(
              aStep->GetPostStepPoint()->GetGlobalTime() / s);
            fEventAction->AddGe77Siblings_x(
              aStep->GetPostStepPoint()->GetPosition().getX() / m);
            fEventAction->AddGe77Siblings_y(
              aStep->GetPostStepPoint()->GetPosition().getY() / m);
            fEventAction->AddGe77Siblings_z(
              aStep->GetPostStepPoint()->GetPosition().getZ() / m);
            fEventAction->AddGe77Siblings_edep(aStep->GetTotalEnergyDeposit() / eV);
            fEventAction->AddGe77Siblings_id(aStep->GetTrack()->GetTrackID());
            fEventAction->AddGe77Siblings_type(
              aStep->GetTrack()->GetParticleDefinition()->GetPDGEncoding());
            int whichVolume = -3;
            if(aStep->GetPostStepPoint()
                 ->GetTouchable()
                 ->GetVolume()
                 ->GetLogicalVolume()
                 ->GetName() == "Lar_log")
              whichVolume = -2;
            if(aStep->GetPostStepPoint()
                 ->GetTouchable()
                 ->GetVolume(0)
                 ->GetLogicalVolume()
                 ->GetName() == "ULar_log")
              whichVolume = -1;
            if(aStep->GetPostStepPoint()
                 ->GetTouchable()
                 ->GetVolume(0)
                 ->GetLogicalVolume()
                 ->GetName() == "Ge_log")
              whichVolume =
                aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() +
                whichReentranceTube * 96;
            fEventAction->AddGe77Siblings_whichVolume(whichVolume);
          }
        }  // individual deposition of sibling or secundary Ge77 interactions

        if(fRunAction->getIndividualGdDepositionInfo())
        {
          if(fEventAction->GetIDListOfGdSiblingParticles().count(
               aStep->GetTrack()->GetParentID()))
          {
            // G4cout << " ______________________________________________ " << G4endl <<
            // aStep->GetTrack()->GetTrackID() << " " <<
            // aStep->GetTrack()->GetParticleDefinition()->GetPDGEncoding() << " "  <<
            // aStep->GetTrack()->GetParentID() << G4endl << "
            // ______________________________________________ " << G4endl;
            fEventAction->AddGdSiblings_timing(
              aStep->GetPostStepPoint()->GetGlobalTime() / s);
            fEventAction->AddGdSiblings_x(
              aStep->GetPostStepPoint()->GetPosition().getX() / m);
            fEventAction->AddGdSiblings_y(
              aStep->GetPostStepPoint()->GetPosition().getY() / m);
            fEventAction->AddGdSiblings_z(
              aStep->GetPostStepPoint()->GetPosition().getZ() / m);
            fEventAction->AddGdSiblings_edep(aStep->GetTotalEnergyDeposit() / eV);
            fEventAction->AddGdSiblings_id(aStep->GetTrack()->GetTrackID());
            fEventAction->AddGdSiblings_type(
              aStep->GetTrack()->GetParticleDefinition()->GetPDGEncoding());
            int whichVolume = -4;
            if(aStep->GetPostStepPoint()
                 ->GetTouchable()
                 ->GetVolume()
                 ->GetLogicalVolume()
                 ->GetName() == "Water_log")
              whichVolume = -3;
            if(aStep->GetPostStepPoint()
                 ->GetTouchable()
                 ->GetVolume()
                 ->GetLogicalVolume()
                 ->GetName() == "Lar_log")
              whichVolume = -2;
            if(aStep->GetPostStepPoint()
                 ->GetTouchable()
                 ->GetVolume(0)
                 ->GetLogicalVolume()
                 ->GetName() == "ULar_log")
              whichVolume = -1;
            if(aStep->GetPostStepPoint()
                 ->GetTouchable()
                 ->GetVolume(0)
                 ->GetLogicalVolume()
                 ->GetName() == "Ge_log")
              whichVolume =
                aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() +
                whichReentranceTube * 96;
            fEventAction->AddGdSiblings_whichVolume(whichVolume);
          }
        }  // individual Gd interactions

        if(aStep->GetPostStepPoint()->GetGlobalTime() / s > 1 &&
           fAllowForLongTimeEmissionReadout == 0)
          return;  // skip all interactiosn >1s

        if(fIndividualDepositionInfo == 0 &&
           fRunAction->getIndividualGeDepositionInfo() == 0)
          return;

        if(fRunAction->getIndividualGeDepositionInfo() == 1 &&
           aStep->GetPostStepPoint()
               ->GetTouchable()
               ->GetVolume(1)
               ->GetLogicalVolume()
               ->GetName() != "Layer_log")
          return;
        // all individual interactions
        {
          fEventAction->AddIndividualEnergyDeposition_Timing(
            aStep->GetPostStepPoint()->GetGlobalTime() / (1000 * ns));
          fEventAction->AddIndividualEnergyDeposition_Energy(
            aStep->GetTotalEnergyDeposit() / eV);
          fEventAction->AddIndividualEnergyDeposition_ReentranceTube(whichReentranceTube);
          fEventAction->AddIndividualEnergyDeposition_Position_x(tmp_x / m);
          fEventAction->AddIndividualEnergyDeposition_Position_y(tmp_y / m);
          fEventAction->AddIndividualEnergyDeposition_Position_z(tmp_z / m);
          fEventAction->AddIndividualEnergyDeposition_LArOrGe(whichVolume);
          fEventAction->AddIndividualEnergyDeposition_ID(aStep->GetTrack()->GetTrackID());
          fEventAction->AddIndividualEnergyDeposition_Type(
            aStep->GetTrack()->GetParticleDefinition()->GetPDGEncoding());
          int tmp = -1;
          if(aStep->GetPostStepPoint()
               ->GetTouchable()
               ->GetVolume(1)
               ->GetLogicalVolume()
               ->GetName() == "Layer_log")
            tmp = aStep->GetPostStepPoint()->GetTouchable()->GetVolume(1)->GetCopyNo() +
                  whichReentranceTube * 96;
          fEventAction->AddIndividualEnergyDeposition_DetectorNumber(tmp);
        }
      }
    }
  }
}

void WLGDSteppingAction::GetDepositionInfo(G4int answer) { fDepositionInfo = answer; }
void WLGDSteppingAction::GetIndividualDepositionInfo(G4int answer)
{
  fIndividualDepositionInfo = answer;
}
void WLGDSteppingAction::AllowForLongTimeEmissionReadout(G4int answer)
{
  fAllowForLongTimeEmissionReadout = answer;
}

void WLGDSteppingAction::DefineCommands()
{
  // Define geometry command directory using generic messenger class
  fStepMessenger = new G4GenericMessenger(this, "/WLGD/step/",
                                          "Commands for controlling stepping action");

  // switch command
  fStepMessenger
    ->DeclareMethod("getDepositionInfo", &WLGDSteppingAction::GetDepositionInfo)
    .SetGuidance(
      "Set whether to obtain energy deposition information inside reentrance tubes")
    .SetGuidance("0 = don't")
    .SetGuidance("1 = do")
    .SetCandidates("0 1")
    .SetDefaultValue("0");

  fStepMessenger
    ->DeclareMethod("getIndividualDepositionInfo",
                    &WLGDSteppingAction::GetIndividualDepositionInfo)
    .SetGuidance("Set whether to obtain individual energy deposition information inside "
                 "reentrance tubes")
    .SetGuidance("0 = don't")
    .SetGuidance("1 = do")
    .SetCandidates("0 1")
    .SetDefaultValue("0");

  fStepMessenger
    ->DeclareMethod("AllowForLongTimeEmissionReadout",
                    &WLGDSteppingAction::AllowForLongTimeEmissionReadout)
    .SetGuidance("Set whether to individual deposition information can be gathered from "
                 "longer than 1sec after start of event.")
    .SetGuidance("0 = don't")
    .SetGuidance("1 = do")
    .SetCandidates("0 1")
    .SetDefaultValue("0");
}
