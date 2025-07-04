// Copyright (C) 2022 Luigi Pertoldi <https://orcid.org/0000-0002-0467-2571>
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

#ifndef _RMG_V_OUTPUT_SCHEME_HH_
#define _RMG_V_OUTPUT_SCHEME_HH_

#include <optional>
#include <string>

#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4Track.hh"
#include "G4UserStackingAction.hh"

#include "RMGDetectorMetadata.hh"

#include "fmt/format.h"

class G4Event;
class RMGVOutputScheme {

  public:

    RMGVOutputScheme() = default;
    virtual ~RMGVOutputScheme() = default;

    // initialization.
    virtual void AssignOutputNames(G4AnalysisManager*) {}

    // functions for individual events.
    virtual void ClearBeforeEvent() {}
    virtual bool ShouldDiscardEvent(const G4Event*) { return false; }
    [[nodiscard]] virtual bool StoreAlways() const { return false; }
    virtual void StoreEvent(const G4Event*) {}

    // hook into RMGStackingAction.
    virtual std::optional<G4ClassificationOfNewTrack> StackingActionClassify(const G4Track*, const int) {
      return std::nullopt;
    }
    virtual std::optional<bool> StackingActionNewStage(const int) { return std::nullopt; }

    // hook into G4TrackingAction.
    virtual void TrackingActionPre(const G4Track*) {};

    virtual void EndOfRunAction(const G4Run*) {};

    // only to be called by the manager, before calling AssignOutputNames.
    void SetNtuplePerDetector(bool ntuple_per_det) { fNtuplePerDetector = ntuple_per_det; }
    void SetNtupleUseVolumeName(bool use_vol_name) { fNtupleUseVolumeName = use_vol_name; }

  protected:

    [[nodiscard]] virtual std::string GetNtupleName(RMGDetectorMetadata det) const {
      if (fNtuplePerDetector) {
        if (!det.name.empty() && fNtupleUseVolumeName) { return det.name; }
        return fmt::format("det{:03}", det.uid);
      }
      return GetNtuplenameFlat();
    }
    [[nodiscard]] virtual std::string GetNtuplenameFlat() const {
      throw new std::logic_error("GetNtuplenameFlat not implemented");
    }

    // helper functions for output schemes.
    void CreateNtupleFOrDColumn(G4AnalysisManager* ana_man, int nt, std::string name, bool use_float) {
      if (use_float) ana_man->CreateNtupleFColumn(nt, name);
      else ana_man->CreateNtupleDColumn(nt, name);
    }
    void FillNtupleFOrDColumn(G4AnalysisManager* ana_man, int nt, int col, double val, bool use_float) {
      if (use_float)
        ana_man->FillNtupleFColumn(nt, col, val); // NOLINT(cppcoreguidelines-narrowing-conversions)
      else ana_man->FillNtupleDColumn(nt, col, val);
    }

    // global options injected by manager.
    bool fNtuplePerDetector = true;
    bool fNtupleUseVolumeName = false;
};

#endif

// vim: tabstop=2 shiftwidth=2 expandtab
