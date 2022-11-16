// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/AbstractVolume.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/DetectorVolume.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Geometry/detail/DetectorVolumeUpdators.hpp"
#include "Acts/Geometry/detail/PortalGenerators.hpp"
#include "Acts/Geometry/detail/PortalHelper.hpp"
#include "Acts/Geometry/detail/SurfaceCandidatesUpdators.hpp"
#include "Acts/Plugins/Json/VolumeBoundsJsonConverter.hpp"
#include "Acts/Utilities/BoundingBox.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "ActsExamples/Experimental/Muon_Detector.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"
#include "nlohmann/json.hpp"

#include "Acts/NavigatorDelegates/MuonDelegate.hpp"


#include <fstream>
#include <iostream>

using namespace Acts::Experimental;
// std::tuple<Acts::Vector3, Acts::Vector3> get_vmin_vmax(const DetectorVolume& detector) {
//     auto gctx = Acts::GeometryContext();
//     auto surfaces = detector.portals();
//     std::vector<Acts::Vector3> vertices;

//     for (auto s : surfaces) {
//         auto surface = s->surface().polyhedronRepresentation(gctx, 1);
//         auto vertex = surface.vertices;
//         for(auto v : vertex){
//             vertices.push_back(v);
//         }
//     }

//     Acts::Vector3 vmin(1e9, 1e9, 1e9);
//     Acts::Vector3 vmax(-1e9, -1e9, -1e9);
//     for(auto v : vertices){
//         vmin = vmin.cwiseMin(v);
//         vmax = vmax.cwiseMax(v);
//     }
//     return std::make_tuple(vmin, vmax);
// }

// std::vector<std::shared_ptr<BoundingBox>> get_bb(std::vector<std::shared_ptr<BoundingBox>> boxes, const DetectorVolume& detector) {
//     auto min_max = get_vmin_vmax(detector);
//     Acts::Vector3 vmin = std::get<0>(min_max);
//     Acts::Vector3 vmax = std::get<1>(min_max);

//     auto box = BoundingBox(&(detector), vmin, vmax);
//     auto shared_box = std::make_shared<BoundingBox>(box);
//     boxes.push_back(shared_box);

//     auto sub_volumes = detector.volumes();
//     for(auto s_vol: sub_volumes){
//         auto sub_sub_volumes = s_vol->volumes();
//         if(sub_sub_volumes.size() != 0){
//             auto not_needed = get_bb(boxes, *s_vol);
//         } else {
//             auto sub_min_max = get_vmin_vmax(*s_vol);
//             Acts::Vector3 sub_vmin = std::get<0>(sub_min_max);
//             Acts::Vector3 sub_vmax = std::get<1>(sub_min_max);
//             auto sub_box = BoundingBox(&(*s_vol), sub_vmin, sub_vmax);
//             auto sub_shared_box = std::make_shared<BoundingBox>(sub_box);
//             boxes.push_back(sub_shared_box);
//         }
//     }
//     return boxes;
// }
    

std::shared_ptr<DetectorVolume> MuonDetector::build_detector(std::string json_file){
    Acts::GeometryContext tContext;
    nlohmann::json muon_vols;

    auto muon_vols_json = std::ifstream(json_file,
								std::ifstream::in | std::ifstream::binary);
    muon_vols_json >> muon_vols;
    muon_vols_json.close();
    
    std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> detector_stations;
    std::vector<std::shared_ptr<BoundingBox>> boxes;
    std::vector<std::shared_ptr<Acts::Surface>> surfaces = {};

    auto portalGenerator = detail::defaultPortalGenerator();
    auto portalsAndSubPortalsGenerator = detail::defaultPortalAndSubPortalGenerator();


    // auto test_file = std::ifstream("/Users/maharris/acts_muon/run/CuboidVolumeBounds.json",
    //                             std::ifstream::in | std::ifstream::binary);
    
    for(size_t i = 0; i < muon_vols.size(); i++){
        auto transform = Acts::Transform3::Identity();
        auto bounds = muon_vols[i]["values"];
        auto rotation = muon_vols[i]["rotation"];
        auto translation = muon_vols[i]["translation"];

        if (translation.is_null()) {
            transform.translation() << Acts::Vector3(0, 0, 0);
        } else {
            transform.translation()
                << Acts::Vector3(translation[0], translation[1], translation[2]);
        }
        if (rotation.is_null()) {
            transform.rotate(Acts::RotationMatrix3::Identity());
        } else {
            Acts::RotationMatrix3 rot;
            rot << rotation[0], rotation[1], rotation[2], rotation[3], rotation[4],
                rotation[5], rotation[6], rotation[7], rotation[8];
            transform.rotate(rot);
        }

        if(muon_vols[i]["type"] == "Cuboid"){
            auto vol = Acts::volumeBoundsFromJson<Acts::CuboidVolumeBounds>(muon_vols[i]);
            auto d_vol = DetectorVolumeFactory::construct(portalGenerator, tContext,
            "MS_detector_vol+"+std::to_string(i), transform,
            std::move(vol), detail::allPortals());
            detector_stations.push_back(d_vol);
        } else if(muon_vols[i]["type"] == "Cylinder"){
            auto vol = Acts::volumeBoundsFromJson<Acts::CylinderVolumeBounds>(muon_vols[i]);
            auto d_vol = DetectorVolumeFactory::construct(portalGenerator, tContext,
            "MS_detector_vol+"+std::to_string(i), transform,
            std::move(vol), detail::allPortals());
            detector_stations.push_back(d_vol);
        } else if(muon_vols[i]["type"] == "Trapezoid"){
            auto vol = Acts::volumeBoundsFromJson<Acts::TrapezoidVolumeBounds>(muon_vols[i]);
            auto d_vol = DetectorVolumeFactory::construct(portalGenerator, tContext,
            "MS_detector_vol+"+std::to_string(i), transform,
            std::move(vol), detail::allPortals());
            detector_stations.push_back(d_vol);
        } else{
            throw std::invalid_argument("Matt, did you forget about: " + std::string(muon_vols[i]["type"]));
        }
        
    }
    auto MS_envelope_bounds = std::make_unique<Acts::CutoutCylinderVolumeBounds>
												                (0, 4000, 14500, 22000, 3200);
    auto MS_envelope = DetectorVolumeFactory::construct(portalsAndSubPortalsGenerator, tContext, "Muon Envelope",
                Acts::Transform3::Identity(), std::move(MS_envelope_bounds), surfaces, detector_stations, detail::allPortals());
    return MS_envelope;
}

void MuonDetector::visualize_detector(Acts::IVisualization3D& helper, std::shared_ptr<DetectorVolume> detector, Acts::GeometryContext tContext){
    Acts::GeometryView3D::drawDetectorVolume(helper, *detector, tContext);
}     

// std::vector<std::shared_ptr<BoundingBox>> MuonDetector::build_bounding_boxes(std::shared_ptr<DetectorVolume> detector){
//     std::vector<std::shared_ptr<BoundingBox>> boxes;
//     auto bounding_boxes = get_bb(boxes, *detector);
//     return bounding_boxes;
// }

// std::tuple<std::vector<std::unique_ptr<BoundingBox>>, std::shared_ptr<BoundingBox>> MuonDetector::build_octree(std::vector<BoundingBox*> boxes){
//     std::vector<std::unique_ptr<BoundingBox>> store;
//     BoundingBox* top_box = Acts::make_octree(store,boxes,5);
//     auto top_box_shared = std::make_shared<BoundingBox>(*top_box);
//     return std::make_tuple(std::move(store), top_box_shared);
// }
    