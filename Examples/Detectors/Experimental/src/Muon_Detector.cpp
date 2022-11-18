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
#include "Acts/Surfaces/StrawSurface.hpp"


#include <fstream>
#include <iostream>

using namespace Acts::Experimental;

std::shared_ptr<DetectorVolume> MuonDetector::build_detector(std::string json_file){
    Acts::GeometryContext tContext;
    Acts::ObjVisualization3D helper;

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


    auto test_file = std::ifstream("/Users/maharris/acts_muon/run/CuboidVolumeBounds.json",
                                std::ifstream::in | std::ifstream::binary);

    auto radius_straw = 10;
    auto half_length_straw = 2100;
    // auto translation_straw = Acts::Vector3(0.0, 3109.0, 10130.0);

    std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>> detector_straws;
    for(auto i = 0; i < 1700; i = i+20){
        auto rotation_straw = Acts::RotationMatrix3::Identity();
        auto transform_straw = Acts::Transform3::Identity();
        transform_straw.translate(Acts::Vector3(0.0, 3109.0+i, 10130.0));
        transform_straw.rotate(rotation_straw);
        auto rotation_straw1 = Acts::RotationMatrix3::Identity();
        auto transform_straw1 = Acts::Transform3::Identity();
        transform_straw1.translate(Acts::Vector3(20, 3109.0+i, 10130.0));
        transform_straw1.rotate(rotation_straw1);
        auto surface = Acts::CylinderVolumeBounds(0, radius_straw, half_length_straw);
        auto surface1 = Acts::CylinderVolumeBounds(0, radius_straw, half_length_straw);
        auto surface_unique = std::make_unique<Acts::CylinderVolumeBounds>(surface);
        auto surface_unique1 = std::make_unique<Acts::CylinderVolumeBounds>(surface1);
        auto detector_straw = DetectorVolumeFactory::construct(portalGenerator, tContext,
            "MS_detector_vol+"+std::to_string(i), transform_straw,
            std::move(surface_unique), detail::allPortals());
        auto detector_straw1 = DetectorVolumeFactory::construct(portalGenerator, tContext,
            "MS_detector_vol1+"+std::to_string(i), transform_straw1,
            std::move(surface_unique1), detail::allPortals());
        detector_straws.push_back(detector_straw);
        detector_straws.push_back(detector_straw1);
    }
    int counter = 0;
    
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
            if(counter == 0){
                auto vol = Acts::volumeBoundsFromJson<Acts::CuboidVolumeBounds>(muon_vols[i]);
                auto d_vol = DetectorVolumeFactory::construct(portalGenerator, tContext,
                "MS_detector_vol+"+std::to_string(i), transform,
                std::move(vol), surfaces, detector_straws, detail::allPortals());
                detector_stations.push_back(d_vol);
                counter = counter + 1;
                Acts::GeometryView3D::drawDetectorVolume(helper, *d_vol, tContext);
                helper.write("TESTtttt.obj");
                helper.clear();
                std::cout << d_vol->volumes().size() << std::endl;
            }
            else{ 
                auto vol = Acts::volumeBoundsFromJson<Acts::CuboidVolumeBounds>(muon_vols[i]);
                auto d_vol = DetectorVolumeFactory::construct(portalGenerator, tContext,
                "MS_detector_vol+"+std::to_string(i), transform,
                std::move(vol), detail::allPortals());
                detector_stations.push_back(d_vol);
            }
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
    