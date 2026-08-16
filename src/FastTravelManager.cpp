#include "FastTravelManager.h"
#include "_ts_SKSEFunctions.h"
#include "DataManager.h"

#include "RE/Skyrim.h"
#include "SKSE/API.h"


namespace IDRC {

    void FastTravelManager::Update() {
        if (RE::UI::GetSingleton()->GameIsPaused()) {
            return;
        }
                
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            return;
        }

        bool patrolQueuedState =_ts_SKSEFunctions::IsFlyingMountPatrolQueued(dragonActor);
        bool fastTravelState = _ts_SKSEFunctions::IsFlyingMountFastTravelling(dragonActor);
        if (patrolQueuedState || fastTravelState) {
            // in dragon-FastTravel mode - check if dragon is allowed to fly
            if (!dragonActor->AsActorState()->actorState2.allowFlying) {
                log::info("IDRC - {}: in FastTravel mode, but not allowed to fly - stopping fast travel...", __func__);

                auto* orbitMarker = DataManager::GetSingleton().GetOrbitMarker();                
                SKSE::GetTaskInterface()->AddTask([dragonActor, orbitMarker]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    if (orbitMarker) {
                        _ts_SKSEFunctions::MoveTo(orbitMarker, dragonActor, 0.0f, 0.0f,  0.0f);
                    } else {
                        log::warn("IDRC - {}: Orbit marker is null", __func__);
                    }

                    // force stop fasttravel via StopFastTravel Package
                    dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 0); // orbit
                    dragonActor->EvaluatePackage();
                });
            }
        } else if (m_lastPatrolQueuedState || m_lastFastTravelState) {
            // leaving FastTravel mode
            auto& flyingModeManager = FlyingModeManager::GetSingleton();
            if (!dragonActor->AsActorState()->actorState2.allowFlying) {
                log::info("IDRC - {}: Leaving FastTravel and not allowed to fly - trigger land", __func__);
                if (flyingModeManager.GetRegisteredForLanding()) {
                    if (!flyingModeManager.GetLandingPosSearchOngoing()) {
                        flyingModeManager.TriggerLand();
                    }
                } else {
                    std::thread([dragonActor]() {
                        // send to new thread so that DragonLandPlayerRiding() is not blocking Update()
                        FlyingModeManager::GetSingleton().DragonLandPlayerRiding(dragonActor, false);
                    }).detach();
                }
            }
        }

        m_lastPatrolQueuedState = patrolQueuedState;
        m_lastFastTravelState = fastTravelState;
    }


    void FastTravelManager::FastTravel(const RE::TESObjectREFR* a_fastTravelTarget) {
        log::info("IDRC - {}", __func__);
        if (!a_fastTravelTarget) {
            log::error("IDRC - {}: error - FastTravelTarget is none", __func__);
            return;
        } 
        
        auto dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: error - dragonActor is none", __func__);
            return;
        }

        if (m_skipFastTravelRequest) {
            // Set in Hooks::PathingHook::UpdateFlightPathData() to skip FastTravel (ExecuteTeleport) requests
            // while pathData is still being updated from the previous FastTravel request.
            // FastTravel is setting pathData to nullptr until new pathData is generated.
            // This can take a few frames (in particular if many pathing requests occur in parallel, eg during combat).
            // Skipping FastTravel in this case avoids continuous nullifying of pathData.

            log::info("IDRC - {}: FastTravel is skipped", __func__);
            return;
        }

        if (!dragonActor->AsActorState()->actorState2.allowFlying) {
            log::info("IDRC - {}: not allowed to fly - cancelling fast travel request", __func__);

            if (dragonActor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kVariable03) == 2) {
                // fast travel active - force stop fasttravel via StopFastTravel Package
                SKSE::GetTaskInterface()->AddTask([dragonActor]() {
                    dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 0); // orbit
                });
            }
            return;
        }

        // Block immediately subsequent FastTravel requests to prevent multiple 
        // concurrent pathing requests for the dragon,
        // until this FastTravel request has successfully triggered UpdatePathData() with valid pathData.
        // Unclear if this is really needed but should prevent  game freeze observed in such a situation.
        // UpdateFlightPathData() will clear this flag once valid pathData is available.
        m_skipFastTravelRequest = true;

        SKSE::GetTaskInterface()->AddTask([a_fastTravelTarget, dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                log::warn("IDRC - {}: Could not get player", __func__);
                return;
            }

            auto* tes = RE::TES::GetSingleton();
            if (!tes) {
                log::warn("{}: Cannot access TES", __FUNCTION__);
                return;
            }
            auto* worldspace = tes->GetRuntimeData2().worldSpace;
            if (!worldspace) {
                log::warn("{}: Cannot access worldspace", __FUNCTION__);
                return;
            }

            // trigger fast travel to a_fastTravelTarget in next frame 
            auto* loc = Utils::GetQueuedTargetLoc(player);
            loc->world         = worldspace;
            loc->interior      = nullptr;
            loc->location      = a_fastTravelTarget->GetPosition();
            loc->angle         = a_fastTravelTarget->GetAngle();
            loc->arrivalFunc   = nullptr;
            loc->arrivalFuncData = 0;
            loc->furnitureRef  = RE::RefHandle{};
            GetRefHandle(a_fastTravelTarget, &loc->fastTravelMarker);
            loc->resetWeather  = false;
            loc->allowAutoSave = false;
            loc->isValid       = true;  // triggers the fast travel on next per-frame ExecuteTeleport call

// Previous solution: call the Papyrus function Game.FastTravel(). 
// This works as well and is an alternative to setting the loc values.
//            _ts_SKSEFunctions::CallPapyrusFunction("Game"sv, "FastTravel"sv, a_fastTravelTarget);

            // trigger fasttravel package
            dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 2);
            dragonActor->EvaluatePackage();
        });
    }
} // namespace IDRC
