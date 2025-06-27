#include "FastTravelManager.h"
#include "_ts_SKSEFunctions.h"
#include "DataManager.h"

#include "RE/Skyrim.h"
#include "SKSE/API.h"


namespace IDRC {

    void FastTravelManager::InitializeData(RE::BGSListForm* a_list) {
        log::info("IDRC - {}", __func__);
        m_fastTravelPackagelist = a_list;
        m_stopFastTravelOngoing = false;
        m_cancelStopFastTravelTriggered = false;
    }

    void FastTravelManager::FastTravel(const RE::TESObjectREFR* a_fastTravelTarget) {
        log::info("IDRC - {}", __func__);
        if (!a_fastTravelTarget) {
            log::error("IDRC - {}: error - FastTravelTarget is none", __func__);
            return;
        } 
        _ts_SKSEFunctions::CallPapyrusFunction("Game"sv, "FastTravel"sv, a_fastTravelTarget);
    }

    bool FastTravelManager::CancelStopFastTravel() {
// Whenever a new FastTravel is triggered, the CancelStopFastTravel() function needs to be called.
// That will make StopFastTravel() to return false immediately.
//
// A new FastTravel is either triggered by user request (DragonFlyTo()), 
// or by the game engine (in combat, to steer the dragon towards the combat target).
// In both cases an ongoing StopFastTravel() cannot return true anymore, because 
// the new fasttravel request overrides the previous one that was triggered in 
// StopFastTravel() to stop fasttravel.
//
// Each function calling StopFastTravel() needs to deal gracefully with the fact that its
// ongoing StopFastTravel() call can be cancelled, and that the dragon stays in FastTravel state.

        log::info("IDRC - {}", __func__);
        if (!m_stopFastTravelOngoing) {
            return true;
        }
    
        m_cancelStopFastTravelTriggered = true;

        int count = 0;
        while (m_stopFastTravelOngoing && count < 20) {
            _ts_SKSEFunctions::WaitWhileGameIsPaused();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));           
            count++;
        }
    
        m_cancelStopFastTravelTriggered = false;
    
        if (m_stopFastTravelOngoing) {
            log::info("IDRC - {}: Failed to cancel StopFastTravel", __func__);
            return false;
        }
    
        log::info("IDRC - {}: Cancelled StopFastTravel", __func__);
        return true;
    }


    // The function StopFastTravel() can be used to reliably switch out of the "dragon-FastTravel" state.
    bool FastTravelManager::StopFastTravel(RE::TESObjectREFR* a_stopFastTravelTarget, 
            float a_height, int a_timeout, std::string a_waitMessage, 
            std::string a_timeoutMessage) {
        // stops dragon fast travel at fHeight above the position of the StopFastTravelTarget
        // returns true once the dragon has left fast travel state, false if timeout
        log::info("IDRC - {}", __func__);

        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        auto* orbitMarker = DataManager::GetSingleton().GetOrbitMarker();

        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is None", __func__);
            return false;
        }
        if (!orbitMarker) {
            log::error("IDRC - {}: OrbitMarker is None", __func__);
            return false;
        }

        // only if the dragon currently is in dragon-FastTravel state  
        if (_ts_SKSEFunctions::IsFlyingMountPatrolQueued(dragonActor) || 
            _ts_SKSEFunctions::IsFlyingMountFastTravelling(dragonActor)) {
                if (!a_stopFastTravelTarget) {
                log::error("IDRC - {}: error - StopFastTravelTarget is none", __func__);
                m_stopFastTravelOngoing = false;
                return false;
            }
    
            // cancel any previously triggered and still ongoing StopFastTravel()
            CancelStopFastTravel();
    
            // trigger a new StopFastTravel sequence
            m_cancelStopFastTravelTriggered = false;
            m_stopFastTravelOngoing = true;
    
            // Wait until the dragon is in a FastTravel package.
            // This could be delayed for example in case a user-triggered dragon breath attack is ongoing.
            // In that case the dragon is still in a shout package when the engine switches to dragon-FastTravel state.
            if (!EnsureFastTravelPackageIsActive(a_timeout, a_timeoutMessage)) {
                if (!a_timeoutMessage.empty() && !m_cancelStopFastTravelTriggered) {                 
                    RE::DebugNotification(a_timeoutMessage.c_str());
                }
                m_stopFastTravelOngoing = false;
                return false;
            }

            // Trigger FastTravel to the StopFastTravelTarget
            SKSE::GetTaskInterface()->AddTask([orbitMarker, a_stopFastTravelTarget, dragonActor, a_height]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                _ts_SKSEFunctions::MoveTo(orbitMarker, a_stopFastTravelTarget, 0.0f, 0.0f, a_height);
                _ts_SKSEFunctions::ClearCombatTargets(dragonActor);
            });
            log::info("IDRC - {}: Cleared CombatTargets", __func__);

            FastTravel(orbitMarker);
            SKSE::GetTaskInterface()->AddTask([dragonActor]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                dragonActor->EvaluatePackage();
            });
    
            // Wait until the dragon has left the dragon-FastTravel state
            int waitCount = 0;
            int messageCount = 0;
            bool patrolQueued = _ts_SKSEFunctions::IsFlyingMountPatrolQueued(dragonActor);
            bool fastTravel = _ts_SKSEFunctions::IsFlyingMountFastTravelling(dragonActor);

            while (!m_cancelStopFastTravelTriggered && waitCount < a_timeout && (fastTravel || patrolQueued)) {
                log::info("IDRC - {}: Wait for fast travel to end ({}). WaitMessage: {}", __func__, waitCount, a_waitMessage);
                // checking bPatrolQueued and bFastTravel in the next cycle of the while loop delays the completion of StopFastTravel() by one cycle
                // This will catch the case where FastTravel (re-)starts directly after PatrolQueued / FastTravel completes
                // ie the loop continues to wait for FastTravel to end
                // otherwise StopFastTravel() would return, with FastTravel starting immediately afterwards
                _ts_SKSEFunctions::WaitWhileGameIsPaused();
                patrolQueued = _ts_SKSEFunctions::IsFlyingMountPatrolQueued(dragonActor);
                fastTravel = _ts_SKSEFunctions::IsFlyingMountFastTravelling(dragonActor);

                SKSE::GetTaskInterface()->AddTask([dragonActor]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    dragonActor->EvaluatePackage();
                });
                
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                waitCount++;
                messageCount++;
                if (messageCount > 50) {
                    messageCount = 0;
                    if (!a_waitMessage.empty()) {
                        RE::DebugNotification(a_waitMessage.c_str());
                    }
                }
            }
    
            if (m_cancelStopFastTravelTriggered) {
                m_stopFastTravelOngoing = false;
                log::info("IDRC - {}: Cancelled.", __func__);
                return false;
            }

            // did the dragon leave the dragon-FastTravel state?
            if ( waitCount >= a_timeout && 
                    (_ts_SKSEFunctions::IsFlyingMountPatrolQueued(dragonActor) || 
                    _ts_SKSEFunctions::IsFlyingMountFastTravelling(dragonActor))) {
                log::info("IDRC - {}: ERROR: Timeout - aborting StopFastTravel(). TimeoutMessage: {}", __func__, a_timeoutMessage);
                if (!a_timeoutMessage.empty()) {
                    RE::DebugNotification(a_timeoutMessage.c_str());
                }
                m_stopFastTravelOngoing = false;
                return false;
            }
        }
        m_stopFastTravelOngoing = false;
        return true;
    }

    bool FastTravelManager::CheckFastTravelConditions() {
        log::info("IDRC - {}", __func__);
        bool bCheckSuccess = true;
        auto *dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is None", __func__);
            return false;
        }
        bool allowFlying = dragonActor->AsActorState()->actorState2.allowFlying;
        bool isPatrolQueued = _ts_SKSEFunctions::IsFlyingMountPatrolQueued(dragonActor);
        bool isFastTravelling = _ts_SKSEFunctions::IsFlyingMountFastTravelling(dragonActor);

        if (isPatrolQueued || isFastTravelling) {
            if (allowFlying == 0) {
                log::info("IDRC - {}: not allowed to fly!", __func__);
            }
            bool packageCheckResult = _ts_SKSEFunctions::CheckForPackage(dragonActor, m_fastTravelPackagelist);

            if (!packageCheckResult) {
                log::info("IDRC - {}: not in fast travel package", __func__);
            SKSE::GetTaskInterface()->AddTask([dragonActor]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    dragonActor->EvaluatePackage();
                });    
                bCheckSuccess = false;
            }
        }
    
        return bCheckSuccess;
    }


    bool FastTravelManager::EnsureFastTravelPackageIsActive(int a_timeout, std::string a_timeoutMessage) {
        log::info("IDRC - {}", __func__);
        int iFTCheckCount = 0;
        while (!CheckFastTravelConditions() && iFTCheckCount < a_timeout && !m_cancelStopFastTravelTriggered) {
            _ts_SKSEFunctions::WaitWhileGameIsPaused();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            iFTCheckCount++;
        }
    
        if (m_cancelStopFastTravelTriggered) {
            log::info("IDRC - {}: Cancelled.", __func__);
            return false;
        }
    
        if (iFTCheckCount >= a_timeout && !CheckFastTravelConditions()) {
            log::info("IDRC - {}: Error - CheckFastTravel timed out!", __func__);
            if (!a_timeoutMessage.empty()) {
                RE::DebugNotification(a_timeoutMessage.c_str());
            }
            return false;
        }
        return true;
    }
} // namespace IDRC
