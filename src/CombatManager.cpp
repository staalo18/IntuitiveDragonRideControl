#include "CombatManager.h"
#include "_ts_SKSEFunctions.h"
#include "DataManager.h"
#include "DisplayManager.h"
#include "FlyingModeManager.h"
#include "ControlsManager.h"
#include "IDRCUtils.h"
#include "APIManager.h"
#include "FastTravelManager.h"
#include "TargetReticleManager.h"

namespace IDRC {   

    void CombatManager::InitializeData(RE::BGSListForm* a_targetPackageList, 
                                RE::BGSListForm* a_breathShoutList, 
                                RE::BGSListForm* a_ballShoutList,
                                RE::TESShout* a_unrelentingForceShout,
                                RE::TESShout* a_attackShout,
                                RE::BGSRefAlias* a_combatTargetAlias) {
        log::info("IDRC - {}", __func__);
        m_combatTargetPackageList = a_targetPackageList;
        SetBreathShoutList(a_breathShoutList);
        SetBallShoutList(a_ballShoutList);
        m_unrelentingForceShout = a_unrelentingForceShout;
        m_attackShout = a_attackShout;
        m_combatTargetAlias = a_combatTargetAlias;

        SetWaitForShout(false);
        SetAttackMode(0);
        m_stopCombat = false;
        m_toggledAutoCombatAttack = false;
        m_registeredForAttack = false;
        m_registeredForTargetSync = false;
        m_attackDisabled = false;
        m_triggerAttack = false;
    }

    int CombatManager::GetAttackMode() {
        return m_attackMode;
    }

    void CombatManager::SetAttackMode(int a_mode) {
        if (a_mode != m_attackMode) {
            m_attackMode = a_mode;
            DataManager::GetSingleton().SendPropertyUpdateEvent("AttackMode", false, 0.0f, m_attackMode);
        }
    }

    void CombatManager::SetAttackDisabled(bool a_disabled) {
        m_attackDisabled = a_disabled;
    }

    bool CombatManager::GetAttackDisabled() {
        return m_attackDisabled;
    }

    bool CombatManager::GetTriggerAttack() {
        return m_triggerAttack;
    }

    void CombatManager::SetTriggerAttack(bool a_trigger) {
        m_triggerAttack = a_trigger;
    }

    bool CombatManager::GetWaitForShout() {
        return m_waitForShout;
    }

    void CombatManager::SetWaitForShout(bool a_wait, bool a_calledFromPapyrus) {
        log::info("IDRC - {}: {}", __func__, a_wait);
        if (a_wait != m_waitForShout) {
            m_waitForShout = a_wait;
            log::info("IDRC - {}: Set to {}", __func__, m_waitForShout);
            if (!a_calledFromPapyrus) {
                DataManager::GetSingleton().SendPropertyUpdateEvent("WaitForShout", m_waitForShout, 0.0f, 0);
            }
        }
    }

    float CombatManager::GetMaxTargetDistance() {
        return m_maxTargetDistance;
    }


    bool CombatManager::GetStopCombat() {
         return m_stopCombat; 
    }

    void CombatManager::SetStopCombat(bool a_stop, bool a_calledFromPapyrus) {
        if (a_stop != m_stopCombat) {
            m_stopCombat = a_stop;

            if (!a_calledFromPapyrus) {
                DataManager::GetSingleton().SendPropertyUpdateEvent("StopCombat", m_stopCombat, 0.0f, 0);
            }
        }
    }

    RE::BGSListForm* CombatManager::GetBreathShoutList() {
        return m_breathShoutList;
    }

    void CombatManager::SetBreathShoutList(RE::BGSListForm* a_breathShoutList) {
        if (!a_breathShoutList) {
            log::error("IDRC - {}: breathShoutList is null", __func__);
        }

        m_breathShoutList = a_breathShoutList;
    }

    RE::BGSListForm* CombatManager::GetBallShoutList() {
        return m_ballShoutList;
    }

    void CombatManager::SetBallShoutList(RE::BGSListForm* a_ballShoutList) {
        if (!a_ballShoutList) {
            log::error("IDRC - {}: ballShoutList is null", __func__);
        }
        
        m_ballShoutList = a_ballShoutList;
    }

    void CombatManager::DragonStartCombat(RE::Actor* a_target) {
        if (!a_target) {
            log::error("IDRC - {}: target is null", __func__);
            return;
        }

        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return;
        }

        // stop ongoing fast travel if any
        if (!FastTravelManager::GetSingleton().StopFastTravel(dragonActor)) {
            log::info("IDRC - {}: Could not stop FastTravel, cancel DragonStartCombat", __func__);
            return;
        }
        
        if (dragonActor->IsInCombat()) {
            SKSE::GetTaskInterface()->AddTask([dragonActor, a_target]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety

                // This just puts the new target on top of the combat target stack
                // need to call StartCombat() (below) to actually make the dragon attack the now target
                _ts_SKSEFunctions::UpdateCombatTarget(dragonActor, a_target);
            });
        }

// TODO - using same workaround as in ForceCombatTargetAliasTo, with same caveats:
        auto handle = _ts_SKSEFunctions::GetHandle(DataManager::GetSingleton().GetRideQuest());
        if (!handle) {
            log::error("IDRC - {}: Quest handle is null", __func__);
            return;
        }
        auto* args = RE::MakeFunctionArguments((RE::Actor*)dragonActor, (RE::Actor*)a_target);
        SKSE::GetTaskInterface()->AddTask([handle, args]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            _ts_SKSEFunctions::SendCustomEvent(handle, "OnDragonStartCombat_SKSE", args);
        });

        int count = 0;
        while (count < 100 && 
               _ts_SKSEFunctions::GetCombatTarget(dragonActor) != a_target) {
            _ts_SKSEFunctions::WaitWhileGameIsPaused();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            count++;
        }
        if (count >= 100) {
            log::error("IDRC - {}: ERROR - Timed out while waiting for dragon to start combat with {} ({})", __func__, a_target->GetName(), a_target->GetFormID());
        }
// END TODO
    }
    
    RE::TESObjectREFR* CombatManager::GetCombatTarget() {
        if (m_combatTargetAlias) {
            return m_combatTargetAlias->GetReference();
        }
        return nullptr;
    }

    bool CombatManager::StopAttack(bool a_forceStop)
    {
        log::info("IDRC - {}", __func__);
        auto& dataManager = DataManager::GetSingleton();
        auto& flyingModeManager = FlyingModeManager::GetSingleton();

        auto* dragonActor = dataManager.GetDragonActor();
        auto* orbitMarker = dataManager.GetOrbitMarker();

        if (!dragonActor) {
            log::error("StopAttack: dragonActor is null");
            return false;
        }
        if (!orbitMarker) {
            log::error("StopAttack: OrbitMarker is null");
            return false;
        }
    
        if (GetWaitForShout() && !a_forceStop && GetAttackMode() > 0) {
            log::info("StopAttack: Cancel Stop Attack {}, {}, {}", GetWaitForShout(), !a_forceStop, GetAttackMode());
            return false;
        }
    
        SetAttackMode(0); // No attack
        SetWaitForShout(false);
        m_registeredForAttack = false;
    
        if (m_toggledAutoCombatAttack) {
            log::info("StopAttack: Resetting auto-combat to false");
            dataManager.SetAutoCombat(false);
            m_toggledAutoCombatAttack = false;
        }
 
        SKSE::GetTaskInterface()->AddTask([orbitMarker, dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            _ts_SKSEFunctions::SetAngle(orbitMarker, dragonActor->GetAngle());
            _ts_SKSEFunctions::ClearLookAt(dragonActor);
            dragonActor->EvaluatePackage();
        });
    
        if (flyingModeManager.GetContinueFlyTo()) {
            flyingModeManager.SetContinueFlyTo(false);
            if (dragonActor->AsActorState()->actorState2.allowFlying == 0) {
                flyingModeManager.DragonLandPlayerRiding(dragonActor, false);
            } else if (flyingModeManager.DragonFlyTo(flyingModeManager.GetFlyToAngle(), false) == 0) {
                flyingModeManager.DragonHoverPlayerRiding(dragonActor, false);
            }
        }
    
        return true;
    }

    bool CombatManager::IsAttackOngoing() {
        return m_registeredForAttack || GetWaitForShout();
    }

    
    bool CombatManager::DragonAttack(bool a_alternateAttack)
    {
        log::info("IDRC - {}", __func__);
        auto& dataManager = DataManager::GetSingleton();
        auto* dragonActor = dataManager.GetDragonActor();
        auto& controlsManager = ControlsManager::GetSingleton();
        auto& flyingModeManager = FlyingModeManager::GetSingleton();
        auto& displayManager = DisplayManager::GetSingleton();

        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return false;
        }

        //  no attack while perch is triggered
        if (flyingModeManager.GetRegisteredForPerch()) {
            log::info("IDRC - {}: Dragon is perching, attack canceled", __func__);
            return false;
        }

        if (m_attackDisabled) {
            log::info("IDRC - {}: Attack is disabled, canceling", __func__);
            return false;
        }
        bool isAlternateAttack = a_alternateAttack;

        // Wait for any ongoing attack to complete
        bool packageCheckResult = _ts_SKSEFunctions::CheckForPackage(dragonActor, m_combatTargetPackageList);
        while (IsAttackOngoing() || GetAttackMode() != 0 || packageCheckResult) {
            _ts_SKSEFunctions::WaitWhileGameIsPaused();
            // in case still in previous attack
		    // wait if player keeps attack key pressed, and start new attack once the previous attack is completed
            if (controlsManager.GetIsKeyPressed(IDRCKey::kSneak)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } else {
                log::info("IDRC - {}: Previous attack still ongoing - cancel attack", __func__);
                return false;
            }
            packageCheckResult = _ts_SKSEFunctions::CheckForPackage(dragonActor, m_combatTargetPackageList);
        }

        m_registeredForAttack = true;
        SetStopCombat(false);
        bool bAttackNotificationDisplayed = false;

        // get the combat target from the reticle (if active), and ensure dragon is in combat with that target
        RE::Actor* combatTarget = TargetReticleManager::GetSingleton().GetCurrentTarget();
        if (combatTarget) {
            if (!bAttackNotificationDisplayed && displayManager.GetDisplayFlyingMode() && displayManager.GetDisplayMessages()) {
                RE::DebugNotification((std::string("Commanding Attack on ") + std::string(combatTarget->GetName())).c_str());
                bAttackNotificationDisplayed = true;
            }
            DragonStartCombat(combatTarget);
        }

        // Get the dragon's combat target
        RE::Actor* target = _ts_SKSEFunctions::GetCombatTarget(dragonActor);

        while (controlsManager.GetIsKeyPressed(IDRCKey::kSneak)) {
            _ts_SKSEFunctions::WaitWhileGameIsPaused();

            if (controlsManager.GetIsKeyPressed(IDRCKey::kRun)) {
                isAlternateAttack = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (target) {
                SKSE::GetTaskInterface()->AddTask([dragonActor, target]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    _ts_SKSEFunctions::SetLookAt(dragonActor, target, true);
                  });
            }
        }
       
        if (APIs::TrueDirectionalMovement && APIs::TrueDirectionalMovement->GetTargetLockState()
            && !TargetReticleManager::GetSingleton().IsReticleLocked()) {
            auto currentTarget = APIs::TrueDirectionalMovement->GetCurrentTarget();
            if (currentTarget) {
                log::info("IDRC - {}: Getting target from TDM: {} ({})", __func__, currentTarget.get()->GetName(), currentTarget.get()->GetFormID());
                if (displayManager.GetDisplayFlyingMode() && displayManager.GetDisplayMessages()) {
                    RE::DebugNotification((std::string("Commanding Attack on ") + std::string(currentTarget.get()->GetName())).c_str());
                    bAttackNotificationDisplayed = true;
                }
                DragonStartCombat(currentTarget.get()->As<RE::Actor>());
            }
        }

        if (!SyncCombatTarget(true)) {
            // don't start the attack if the CombatTarget alias cannot be sync'ed!
            if (StopAttack()) {
                m_registeredForAttack = false;
                return false;
            }
        }

        if (SetShoutMode(0) == 0) { // setting to breath
            log::error("IDRC - {}: Error: No Shout Found", __func__);            
            if (StopAttack()) { // cancel the requested attack
                m_registeredForAttack = false;
                return false;
            }
        }

        // Final update in case the dragon has switched the combat target again (or target died) while resetting fast travel...
        target = _ts_SKSEFunctions::GetCombatTarget(dragonActor);

        if (!target) {
            log::info("IDRC - {}: Starting attack - no CombatTarget", __func__);
            // Display attack notification
            if (!bAttackNotificationDisplayed && displayManager.GetDisplayFlyingMode() && displayManager.GetDisplayMessages()) {
                RE::DebugNotification("Commanding Attack");
                bAttackNotificationDisplayed = true;
            }

            // Use DragonTurnMarker as the target
            float angleZ = dragonActor->GetAngleZ();
            auto* dragonTurnMarker = flyingModeManager.GetDragonTurnMarker();
            SKSE::GetTaskInterface()->AddTask([dragonTurnMarker, dragonActor, angleZ]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                _ts_SKSEFunctions::MoveTo(dragonTurnMarker, dragonActor, 10000.0f * std::sin(angleZ), 10000.0f * std::cos(angleZ), 0.0f);
                _ts_SKSEFunctions::SetLookAt(dragonActor, dragonTurnMarker, true);
              });

            if (dragonActor->HasShout(m_unrelentingForceShout) && (isAlternateAttack || controlsManager.GetIsKeyPressed(IDRCKey::kRun))) {
                SetShoutMode(2); // Set to Unrelenting Force
            }

            SetAttackMode(1); // Attack immediately
        } else {
            log::info("IDRC - {}: Starting attack - CombatTarget: {} ({})", __func__, target->GetName(), target->GetFormID());
            // Display attack notification
            if (!bAttackNotificationDisplayed && displayManager.GetDisplayFlyingMode() && displayManager.GetDisplayMessages()) {
                RE::DebugNotification((std::string("Commanding Attack on ") + std::string(target->GetName())).c_str());
                bAttackNotificationDisplayed = true;
            }
            SKSE::GetTaskInterface()->AddTask([dragonActor, target]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                _ts_SKSEFunctions::SetLookAt(dragonActor, target, true);
              });

            if (_ts_SKSEFunctions::GetDistance(dragonActor, target) < m_maxTargetDistance && 
                _ts_SKSEFunctions::HasLOS(dragonActor, target)) {
                if (_ts_SKSEFunctions::GetFlyingState(dragonActor) == 3) { // Hovering
                    SetAttackMode(3); // Breath immediately from hover
                } else {
                    SetAttackMode(1); // Breath immediately
                }
            } else if (dragonActor->AsActorState()->actorState2.allowFlying == 0) {
                if (!target->GetParentCell()->IsAttached() || _ts_SKSEFunctions::GetDistance(dragonActor, target) > 8000) {
                    RE::DebugNotification("The target is too far away - Cancelling attack");
                    if (StopAttack()) {
                        m_registeredForAttack = false;
                        return false;
                    }
                }

                if (dragonActor->HasShout(m_unrelentingForceShout) && (isAlternateAttack || controlsManager.GetIsKeyPressed(IDRCKey::kRun))) {
                    SetShoutMode(2); // Set to Unrelenting Force
                } else if (_ts_SKSEFunctions::GetDistance(dragonActor, target) >= m_maxTargetDistance) {
                    SetShoutMode(1); // Set to Ball/Storm
                }

                SetAttackMode(1); // Breath immediately
            } else if (dragonActor->HasShout(m_unrelentingForceShout) && (isAlternateAttack || controlsManager.GetIsKeyPressed(IDRCKey::kRun))) {
                if (!target->GetParentCell()->IsAttached() || _ts_SKSEFunctions::GetDistance(dragonActor, target) > 8000) {
                    RE::DebugNotification("The target is too far away - Cancelling attack");
                    if (StopAttack()) {
                        m_registeredForAttack = false;
                        return false;
                    }
                }

                SetShoutMode(2); // Set to Unrelenting Force
                SetAttackMode(1); // Breath immediately
            } else {
                // ensure dragon is not fast travelling
                if (!FastTravelManager::GetSingleton().StopFastTravel(dragonActor)) {
                    StopAttack(); 
                    RE::DebugNotification("Could not attack - cancelling");
                    m_registeredForAttack = false;
                    return false;
                }

                // in case combat stopped while waiting for StopFastTravel to complete, re-initiate combat
                if (_ts_SKSEFunctions::GetCombatState(dragonActor) == 0) {
                    if (target && !target->IsDead() && !m_attackDisabled) {
                        DragonStartCombat(target);
                    } else {
                        if (StopAttack()) { 
                            RE::DebugNotification("Could not attack - cancelling");
                            m_registeredForAttack = false;
                            return false;
                        }
                    }
                }

                if (_ts_SKSEFunctions::GetFlyingState(dragonActor) == 3 && _ts_SKSEFunctions::GetDistance(dragonActor, target) < m_maxTargetDistance) {
                    SetAttackMode(3); // Breath immediately from hover
                } else {
                    SetAttackMode(0); // No shout package, let the dragon initiate combat
                    if (!dataManager.GetAutoCombat()) {
                        dataManager.SetAutoCombat(true);
                        m_toggledAutoCombatAttack = true;
                    }
                }
            }
        }

        SetWaitForShout(true);

        // another sync of the CombatTarget alias
        if (!SyncCombatTarget(true)) {
            // don't continue the attack if the CombatTarget alias cannot be sync'ed!
            if (StopAttack()) {
                m_registeredForAttack = false;
                return false;
            }
        }

        log::info("IDRC - {}: AttackMode: {}", __func__, GetAttackMode());

        auto* combatTargetActor = m_combatTargetAlias ? m_combatTargetAlias->GetActorReference() : nullptr;
        
        if ((combatTargetActor != _ts_SKSEFunctions::GetCombatTarget(dragonActor)) || 
            (target && target->IsDead()) || flyingModeManager.GetRegisteredForLanding()) {
            // Player has triggered a flying state change while the requested attack is not triggered yet
            // or combat target got out-of-sync
        
            if (StopAttack()) { // Cancel the requested attack
                m_registeredForAttack = false;
       
                if (target && target->IsDead()) {
                    RE::DebugNotification("Target is dead - Cancelling attack");
                } else if (flyingModeManager.GetRegisteredForLanding()) {
                    RE::DebugNotification("Landing triggered - Stopping attack");
                }
        
                return false;
            }
        }
        
        if (!dragonActor->AsActorState()->actorState2.allowFlying && 
        flyingModeManager.GetFlyingMode() == FlyingMode::kLanded && 
            _ts_SKSEFunctions::GetFlyingState(dragonActor) == 0) {
            // Trigger another SetAllowFlying(false) to prevent dragon from taking off after attack
            // (sometimes this happened for dragon followers)
            Utils::SetAllowFlying(false);
        }

        // Update the target to the current combat target
        target = m_combatTargetAlias ? m_combatTargetAlias->GetActorReference() : nullptr;

        // TODO: This EvaluatePackage is not yet working as expected:
        // It is triggered before the AttackMode change reaches Papyrus, and therefore
        // the package is still assuming AttackMode == 0
        // Not yet clear how to resolve this. For now, triggering an EvaluatePackage on
        // Papyrus level via SetAttackMode()
        SKSE::GetTaskInterface()->AddTask([dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            dragonActor->EvaluatePackage();
            log::info("IDRC - {}: EvaluatePackage", __func__);
        });

        // Monitor the attack progress
        int count = 0;
        int notificationCount = 0;
        while (GetWaitForShout()) {
            _ts_SKSEFunctions::WaitWhileGameIsPaused();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            count++;
            notificationCount++;

            if (_ts_SKSEFunctions::GetCombatState(dragonActor) == 0) {
                if (target && !target->IsDead() && !m_attackDisabled) {
                    DragonStartCombat(target);
                } else {
                    SetWaitForShout(false);
                    StopAttack();
                }
            }

            if (count > 150) {
                SetWaitForShout(false);
                StopAttack();
                RE::DebugNotification("Could not attack - cancelling");
            }

            if (displayManager.GetDisplayFlyingMode() && notificationCount >= 25) {
                RE::DebugNotification("Attack in progress");
                notificationCount = 0;
            }
        }

        m_registeredForAttack = false;

        if (dragonActor->AsActorState()->actorState2.allowFlying == 0 || 
            flyingModeManager.GetRegisteredForLanding()) {
            flyingModeManager.DragonLandPlayerRiding(dragonActor, false);
        } else  {
            auto* orbitMarker = dataManager.GetOrbitMarker();
            if (!orbitMarker) {
                log::error("IDRC - {}: OrbitMarker is null", __func__);
                return false;
            }
            flyingModeManager.DragonHoverPlayerRiding(orbitMarker, false);
        }

        return true;
    }

    RE::TESShout* CombatManager::GetShout(const RE::BGSListForm* a_shoutList) {
        if (!a_shoutList) {
            log::error("IDRC - {}: ShoutList is null", __func__);
            return nullptr;
        }
    
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
    
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return nullptr;
        }
    
        // Iterate through the ShoutList in reverse order
        for (int i = a_shoutList->forms.size() - 1; i >= 0; --i) {
            auto* form = a_shoutList->forms[i];
            if (!form) {
                continue;
            }
    
            auto* shout = form->As<RE::TESShout>();
            if (shout && dragonActor->HasShout(shout)) {
                return shout;
            }
        }
    
        log::info("IDRC - {}: No valid shout found in ShoutList", __func__);
        return nullptr;
    }

    bool CombatManager::SetShoutMode(int a_shoutMode) {
        log::info("IDRC - {}: {}", __func__, a_shoutMode);
    
        RE::TESShout* usedShout = nullptr;
    
        if (a_shoutMode == 0) { // Breath
            usedShout = GetShout(GetBreathShoutList());
        } else if (a_shoutMode == 1) { // Ball / Storm
            usedShout = GetShout(GetBallShoutList());
        } else if (a_shoutMode == 2) { // Unrelenting Force
            usedShout = m_unrelentingForceShout;
        }
    
        if (!usedShout) {
            log::error("IDRC - {}: No Shout found!", __func__);
            return false;
        }
        if (m_attackShout->variations && m_attackShout->variations[0].spell) {
            log::info("IDRC - {}: Old AttackShout-spell0: {}", __func__, m_attackShout->variations[0].spell->GetFormID());
        } else {
            log::info("IDRC - {}: Old AttackShout-spell0 is null", __func__);
        }

        for (int i = 0; i < 3; ++i) {
            auto* word = usedShout->variations[i].word;
            auto* spell = usedShout->variations[i].spell;
    
            SKSE::GetTaskInterface()->AddTask([this, word, spell, i]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
                if (word) {
                    this->m_attackShout->variations[i].word = word;
                }
                if (spell) {
                    this->m_attackShout->variations[i].spell = spell;
                } 
            });

        }
        int count = 0;
        while (count < 100 && m_attackShout->variations[0].spell != usedShout->variations[0].spell) {
            // wait until the spell is updated
            // TODO: is there a better way???
            _ts_SKSEFunctions::WaitWhileGameIsPaused();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            count++;
        }
        if (count >= 100) { // waited > 1sec
            log::error("IDRC - {}: ERROR - Timed out while waiting for AttackShout to sync!", __func__);
            return false;
        }

        if (m_attackShout->variations && m_attackShout->variations[0].spell) {
            log::info("IDRC - {}: New AttackShout-spell0: {}", __func__, m_attackShout->variations[0].spell->GetFormID());
        } else {
            log::info("IDRC - {}: New AttackShout-spell0 is null", __func__);
        }
    
        return true;
    }
    

    bool CombatManager::SyncCombatTarget(bool a_forceSync) {
        log::info("IDRC - {}: {}, {}", __func__, a_forceSync, m_registeredForTargetSync);
        auto& dataManager = DataManager::GetSingleton();
        auto* dragonActor = dataManager.GetDragonActor();

        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return false;
        }

        RE::Actor* currentCombatTarget =  _ts_SKSEFunctions::GetCombatTarget(dragonActor);
        if (currentCombatTarget == GetCombatTarget()) {
            log::info("IDRC - {}: Already synced", __func__);
            return true;
        }

        // If already registered for target sync, return false
        if (m_registeredForTargetSync) {
            log::info("IDRC - {}: Already registered. Doing nothing and returning false", __func__);
            return false;
        }

        m_registeredForTargetSync = true;

        if (a_forceSync) {
            bool packageCheckResult = _ts_SKSEFunctions::CheckForPackage(dragonActor, m_combatTargetPackageList);
            int count = 0;
            while (packageCheckResult && count < 20) {
                _ts_SKSEFunctions::WaitWhileGameIsPaused();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                log::info("IDRC - {}: Waiting for TargetPackage to end", __func__);
                packageCheckResult = _ts_SKSEFunctions::CheckForPackage(dragonActor, m_combatTargetPackageList);
                count++;
            }
            if (packageCheckResult) {
                log::error("IDRC - {}: ERROR - Timed out waiting for TargetPackage to end!", __func__);
                m_registeredForTargetSync = false;
                return false;
            }
        }

        if (currentCombatTarget) {
            log::info("IDRC - {}: Updating to {}", __func__, currentCombatTarget->GetFormID());
        } else {
            log::info("IDRC - {}: Clearing target", __func__);
        }

        if (!Utils::ForceAliasTo(m_combatTargetAlias, currentCombatTarget)) {
            log::error("IDRC - {}: ERROR - Could not force alias to combat target", __func__);
            m_registeredForTargetSync = false;
            return false;
        }

        SKSE::GetTaskInterface()->AddTask([dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            dragonActor->EvaluatePackage();
        });

        m_registeredForTargetSync = false;

        return true;
    }

    RE::BGSListForm* CombatManager::GetCombatTargetPackageList() {
        return m_combatTargetPackageList;
    }

    bool CombatManager::IsAutoCombatAttackToggled() {
        return m_toggledAutoCombatAttack;
    }   

} // namespace IDRC

