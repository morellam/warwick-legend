#include "WLGDDetectorConstruction.hh"

#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4Tubs.hh"

#include "G4Colour.hh"
#include "G4VisAttributes.hh"

#include "G4MultiFunctionalDetector.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4SDManager.hh"
#include "G4VPrimitiveScorer.hh"
#include "WLGDPSLocation.hh"

#include "WLGDBiasMultiParticleChangeCrossSection.hh"

#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"

WLGDDetectorConstruction::WLGDDetectorConstruction()
: G4VUserDetectorConstruction()
, fBaseline(true)
, fDetectorMessenger(nullptr)
{
    DefineCommands();
}

WLGDDetectorConstruction::~WLGDDetectorConstruction() { delete fDetectorMessenger; }

G4VPhysicalVolume* WLGDDetectorConstruction::Construct()
{
    if(fBaseline)
        return SetupBaseline();

    else
        return SetupAlternative();
}

void WLGDDetectorConstruction::ConstructSDandField()
{
    auto lardet = new G4MultiFunctionalDetector("LarDet");
    G4SDManager::GetSDMpointer()->AddNewDetector(lardet);
    auto ulardet = new G4MultiFunctionalDetector("ULarDet");
    G4SDManager::GetSDMpointer()->AddNewDetector(ulardet);
    auto gedet = new G4MultiFunctionalDetector("GeDet");
    G4SDManager::GetSDMpointer()->AddNewDetector(gedet);
    auto waterdet = new G4MultiFunctionalDetector("WaterDet");
    G4SDManager::GetSDMpointer()->AddNewDetector(waterdet);

    auto primitive = new G4PSEnergyDeposit("Edep", "MeV");  // unit [MeV]
    waterdet->RegisterPrimitive(primitive);
    lardet->RegisterPrimitive(primitive);
    ulardet->RegisterPrimitive(primitive);
    gedet->RegisterPrimitive(primitive);

    auto locprimitive = new WLGDPSLocation("Loc", "m");  // unit [m]
    waterdet->RegisterPrimitive(locprimitive);
    lardet->RegisterPrimitive(locprimitive);
    ulardet->RegisterPrimitive(locprimitive);
    gedet->RegisterPrimitive(locprimitive);

    // ----------------------------------------------
    // -- operator creation and attachment to volume:
    // ----------------------------------------------
    auto biasnXS  = new WLGDBiasMultiParticleChangeCrossSection();
    auto biasmuXS = new WLGDBiasMultiParticleChangeCrossSection();
    biasmuXS->AddParticle("mu-");
    biasnXS->AddParticle("neutron");

    if(fBaseline)
    {
        // -- Fetch volume for scoring:
        G4LogicalVolume* logicWater =
            G4LogicalVolumeStore::GetInstance()->GetVolume("Water_log");
        G4LogicalVolume* logicLar =
            G4LogicalVolumeStore::GetInstance()->GetVolume("Lar_log");
        G4LogicalVolume* logicULar =
            G4LogicalVolumeStore::GetInstance()->GetVolume("Ular_log");
        G4LogicalVolume* logicGe =
            G4LogicalVolumeStore::GetInstance()->GetVolume("Ge_log");

        SetSensitiveDetector(logicWater, waterdet);
        SetSensitiveDetector(logicLar, lardet);
        SetSensitiveDetector(logicULar, ulardet);
        SetSensitiveDetector(logicGe, gedet);

        // muon bias
        biasmuXS->AttachTo(logicWater);
        biasmuXS->AttachTo(logicLar);
        biasmuXS->AttachTo(logicULar);
        // neutron bias
        biasnXS->AttachTo(logicGe);
    }

    else
    {
        // -- Fetch volume for scoring:
        G4LogicalVolume* logicLar =
            G4LogicalVolumeStore::GetInstance()->GetVolume("Lar_log");
        G4LogicalVolume* logicULar =
            G4LogicalVolumeStore::GetInstance()->GetVolume("Ular_log");
        G4LogicalVolume* logicGe =
            G4LogicalVolumeStore::GetInstance()->GetVolume("Ge_log");

        SetSensitiveDetector(logicLar, lardet);
        SetSensitiveDetector(logicULar, ulardet);
        SetSensitiveDetector(logicGe, gedet);

        // muon bias
        biasmuXS->AttachTo(logicLar);
        biasmuXS->AttachTo(logicULar);
        // neutron bias
        biasnXS->AttachTo(logicGe);
    }
}

G4VPhysicalVolume* WLGDDetectorConstruction::SetupAlternative()
{
    G4Material* worldMaterial =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
    G4Material* larMat = G4NistManager::Instance()->FindOrBuildMaterial("G4_lAr");
    G4Material* concreteWallMat =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_CONCRETE");
    G4Material* airMat = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
    G4Material* steelMat =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_STAINLESS-STEEL");
    G4Material* copperMat = G4NistManager::Instance()->FindOrBuildMaterial("G4_Cu");

    auto H     = new G4Element("Hydrogen", "H", 1., 1.00794 * g / mole);
    auto C     = new G4Element("Carbon", "C", 6., 12.011 * g / mole);
    auto O     = new G4Element("Oxygen", "O", 8., 16.00 * g / mole);
    auto N     = new G4Element("Nitrogen", "N", 7., 14.00 * g / mole);
    auto puMat = new G4Material("polyurethane", 0.3 * g / cm3, 4);  // high density foam
    puMat->AddElement(H, 16);
    puMat->AddElement(O, 2);
    puMat->AddElement(C, 8);
    puMat->AddElement(N, 2);

    // enriched Germanium from isotopes
    auto Ge_74 = new G4Isotope("Ge74", 32, 74, 74.0 * g / mole);
    auto Ge_76 = new G4Isotope("Ge76", 32, 76, 76.0 * g / mole);

    auto eGe = new G4Element("enriched Germanium", "enrGe", 2);
    eGe->AddIsotope(Ge_76, 88. * perCent);
    eGe->AddIsotope(Ge_74, 12. * perCent);

    G4double density = 3.323 * mg / cm3;
    auto     roiMat  = new G4Material("enrGe", density, 1);
    roiMat->AddElement(eGe, 1);

    // size parameter, unit [cm]
    // cavern
    G4double stone     = 100.0;  // Hall wall thickness 1 m
    G4double hallhside = 850.0;  // Hall cube side 17 m
    // cryostat
    G4double tankhside  = 650;   // cryostat cube side 13 m
    G4double outerwall  = 1.2;   // outer SS wall thickness
    G4double insulation = 80.0;  // polyurethane foam
    G4double innerwall  = 0.12;  // inner SS membrane
    // copper tubes with Germanium ROI
    G4double copper    = 0.35;    // tube thickness 0.5 mm
    G4double curad     = 40.0;    // copper tube diam 80 cm
    G4double cuhheight = 334.34;  // copper tube height 7 m inside cryostat
    G4double cushift   = 234.34;  // shift cu tube inside cryostat to top
    G4double ringrad   = 100.0;   // cu tube placement ring radius
    // Ge cylinder for 250 kg at 5.32 g/cm3
    G4double roiradius     = 25.0;   // detector region diam 50 cm
    G4double roihalfheight = 11.97;  // detector region height 24 cm

    // total
    G4double offset =
        hallhside - tankhside;  // shift cavern floor to keep detector centre at origin
    G4double worldside = hallhside + stone + offset + 0.1;  // larger than rest
    G4double larside =
        tankhside - outerwall - insulation - innerwall;  // cube side of LAr volume

    fvertexZ = worldside - 0.1;  // max vertex height
    fmaxrad  = fvertexZ;         // max vertex circle radius

    // Volumes for this geometry

    //
    // World
    //
    G4VSolid* worldSolid =
        new G4Box("World", worldside * cm, worldside * cm, worldside * cm);
    auto fWorldLogical = new G4LogicalVolume(worldSolid, worldMaterial, "World_log");
    auto fWorldPhysical =
        new G4PVPlacement(0, G4ThreeVector(), fWorldLogical, "World_phys", 0, false, 0);

    //
    // Cavern
    //
    G4VSolid* cavernSolid = new G4Box("Cavern", (hallhside + stone) * cm,
                                      (hallhside + stone) * cm, (hallhside + stone) * cm);
    auto fCavernLogical = new G4LogicalVolume(cavernSolid, concreteWallMat, "Cavern_log");
    auto fCavernPhysical =
        new G4PVPlacement(0, G4ThreeVector(0., 0., offset * cm), fCavernLogical,
                          "Cavern_phys", fWorldLogical, false, 0);

    //
    // Hall
    //
    G4VSolid* hallSolid =
        new G4Box("Cavern", hallhside * cm, hallhside * cm, hallhside * cm);
    auto fHallLogical = new G4LogicalVolume(hallSolid, airMat, "Hall_log");
    auto fHallPhysical =
        new G4PVPlacement(0, G4ThreeVector(0., 0., offset * cm), fHallLogical,
                          "Hall_phys", fCavernLogical, false, 0);

    //
    // Tank
    //
    G4VSolid* tankSolid =
        new G4Box("Tank", tankhside * cm, tankhside * cm, tankhside * cm);
    auto fTankLogical  = new G4LogicalVolume(tankSolid, steelMat, "Tank_log");
    auto fTankPhysical = new G4PVPlacement(0, G4ThreeVector(), fTankLogical, "Tank_phys",
                                           fHallLogical, false, 0);

    //
    // Insulator
    //
    G4VSolid* puSolid =
        new G4Box("Insulator", (tankhside - outerwall) * cm, (tankhside - outerwall) * cm,
                  (tankhside - outerwall) * cm);
    auto fPuLogical  = new G4LogicalVolume(puSolid, puMat, "Pu_log");
    auto fPuPhysical = new G4PVPlacement(0, G4ThreeVector(), fPuLogical, "Pu_phys",
                                         fTankLogical, false, 0);

    //
    // Membrane
    //
    G4VSolid* membraneSolid =
        new G4Box("Membrane", (tankhside - outerwall - insulation) * cm,
                  (tankhside - outerwall - insulation) * cm,
                  (tankhside - outerwall - insulation) * cm);
    auto fMembraneLogical  = new G4LogicalVolume(membraneSolid, steelMat, "Membrane_log");
    auto fMembranePhysical = new G4PVPlacement(0, G4ThreeVector(), fMembraneLogical,
                                               "Membrane_phys", fPuLogical, false, 0);

    //
    // LAr
    //
    G4VSolid* larSolid    = new G4Box("LAr", larside * cm, larside * cm, larside * cm);
    auto      fLarLogical = new G4LogicalVolume(larSolid, larMat, "Lar_log");
    auto fLarPhysical     = new G4PVPlacement(0, G4ThreeVector(), fLarLogical, "Lar_phys",
                                          fMembraneLogical, false, 0);

    //
    // Copper tubes
    //
    G4VSolid* copperSolid =
        new G4Tubs("Copper", 0.0 * cm, curad * cm, cuhheight * cm, 0.0, CLHEP::twopi);
    auto fCopperLogical = new G4LogicalVolume(copperSolid, copperMat, "Copper_log");
    auto fCuPhysical_px =
        new G4PVPlacement(0, G4ThreeVector(ringrad * cm, 0., cushift * cm),
                          fCopperLogical, "Copper_phys1", fLarLogical, false, 0);
    auto fCuPhysical_py =
        new G4PVPlacement(0, G4ThreeVector(0., ringrad * cm, cushift * cm),
                          fCopperLogical, "Copper_phys2", fLarLogical, false, 0);
    auto fCuPhysical_mx =
        new G4PVPlacement(0, G4ThreeVector(-ringrad * cm, 0., cushift * cm),
                          fCopperLogical, "Copper_phys3", fLarLogical, false, 0);
    auto fCuPhysical_my =
        new G4PVPlacement(0, G4ThreeVector(0., -ringrad * cm, cushift * cm),
                          fCopperLogical, "Copper_phys4", fLarLogical, false, 0);

    //
    // ULAr bath
    //
    G4VSolid* ularSolid    = new G4Tubs("ULar", 0.0 * cm, (curad - copper) * cm,
                                     cuhheight * cm, 0.0, CLHEP::twopi);
    auto      fUlarLogical = new G4LogicalVolume(ularSolid, larMat, "ULar_log");
    auto      fUlarPhysical_px =
        new G4PVPlacement(0, G4ThreeVector(ringrad * cm, 0., cushift * cm), fUlarLogical,
                          "ULar_phys1", fCopperLogical, false, 0);
    auto fUlarPhysical_py =
        new G4PVPlacement(0, G4ThreeVector(0., ringrad * cm, cushift * cm), fUlarLogical,
                          "ULar_phys2", fCopperLogical, false, 0);
    auto fUlarPhysical_mx =
        new G4PVPlacement(0, G4ThreeVector(-ringrad * cm, 0., cushift * cm), fUlarLogical,
                          "ULar_phys3", fCopperLogical, false, 0);
    auto fUlarPhysical_my =
        new G4PVPlacement(0, G4ThreeVector(0., -ringrad * cm, cushift * cm), fUlarLogical,
                          "ULar_phys4", fCopperLogical, false, 0);

    //
    // Germanium
    //
    G4VSolid* geSolid    = new G4Tubs("ROI", 0.0 * cm, roiradius * cm, roihalfheight * cm,
                                   0.0, CLHEP::twopi);
    auto      fGeLogical = new G4LogicalVolume(geSolid, roiMat, "Ge_log");
    auto      fGePhysical_px =
        new G4PVPlacement(0, G4ThreeVector(ringrad * cm, 0., 0.), fGeLogical, "Ge_phys1",
                          fUlarLogical, false, 0);
    auto fGePhysical_py =
        new G4PVPlacement(0, G4ThreeVector(0., ringrad * cm, 0.), fGeLogical, "Ge_phys2",
                          fUlarLogical, false, 0);
    auto fGePhysical_mx =
        new G4PVPlacement(0, G4ThreeVector(-ringrad * cm, 0., 0.), fGeLogical, "Ge_phys3",
                          fUlarLogical, false, 0);
    auto fGePhysical_my =
        new G4PVPlacement(0, G4ThreeVector(0., -ringrad * cm, 0.), fGeLogical, "Ge_phys4",
                          fUlarLogical, false, 0);

    //
    // Visualization attributes
    //
    fWorldLogical->SetVisAttributes(G4VisAttributes::GetInvisible());

    auto redVisAtt = new G4VisAttributes(G4Colour::Red());
    redVisAtt->SetVisibility(true);
    auto greyVisAtt = new G4VisAttributes(G4Colour::Grey());
    greyVisAtt->SetVisibility(true);
    auto greenVisAtt = new G4VisAttributes(G4Colour::Green());
    greenVisAtt->SetVisibility(true);
    auto blueVisAtt = new G4VisAttributes(G4Colour::Blue());
    blueVisAtt->SetVisibility(true);

    fCavernLogical->SetVisAttributes(redVisAtt);
    fHallLogical->SetVisAttributes(greyVisAtt);
    fTankLogical->SetVisAttributes(blueVisAtt);
    fPuLogical->SetVisAttributes(greyVisAtt);
    fMembraneLogical->SetVisAttributes(blueVisAtt);
    fLarLogical->SetVisAttributes(greyVisAtt);
    fCopperLogical->SetVisAttributes(greenVisAtt);
    fUlarLogical->SetVisAttributes(greyVisAtt);
    fGeLogical->SetVisAttributes(redVisAtt);

    return fWorldPhysical;
}

G4VPhysicalVolume* WLGDDetectorConstruction::SetupBaseline()
{
    // Materials for this geometry
    G4Material* worldMaterial =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
    G4Material* larMat = G4NistManager::Instance()->FindOrBuildMaterial("G4_lAr");
    G4Material* concreteWallMat =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_CONCRETE");
    G4Material* airMat   = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
    G4Material* waterMat = G4NistManager::Instance()->FindOrBuildMaterial("G4_WATER");
    G4Material* steelMat =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_STAINLESS-STEEL");
    G4Material* copperMat = G4NistManager::Instance()->FindOrBuildMaterial("G4_Cu");

    // enriched Germanium from isotopes
    auto Ge_74 = new G4Isotope("Ge74", 32, 74, 74.0 * g / mole);
    auto Ge_76 = new G4Isotope("Ge76", 32, 76, 76.0 * g / mole);

    auto eGe = new G4Element("enriched Germanium", "enrGe", 2);
    eGe->AddIsotope(Ge_76, 88. * perCent);
    eGe->AddIsotope(Ge_74, 12. * perCent);

    G4double density = 3.323 * mg / cm3;
    auto     roiMat  = new G4Material("enrGe", density, 1);
    roiMat->AddElement(eGe, 1);

    // constants
    // size parameter, unit [cm]
    G4double offset = 200.0;  // shift cavern floor to keep detector centre at origin
    // cavern
    G4double stone       = 100.0;  // Hall wall thickness 1 m
    G4double hallrad     = 600.0;  // Hall cylinder diam 12 m
    G4double hallhheight = 850.0;  // Hall cylinder height 17 m
    // water tank
    G4double tankwalltop = 0.6;  // water tank thickness at top 6 mm
    G4double tankwallbot = 0.8;  // water tank thickness at bottom 8 mm
    G4double tankrad     = 550;  // water tank diam 11 m
    G4double tankhheight = 650;  // water tank height 13 m
    // cryostat
    G4double cryowall   = 3.0;    // cryostat wall thickness from GERDA
    G4double vacgap     = 1.0;    // vacuum gap between walls
    G4double cryrad     = 350.0;  // cryostat diam 7 m
    G4double cryhheight = 350.0;  // cryostat height 7 m
    // copper tubes with Germanium ROI
    G4double copper    = 0.35;   // tube thickness 3.5 mm
    G4double curad     = 40.0;   // copper tube diam 80 cm
    G4double cuhheight = 200.0;  // copper tube height 4 m inside cryostat
    G4double cushift   = 150.0;  // shift cu tube inside cryostat to top
    G4double ringrad   = 100.0;  // cu tube placement ring radius
    // Ge cylinder for 250 kg at 5.32 g/cm3
    G4double roiradius     = 25.0;   // detector region diam 50 cm
    G4double roihalfheight = 11.97;  // detector region height 24 cm

    fvertexZ = hallhheight + stone + offset;
    fmaxrad  = hallrad + stone;

    // Volumes for this geometry

    //
    // World
    //
    G4VSolid* worldSolid =
        new G4Tubs("World", 0.0 * cm, (hallrad + stone + 0.1) * cm,
                   (hallhheight + stone + offset + 0.1) * cm, 0.0, CLHEP::twopi);
    auto fWorldLogical = new G4LogicalVolume(worldSolid, worldMaterial, "World_log");
    auto fWorldPhysical =
        new G4PVPlacement(0, G4ThreeVector(), fWorldLogical, "World_phys", 0, false, 0);

    //
    // Cavern
    //
    G4VSolid* cavernSolid = new G4Tubs("Cavern", 0.0 * cm, (hallrad + stone) * cm,
                                       (hallhheight + stone) * cm, 0.0, CLHEP::twopi);
    auto fCavernLogical = new G4LogicalVolume(cavernSolid, concreteWallMat, "Cavern_log");
    auto fCavernPhysical =
        new G4PVPlacement(0, G4ThreeVector(0., 0., offset * cm), fCavernLogical,
                          "Cavern_phys", fWorldLogical, false, 0);

    //
    // Hall
    //
    G4VSolid* hallSolid =
        new G4Tubs("Hall", 0.0 * cm, hallrad * cm, hallhheight * cm, 0.0, CLHEP::twopi);
    auto fHallLogical = new G4LogicalVolume(hallSolid, airMat, "Hall_log");
    auto fHallPhysical =
        new G4PVPlacement(0, G4ThreeVector(0., 0., offset * cm), fHallLogical,
                          "Hall_phys", fCavernLogical, false, 0);

    //
    // Tank
    //
    G4VSolid* tankSolid =
        new G4Cons("Tank", 0.0 * cm, (tankrad + tankwallbot) * cm, 0.0 * cm,
                   (tankrad + tankwalltop) * cm, tankhheight * cm, 0.0, CLHEP::twopi);
    auto fTankLogical  = new G4LogicalVolume(tankSolid, steelMat, "Tank_log");
    auto fTankPhysical = new G4PVPlacement(0, G4ThreeVector(), fTankLogical, "Tank_phys",
                                           fHallLogical, false, 0);

    //
    // Water
    //
    G4VSolid* waterSolid =
        new G4Tubs("Water", 0.0 * cm, tankrad * cm, (tankhheight - tankwallbot) * cm, 0.0,
                   CLHEP::twopi);
    auto fWaterLogical  = new G4LogicalVolume(waterSolid, waterMat, "Water_log");
    auto fWaterPhysical = new G4PVPlacement(0, G4ThreeVector(), fWaterLogical,
                                            "Water_phys", fTankLogical, false, 0);

    //
    // outer cryostat
    //
    G4VSolid* coutSolid =
        new G4Tubs("Cout", 0.0 * cm, cryrad * cm, cryhheight * cm, 0.0, CLHEP::twopi);
    auto fCoutLogical  = new G4LogicalVolume(coutSolid, steelMat, "Cout_log");
    auto fCoutPhysical = new G4PVPlacement(0, G4ThreeVector(), fCoutLogical, "Cout_phys",
                                           fWaterLogical, false, 0);

    //
    // vacuum gap
    //
    G4VSolid* cvacSolid    = new G4Tubs("Cvac", 0.0 * cm, (cryrad - cryowall) * cm,
                                     cryhheight * cm, 0.0, CLHEP::twopi);
    auto      fCvacLogical = new G4LogicalVolume(cvacSolid, worldMaterial, "Cvac_log");
    auto fCvacPhysical = new G4PVPlacement(0, G4ThreeVector(), fCvacLogical, "Cvac_phys",
                                           fCoutLogical, false, 0);

    //
    // inner cryostat
    //
    G4VSolid* cinnSolid = new G4Tubs("Cinn", 0.0 * cm, (cryrad - cryowall - vacgap) * cm,
                                     cryhheight * cm, 0.0, CLHEP::twopi);
    auto      fCinnLogical = new G4LogicalVolume(cinnSolid, steelMat, "Cinn_log");
    auto fCinnPhysical = new G4PVPlacement(0, G4ThreeVector(), fCinnLogical, "Cinn_phys",
                                           fCvacLogical, false, 0);

    //
    // LAr bath
    //
    G4VSolid* larSolid =
        new G4Tubs("LAr", 0.0 * cm, (cryrad - 2 * cryowall - vacgap) * cm,
                   cryhheight * cm, 0.0, CLHEP::twopi);
    auto fLarLogical  = new G4LogicalVolume(larSolid, larMat, "Lar_log");
    auto fLarPhysical = new G4PVPlacement(0, G4ThreeVector(), fLarLogical, "Lar_phys",
                                          fCinnLogical, false, 0);

    //
    // cryostat Lid
    //
    G4VSolid* lidSolid =
        new G4Tubs("Lid", 0.0 * cm, cryrad * cm, cryowall / 2.0 * cm, 0.0, CLHEP::twopi);
    auto fLidLogical = new G4LogicalVolume(lidSolid, steelMat, "Lid_log");
    auto fLidPhysical =
        new G4PVPlacement(0, G4ThreeVector(0., 0., (cryhheight + cryowall / 2.0) * cm),
                          fLidLogical, "Lid_phys", fWaterLogical, false, 0);
    auto fBotLogical = new G4LogicalVolume(lidSolid, steelMat, "Bot_log");
    auto fBotPhysical =
        new G4PVPlacement(0, G4ThreeVector(0., 0., -(cryhheight + cryowall / 2.0) * cm),
                          fBotLogical, "Bot_phys", fWaterLogical, false, 0);

    //
    // copper tubes
    //
    G4VSolid* copperSolid =
        new G4Tubs("Copper", 0.0 * cm, curad * cm, cuhheight * cm, 0.0, CLHEP::twopi);
    auto fCopperLogical = new G4LogicalVolume(copperSolid, copperMat, "Copper_log");
    auto fCuPhysical_px =
        new G4PVPlacement(0, G4ThreeVector(ringrad * cm, 0., cushift * cm),
                          fCopperLogical, "Copper_phys1", fLarLogical, false, 0);
    auto fCuPhysical_py =
        new G4PVPlacement(0, G4ThreeVector(0., ringrad * cm, cushift * cm),
                          fCopperLogical, "Copper_phys2", fLarLogical, false, 0);
    auto fCuPhysical_mx =
        new G4PVPlacement(0, G4ThreeVector(-ringrad * cm, 0., cushift * cm),
                          fCopperLogical, "Copper_phys3", fLarLogical, false, 0);
    auto fCuPhysical_my =
        new G4PVPlacement(0, G4ThreeVector(0., -ringrad * cm, cushift * cm),
                          fCopperLogical, "Copper_phys4", fLarLogical, false, 0);

    //
    // ULAr bath
    //
    G4VSolid* ularSolid    = new G4Tubs("ULar", 0.0 * cm, (curad - copper) * cm,
                                     cuhheight * cm, 0.0, CLHEP::twopi);
    auto      fUlarLogical = new G4LogicalVolume(ularSolid, larMat, "ULar_log");
    auto      fUlarPhysical_px =
        new G4PVPlacement(0, G4ThreeVector(ringrad * cm, 0., cushift * cm), fUlarLogical,
                          "ULar_phys1", fCopperLogical, false, 0);
    auto fUlarPhysical_py =
        new G4PVPlacement(0, G4ThreeVector(0., ringrad * cm, cushift * cm), fUlarLogical,
                          "ULar_phys2", fCopperLogical, false, 0);
    auto fUlarPhysical_mx =
        new G4PVPlacement(0, G4ThreeVector(-ringrad * cm, 0., cushift * cm), fUlarLogical,
                          "ULar_phys3", fCopperLogical, false, 0);
    auto fUlarPhysical_my =
        new G4PVPlacement(0, G4ThreeVector(0., -ringrad * cm, cushift * cm), fUlarLogical,
                          "ULar_phys4", fCopperLogical, false, 0);

    //
    // Germanium
    //
    G4VSolid* geSolid    = new G4Tubs("ROI", 0.0 * cm, roiradius * cm, roihalfheight * cm,
                                   0.0, CLHEP::twopi);
    auto      fGeLogical = new G4LogicalVolume(geSolid, roiMat, "Ge_log");
    auto      fGePhysical_px =
        new G4PVPlacement(0, G4ThreeVector(ringrad * cm, 0., 0.), fGeLogical, "Ge_phys1",
                          fUlarLogical, false, 0);
    auto fGePhysical_py =
        new G4PVPlacement(0, G4ThreeVector(0., ringrad * cm, 0.), fGeLogical, "Ge_phys2",
                          fUlarLogical, false, 0);
    auto fGePhysical_mx =
        new G4PVPlacement(0, G4ThreeVector(-ringrad * cm, 0., 0.), fGeLogical, "Ge_phys3",
                          fUlarLogical, false, 0);
    auto fGePhysical_my =
        new G4PVPlacement(0, G4ThreeVector(0., -ringrad * cm, 0.), fGeLogical, "Ge_phys4",
                          fUlarLogical, false, 0);

    //
    // Visualization attributes
    //
    fWorldLogical->SetVisAttributes(G4VisAttributes::GetInvisible());

    auto redVisAtt = new G4VisAttributes(G4Colour::Red());
    redVisAtt->SetVisibility(true);
    auto greyVisAtt = new G4VisAttributes(G4Colour::Grey());
    greyVisAtt->SetVisibility(true);
    auto greenVisAtt = new G4VisAttributes(G4Colour::Green());
    greenVisAtt->SetVisibility(true);
    auto blueVisAtt = new G4VisAttributes(G4Colour::Blue());
    blueVisAtt->SetVisibility(true);

    fCavernLogical->SetVisAttributes(redVisAtt);
    fHallLogical->SetVisAttributes(greyVisAtt);
    fTankLogical->SetVisAttributes(greenVisAtt);
    fWaterLogical->SetVisAttributes(greyVisAtt);
    fLarLogical->SetVisAttributes(greyVisAtt);
    fCoutLogical->SetVisAttributes(blueVisAtt);
    fCvacLogical->SetVisAttributes(greyVisAtt);
    fCinnLogical->SetVisAttributes(blueVisAtt);
    fLidLogical->SetVisAttributes(blueVisAtt);
    fBotLogical->SetVisAttributes(blueVisAtt);
    fCopperLogical->SetVisAttributes(greenVisAtt);
    fUlarLogical->SetVisAttributes(greyVisAtt);
    fGeLogical->SetVisAttributes(redVisAtt);

    return fWorldPhysical;
}

void WLGDDetectorConstruction::DefineCommands()
{
    // Define geometry command directory using generic messenger class
    fDetectorMessenger =
        new G4GenericMessenger(this, "/WLGD/detector/",
                               "Detector baseline (true) or alternative (false)");

    // switch command
    auto& switchCmd = fDetectorMessenger->DeclareProperty("baseline", fBaseline);
    switchCmd.SetGuidance("Set baseline geometry (true) or alternative (false).");
    switchCmd.SetParameterName("baseline", true);
    switchCmd.SetDefaultValue("true");
}
