// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/Detector.hpp"
#include "Acts/Geometry/DetectorVolume.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/NavigationDelegates.hpp"
#include "Acts/Geometry/NavigationState.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"


#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Utilities/Frustum.hpp"

namespace Acts {
namespace Experimental {

/// @brief The end of world sets the volume pointer of the
/// navigation state to nullptr, usually indicates the end of
/// the known world, hence the name
using Frustum3 = Frustum<ActsScalar, 3, 3>;

struct MuonImpl{

    MuonImpl(const DetectorVolume& volume){
        ObjVisualization3D helper;
        auto gctx = GeometryContext();
        std::vector<BoundingBox*> boxes;
        auto volumes = volume.volumes();
        std::cout << "volumes size: " << volumes.size() << std::endl;
        for (const auto v : volumes) {
            auto bb = v->getBoundingBox();
            boxes.push_back(&(*bb));
        }
        std::vector<std::unique_ptr<BoundingBox>> store;
        auto top_box = make_octree(store, boxes, 500);
        const BoundingBox* lnode = top_box;
        m_octree = std::move(lnode);
        m_store = std::move(store);
    }

    inline std::vector<std::vector<const Portal*>> search_octree(const Frustum3& frustum, const BoundingBox *lnode) {
        ObjVisualization3D helper;
        auto gctx = GeometryContext();
        std::vector<std::vector<const Portal*>> candidate_portals;

        do{
            if(lnode->intersect(frustum)){
                if(lnode->hasEntity()){
                    auto portals = lnode->entity()->portals();
                    if(std::find(candidate_portals.begin(), candidate_portals.end(), lnode->entity()->portals()) == candidate_portals.end()){
                        candidate_portals.push_back(portals);
                        lnode = lnode->getSkip();
                    }else{
                        lnode = lnode->getSkip();
                    }
                }else{
                    lnode = lnode->getLeftChild();
                }     
            } else{
                lnode = lnode->getSkip();
            }
        }while(lnode != nullptr);

        std::cout << "candidate portals size: " << candidate_portals.size() << std::endl;
        return candidate_portals;
    }

    inline void update(const GeometryContext& gctx,
                            NavigationState& nState) noexcept(false) {
    
        ObjVisualization3D helper;
        const Frustum3 frustum(nState.position, nState.direction, M_PI/30);
        auto normals = frustum.normals();

        if((nState.surfaceCandidates.size() == 0) and (std::all_of(normals.begin(), normals.end(), [&nState, &frustum](auto& i) {
            return (nState.position - frustum.origin()).dot(i) >= 0;}))) {
            auto candidate_portals = search_octree(frustum, get_octree());
            //draw candidate_portals
            // for(auto portals : candidate_portals){
            //     for(auto portal : portals){
            //         GeometryView3D::drawPortal(helper, *portal, gctx);
            //     }
            // }
            // helper.write("testing!.obj");
            // helper.clear();
            std::cout << "candidate portals size: " << candidate_portals.size() << std::endl;
            for(auto portals : candidate_portals){
                for(auto& portal: portals){
                    auto surface_candidate = NavigationState::SurfaceCandidate{
                        ObjectIntersection<Surface>{}, &(portal->surface()), portal, nState.surfaceBoundaryCheck};
                    nState.surfaceCandidates.push_back(*(&surface_candidate));
                }
            }
        }   

        if( (std::any_of(normals.begin(), normals.end(), [&nState, &frustum](auto& i) {
            return (nState.position - frustum.origin()).dot(i) < 0;})) ) {
                nState.surfaceCandidates.clear();
                nState.surfaceCandidate = {};
        } 
        else{
            auto nCandidates = nState.surfaceCandidates;
            for(auto& cand : nCandidates){
                const Surface& sRep =
                    (cand.surface != nullptr) ? (*cand.surface) : (cand.portal->surface());
                auto intersection = sRep.intersect(gctx, nState.position, nState.direction, cand.bCheck);
                cand.objectIntersection = intersection;
            }
            
            //EVERYTHING ABOVE HERE SHOULD BE ONLY RUN ONCE UNTIL WE LEAVE OCTREE
            std::sort(nCandidates.begin(), nCandidates.end(), [](const NavigationState::SurfaceCandidate& a, const NavigationState::SurfaceCandidate& b){
                auto a_pathlength = a.objectIntersection.intersection.pathLength;
                auto b_pathlength = b.objectIntersection.intersection.pathLength;
                auto a_status = a.objectIntersection.intersection.status;
                auto reachable = [](Intersection3D::Status status){
                    return status == Intersection3D::Status::reachable;
                };
                auto on_surface = [](Intersection3D::Status status){
                    return status == Intersection3D::Status::onSurface;
                };
                if(reachable(a_status) or on_surface(a_status)){
                    return std::abs(a_pathlength) < std::abs(b_pathlength);
                } else{
                    return false;
                }
                return false;
        });

            nState.surfaceCandidate = nCandidates.begin();
        }

        // nState.surfaceCandidate->objectIntersection.intersection.pathLength = 412;

        //From here we would ideally constrain the step but i am not sure how to do that yet

        std::cout << "nState.surfaceCandidate: " << nState.surfaceCandidate->objectIntersection.intersection.pathLength << std::endl;
        std::cout << "nState.surfaceCandidate: " << nState.surfaceCandidate->objectIntersection.intersection.status << std::endl;   
    }

    const BoundingBox* get_octree(){
        return m_octree;
    }

    private:
    const BoundingBox* m_octree;
    std::vector<std::unique_ptr<BoundingBox>> m_store;
};

// inline static ManagedSurfaceCandidatesUpdator MuonDelegate() {
//   ManagedSurfaceCandidatesUpdator updator;
//   SurfaceCandidatesUpdator sFinder;
//   sFinder.connect<&MuonImpl::update>();
//   updator.delegate = std::move(sFinder);
//   updator.implementation = nullptr;
//   return updator;
// }


}  // namespace Experimental
}  // namespace Acts
