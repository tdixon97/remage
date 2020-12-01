#include "RMGManagerMessenger.hh"

#include <fstream>

#include "globals.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "Randomize.hh"

#include "RMGManager.hh"
#include "RMGManagementRunAction.hh"
#include "RMGLog.hh"

RMGManagerMessenger::RMGManagerMessenger(RMGManager*) {

  fDirectory = new G4UIdirectory("/RMG/manager/");
  fDirectory->SetGuidance("Management controls.");

  fRMGLogCmd = new G4UIcmdWithAString("/RMG/manager/mglog", this);
  fRMGLogCmd->SetGuidance("Set severity of logs to report to stdout.");
  fRMGLogCmd->SetGuidance("Options, in ascending order of severity, are:");
  fRMGLogCmd->SetGuidance("debug: Displays all logs ");
  fRMGLogCmd->SetGuidance("detail: All logs, except debugging(default)");
  fRMGLogCmd->SetGuidance("summary: All logs, except debugging and trace");
  fRMGLogCmd->SetGuidance("warning: All logs, except trace, debugging and routine:");
  fRMGLogCmd->SetGuidance("error: Only error and fatal logs.");
  fRMGLogCmd->SetGuidance("fatal: Only fatal logs.");
  fRMGLogCmd->SetCandidates("debug detail summary warning wrror fatal");

  fHEPRandomSeedCmd = new G4UIcmdWithAnInteger("/RMG/manager/heprandomseed", this);
  fHEPRandomSeedCmd->SetGuidance("Sets random number generator seed.");

  fUseInternalSeedCmd = new G4UIcmdWithAnInteger("/RMG/manager/useInternalSeed", this);
  fUseInternalSeedCmd->SetGuidance("Sets random number generator seed using internal seed table.");
  fUseInternalSeedCmd->SetGuidance("Provide the index of the seed you would like to use (0-429)");
  fUseInternalSeedCmd->SetParameterName("index", false);
  fUseInternalSeedCmd->SetRange("index>=0 && index<430");

  fSeedWithDevRandomCmd = new G4UIcmdWithoutParameter("/RMG/manager/seedWithDevRandom", this);
  fSeedWithDevRandomCmd->SetGuidance("Seed random number generator with a \"truly\" random number");
  fSeedWithDevRandomCmd->SetGuidance("generated by a read from /dev/random");

  fUseRandomEngineCmd = new G4UIcmdWithAString("/RMG/manager/useRandomEngine", this);
  fUseRandomEngineCmd->SetGuidance("Sets type of random engine to use.");
  fUseRandomEngineCmd->SetCandidates("JamesRandom RanLux MTwist");
}

RMGManagerMessenger::~RMGManagerMessenger() {
  delete fDirectory;
  delete fRMGLogCmd;
  delete fUseRandomEngineCmd;
  delete fHEPRandomSeedCmd;
  delete fUseInternalSeedCmd;
  delete fSeedWithDevRandomCmd;
}

void RMGManagerMessenger::SetNewValue(G4UIcommand* cmd, G4String new_values) {

  if (cmd == fRMGLogCmd) {

    if      (new_values == "debug"  ) RMGLog::SetLogLevelScreen(RMGLog::debug);
    else if (new_values == "detail" ) RMGLog::SetLogLevelScreen(RMGLog::detail);
    else if (new_values == "summary") RMGLog::SetLogLevelScreen(RMGLog::summary);
    else if (new_values == "warning") RMGLog::SetLogLevelScreen(RMGLog::warning);
    else if (new_values == "error"  ) RMGLog::SetLogLevelScreen(RMGLog::error);
    else if (new_values == "fatal"  ) RMGLog::SetLogLevelScreen(RMGLog::fatal);
    else RMGLog::Out(RMGLog::error, "Unknown option.");
  }
  else if (cmd == fHEPRandomSeedCmd) {

    G4long seed = std::stol(new_values);

    if (seed >= std::numeric_limits<long>::max()) {
      RMGLog::Out(RMGLog::error, "Seed ", new_values, " is too large. Largest possible seed is ",
          std::numeric_limits<long>::max(), ". Setting seed to 0.");
      CLHEP::HepRandom::setTheSeed(0);
    }
    else CLHEP::HepRandom::setTheSeed(seed);
    RMGLog::Out(RMGLog::summary, "CLHEP::HepRandom seed set to: ", seed);
    RMGManager::GetRMGManager()->GetRMGRunAction()->SetControlledRandomization();
  }
  else if (cmd == fUseInternalSeedCmd) {

    auto index = fUseInternalSeedCmd->GetNewIntValue(new_values);

    long seeds[2];
    int table_index = index/2;
    CLHEP::HepRandom::getTheTableSeeds(seeds, table_index);

    int array_index = index % 2;
    CLHEP::HepRandom::setTheSeed(seeds[array_index]);
    RMGLog::Out(RMGLog::summary, "CLHEP::HepRandom seed set to: ", seeds[array_index]);
    RMGManager::GetRMGManager()->GetRMGRunAction()->SetControlledRandomization();
  }
  else if (cmd == fSeedWithDevRandomCmd) {

    std::ifstream devrandom("/dev/random");
    if (devrandom.bad()) {
      RMGLog::Out(RMGLog::warning, "Couldn't open /dev/random. Setting seed to 0.");
      CLHEP::HepRandom::setTheSeed(0);
    }
    else {
      RMGLog::Out(RMGLog::debug, "Reading a seed from /dev/random...");
      long seed;
      devrandom.read((char*)(&seed), sizeof(long));

      // Negative seeds give nasty sequences for some engines. For example,
      // CLHEP's JamesRandom.cc contains a specific check for this. Might
      // as well make all seeds positive; randomness is not affected (one
      // bit of randomness goes unused).
      if (seed < 0) seed = -seed;

      CLHEP::HepRandom::setTheSeed(seed);
      RMGLog::Out(RMGLog::summary, "CLHEP::HepRandom seed set to: ", seed);
      RMGManager::GetRMGManager()->GetRMGRunAction()->SetControlledRandomization();
    }
    devrandom.close();
  }
  else if (cmd == fUseRandomEngineCmd) {

    if (new_values == "JamesRandom") {
      CLHEP::HepRandom::setTheEngine(new CLHEP::HepJamesRandom);
      RMGLog::Out(RMGLog::summary, "Using James engine.");
    }
    else if (new_values == "RanLux") {
      CLHEP::HepRandom::setTheEngine(new CLHEP::RanluxEngine);
      RMGLog::Out(RMGLog::summary, "Using RanLux engine.");
    }
    else if (new_values == "MTwist") {
      CLHEP::HepRandom::setTheEngine(new CLHEP::MTwistEngine);
      RMGLog::Out(RMGLog::summary, "Using MTwist engine.");
    }
    else {
      CLHEP::HepRandom::setTheEngine(new CLHEP::HepJamesRandom);
      RMGLog::Out(RMGLog::error, new_values, " engine unknown, setting default engine.");
      RMGLog::Out(RMGLog::summary, "Using James engine.");
    }
  }
}

// vim: tabstop=2 shiftwidth=2 expandtab
