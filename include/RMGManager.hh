// Copyright (C) 2022 Luigi Pertoldi <gipert@pm.me>
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
// details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _RMG_MANAGER_HH_
#define _RMG_MANAGER_HH_

#include <atomic>
#include <memory>
#include <vector>

#include "G4RunManager.hh"
#include "G4RunManagerFactory.hh"
#include "G4Threading.hh"
#include "G4VisManager.hh"
#include "globals.hh"

#include "RMGExceptionHandler.hh"
#include "RMGLog.hh"
#include "RMGUserInit.hh"

class G4VUserPhysicsList;
class RMGHardware;
class RMGUserAction;
class G4GenericMessenger;
class G4UIExecutive;
class RMGManager {

  public:

    RMGManager() = delete;
    RMGManager(std::string app_name, int argc, char** argv);
    ~RMGManager() = default;

    RMGManager(RMGManager const&) = delete;
    RMGManager& operator=(RMGManager const&) = delete;
    RMGManager(RMGManager&&) = delete;
    RMGManager& operator=(RMGManager&&) = delete;

    // getters
    static inline RMGManager* Instance() { return fRMGManager; }
    G4RunManager* GetG4RunManager();
    G4VisManager* GetG4VisManager();
    RMGHardware* GetDetectorConstruction();
    G4VUserPhysicsList* GetProcessesList();
    [[nodiscard]] inline auto GetUserInit() const { return fUserInit; }
    [[nodiscard]] inline int GetPrintModulo() const { return fPrintModulo; }

    inline bool IsExecSequential() {
      return fG4RunManager->GetRunManagerType() == G4RunManager::RMType::sequentialRM;
    }
    [[nodiscard]] inline bool IsPersistencyEnabled() const { return fIsPersistencyEnabled; }
    inline const std::string& GetOutputFileName() { return fOutputFile; }
    [[nodiscard]] inline bool GetOutputOverwriteFiles() const { return fOutputOverwriteFiles; }
    inline const std::string& GetOutputNtupleDirectory() { return fOutputNtupleDirectory; }
    [[nodiscard]] inline bool GetOutputNtuplePerDetector() const {
      return fOutputNtuplePerDetector;
    }
    [[nodiscard]] inline bool GetOutputNtupleUseVolumeName() const {
      return fOutputNtupleUseVolumeName;
    }

    // setters
    inline void SetUserInit(G4RunManager* g4_manager) {
      fG4RunManager = std::unique_ptr<G4RunManager>(g4_manager);
    }
    inline void SetUserInit(G4VisManager* vis) {
      fG4VisManager = std::unique_ptr<G4VisManager>(vis);
    }
    inline void SetUserInit(RMGHardware* det) { fDetectorConstruction = det; }
    inline void SetUserInit(G4VUserPhysicsList* proc) { fPhysicsList = proc; }

    inline void SetInteractive(bool flag = true) { fInteractive = flag; }
    inline void SetNumberOfThreads(int nthreads) { fNThreads = nthreads; }
    inline void SetPrintModulo(int n_ev) { fPrintModulo = n_ev > 0 ? n_ev : -1; }

    inline void EnablePersistency(bool flag = true) { fIsPersistencyEnabled = flag; }
    inline void IncludeMacroFile(std::string filename) { fMacroFileNames.emplace_back(filename); }
    void Initialize();
    void Run();

    void SetRandEngine(std::string name);
    void SetRandEngineSeed(long seed);
    void SetRandEngineInternalSeed(int index);
    void SetRandSystemEntropySeed();
    bool ApplyRandEngineForCurrentThread();
    [[nodiscard]] inline bool GetRandIsControlled() const { return fIsRandControlled; }
    [[nodiscard]] inline bool GetRandEngineSelected() const { return !fRandEngineName.empty(); }

    void SetLogLevel(std::string level);

    inline void SetOutputFileName(std::string filename) { fOutputFile = filename; }
    inline void SetOutputOverwriteFiles(bool overwrite) { fOutputOverwriteFiles = overwrite; }
    inline void SetOutputNtupleDirectory(std::string dir) { fOutputNtupleDirectory = dir; }
    inline int RegisterNtuple(int det_uid, int ntuple_id) {
      auto res = fNtupleIDs.emplace(det_uid, ntuple_id);
      if (!res.second)
        RMGLog::OutFormatDev(RMGLog::fatal, "Ntuple for detector with UID {} is already registered",
            det_uid);
      return this->GetNtupleID(det_uid);
    }
    inline int GetNtupleID(int det_uid) { return fNtupleIDs[det_uid]; }

    [[nodiscard]] inline bool HadWarning() const {
      return fExceptionHandler->HadWarning() || RMGLog::HadWarning();
    }
    [[nodiscard]] inline bool HadError() const {
      return fExceptionHandler->HadError() || RMGLog::HadError();
    }

    // NOLINTNEXTLINE(readability-make-member-function-const)
    inline void ActivateOptionalOutputScheme(std::string name) {
      GetUserInit()->ActivateOptionalOutputScheme(name);
    }

    inline static void AbortRunGracefully() {
      if (!fAbortRun.is_lock_free()) return;
      fAbortRun = true;
    }
    inline static bool ShouldAbortRun() { return fAbortRun; }

  private:

    void SetUpDefaultG4RunManager(G4RunManagerType type = G4RunManagerType::Default);
    void SetUpDefaultG4VisManager();
    void SetUpDefaultDetectorConstruction();
    void SetUpDefaultProcessesList();
    void SetUpDefaultUserAction();
    void CheckRandEngineMTState();

    std::unique_ptr<G4UIExecutive> StartInteractiveSession();

    std::string fApplicationName;
    int fArgc;
    char** fArgv;
    std::vector<std::string> fMacroFileNames;
    bool fInteractive = false;
    bool fIsPersistencyEnabled = true;
    int fPrintModulo = -1;
    int fNThreads = 1;

    bool fIsRandControlled = false;
    bool fIsRandControlledAtEngineChange = false;
    std::string fRandEngineName;

    std::string fOutputFile = "detector-hits.root";
    bool fOutputOverwriteFiles = false;
    bool fOutputNtuplePerDetector = true;
    bool fOutputNtupleUseVolumeName = false;
    std::string fOutputNtupleDirectory = "stp";
    // track internal id of detector NTuples
    static G4ThreadLocal std::map<int, int> fNtupleIDs;


    static RMGManager* fRMGManager;
    static std::atomic<bool> fAbortRun;

    std::unique_ptr<G4RunManager> fG4RunManager = nullptr;
    std::unique_ptr<G4VisManager> fG4VisManager = nullptr;

    RMGExceptionHandler* fExceptionHandler = nullptr;
    G4VUserPhysicsList* fPhysicsList = nullptr;
    RMGHardware* fDetectorConstruction = nullptr;

    RMGUserAction* fUserAction = nullptr;
    // store partial custom actions to be used later in RMGUserAction
    std::shared_ptr<RMGUserInit> fUserInit;

    // messenger stuff
    std::unique_ptr<G4GenericMessenger> fMessenger;
    std::unique_ptr<G4GenericMessenger> fLogMessenger;
    std::unique_ptr<G4GenericMessenger> fRandMessenger;
    std::unique_ptr<G4GenericMessenger> fOutputMessenger;
    void DefineCommands();
};

#endif

// vim: tabstop=2 shiftwidth=2 expandtab
