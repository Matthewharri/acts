// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Experimental/Muon_Detector.hpp"
#include "Acts/Geometry/AbstractVolume.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Geometry/Portal.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Utilities/BoundingBox.hpp"

#include "Acts/Geometry/Volume.hpp"

#include "Acts/NavigatorDelegates/MuonDelegate.hpp"
#include "Acts/Geometry/NavigationState.hpp"


/// @brief main executable
///
/// @param argc The argument count
/// @param argv The argument list
int main(int argc, char* argv[]) {
  // --------------------------------------------------------------------------------
  MuonDetector detector;
  Acts::ObjVisualization3D helper;
  auto gctx = Acts::GeometryContext();
  //Build MS
  auto MS = detector.build_detector("/Users/maharris/acts_muon/muon_volume/muon_vols.json");

  //Visualize MS
  detector.visualize_detector(helper, MS, gctx);
  helper.write("muon_vols.obj");
  helper.clear();
  auto box = (*MS).getBoundingBox();
  (box)->draw(helper);
  helper.write("TEST_muon_box.obj");
  helper.clear();

  auto vols = MS->volumes();
  for (auto vol : vols) {
    auto new_box = (*vol).getBoundingBox();
    (new_box)->draw(helper);
  }
  helper.write("INSIDE_BB.obj");
  helper.clear();

  NavigationState navState;
  navState.currentVolume = &(*MS);
  // for(auto i = 0; i < 9000; i++){
  // navState.position = Acts::Vector3(i,i-0.5,i-0.75);
  navState.position = Acts::Vector3(0,0,0);
  navState.direction = Acts::Vector3(2,2,6).normalized();
  auto test = MuonImpl(*MS);
  test.update(gctx, navState);
  // }
  
 


  return 0;
}
















