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

        // std::cout << "candidate portals size: " << candidate_portals.size() << std::endl;
        return candidate_portals;
    }

    //If we want to test other methods for getting candidates, 
    ///I think we only need to make a new update_candidates function
    inline void update_candidates(const GeometryContext gctx, NavigationState& nState){
        const Frustum3 frustum(nState.position, nState.direction, M_PI/30);
        auto normals = frustum.normals();
        m_frustum = frustum;
        //This is a check to make sure we are inside the frustum (probably at origin if we are here)
        if((std::all_of(normals.begin(), normals.end(), [&nState, &frustum](auto& i) {
            return (nState.position - frustum.origin()).dot(i) >= 0;}))) {
            auto candidate_portals = search_octree(get_frustum(), get_octree());
            //draw candidate_portals
            // for(auto portals : candidate_portals){
            //     for(auto portal : portals){
            //         GeometryView3D::drawPortal(helper, *portal, gctx);
            //     }
            // }
            // helper.write("testing!.obj");
            // helper.clear();
            // std::cout << "candidate portals size: " << candidate_portals.size() << std::endl;
            for(auto portals : candidate_portals){
                for(auto& portal: portals){
                    auto surface_candidate = NavigationState::SurfaceCandidate{
                        ObjectIntersection<Surface>{}, &(portal->surface()), portal, nState.surfaceBoundaryCheck};
                    nState.surfaceCandidates.push_back(*(&surface_candidate));
                }
            }
        }  
    } //update_candidates function

    inline void update(const GeometryContext& gctx,
                            NavigationState& nState) noexcept(false) {
    
        ObjVisualization3D helper;
        //If nState.surfaceCandidates is empty, then we should initalize it by finding the volumse intersected by frustum
        if((nState.surfaceCandidates.empty())){
            update_candidates(gctx, nState);
        }

        //This will retrieve the frustum we have saved and we will check if we are still inside of the frustum
        auto frustum = get_frustum();
        auto normals = frustum.normals();
        if( (std::any_of(normals.begin(), normals.end(), [&nState, &frustum](auto& i) {
            return (nState.position - frustum.origin()).dot(i) < 0;})) ) {
                nState.surfaceCandidates.clear();
                nState.surfaceCandidate = {};
                update_candidates(gctx, nState);
                update(gctx, nState);
                return;
        } 
        //If we are inside the frustum, we find the closest surface to our location that is reachable 
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
                auto b_status = b.objectIntersection.intersection.status;
                auto reachable = [](Intersection3D::Status status){
                    return status == Intersection3D::Status::reachable;
                };
                auto on_surface = [](Intersection3D::Status status){
                    return status == Intersection3D::Status::onSurface;
                };

                if((std::abs(a_pathlength) < std::abs(b_pathlength)) and (a_pathlength > 0)){
                    if(reachable(a_status) or on_surface(a_status)){
                        return true;
                    } else{
                        return false;
                    }
                }
                if(std::abs(a_pathlength) > std::abs(b_pathlength) and (b_pathlength > 0)){
                    if(reachable(b_status) or on_surface(b_status)){
                        return false;
                    } else{
                        return true;
                    }
                }
                return false;

            });
            //The first candidate will be the closest, and reachable candidate, so we set it as the current candidate
            nState.surfaceCandidate = nCandidates.begin();
            // for(auto cand : nCandidates){
            //     std::cout << "candidate: " << cand.objectIntersection.intersection.pathLength << " status: " << cand.objectIntersection.intersection.status << std::endl;
            // }
        }

        // nState.surfaceCandidate->objectIntersection.intersection.pathLength = 412;

        //From here we would ideally constrain the step but i am not sure how to do that yet

        std::cout << "nState.surfaceCandidate: " << nState.surfaceCandidate->objectIntersection.intersection.pathLength << std::endl;
        std::cout << "nState.surfaceCandidate: " << nState.surfaceCandidate->objectIntersection.intersection.status << std::endl;   
    } // update function

    const BoundingBox* get_octree(){
        return m_octree;
    }

    const Frustum3& get_frustum(){
        return m_frustum;
    }

    private:
    const BoundingBox* m_octree;
    std::vector<std::unique_ptr<BoundingBox>> m_store;
    Frustum3 m_frustum = Frustum3(Vector3(0, 0, 0), Vector3(0, 0, 1), M_PI_2);
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
