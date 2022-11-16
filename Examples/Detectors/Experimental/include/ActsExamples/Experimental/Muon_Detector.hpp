// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/DetectorVolume.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"

#include <memory>
#include <vector>

using namespace Acts::Experimental;
// using BoundingBox = Acts::AxisAlignedBoundingBox<Acts::Experimental::DetectorVolume, Acts::ActsScalar, 3>;

struct MuonDetector {

  std::shared_ptr<DetectorVolume> build_detector(std::string json_file);

  void visualize_detector(Acts::IVisualization3D& helper, std::shared_ptr<DetectorVolume> detector, Acts::GeometryContext tContext);

  // std::vector<std::shared_ptr<BoundingBox>> build_bounding_boxes(std::shared_ptr<DetectorVolume> detector);

  // std::tuple<std::vector<std::unique_ptr<BoundingBox>>, std::shared_ptr<BoundingBox>> build_octree(std::vector<BoundingBox*> boxes);
};
