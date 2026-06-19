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

    void CombatManager::InitializeData(RE::BGSListForm* a_breathShoutList, 
                                RE::BGSListForm* a_ballShoutList,
                                RE::TESShout* a_unrelentingForceShout,
                                RE::TESShout* a_attackShout,
                                RE::BGSRefAlias* a_combatTargetAlias) {
        log::info("IDRC - {}", __func__);
        SetBreathShoutList(a_breathShoutList);
        SetBallShoutList(a_ballShoutList);
        m_unrelentingForceShout = a_unrelentingForceShout;
        m_attackShout = a_attackShout;
        m_combatTargetAlias = a_combatTargetAlias;

        m_stopCombat = false;
        m_attackDisabled = false;
    }

    void CombatManager::SetAttackDisabled(bool a_disabled) {
        m_attackDisabled = a_disabled;
    }

    bool CombatManager::GetAttackDisabled() {
        return m_attackDisabled;
    }

    float CombatManager::GetMaxTargetDistance() {
        return m_maxTargetDistance;
    }


    bool CombatManager::GetStopCombat() {
        return false;
//         return m_stopCombat; 
    }

    void CombatManager::SetStopCombat(bool a_stop, bool a_calledFromPapyrus) {
        m_stopCombat = false;
        return;
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

        if (_ts_SKSEFunctions::GetDistance(dragonActor, a_target) >= 5000.0f) {
            log::info("IDRC - {}: Target is too far away, cancel DragonStartCombat", __func__);
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

        StartCombat(dragonActor, a_target, dragonActor->GetCombatGroup());
    }
    
//    RE::TESObjectREFR* CombatManager::GetCombatTarget() {
//        if (m_combatTargetAlias) {
//            return m_combatTargetAlias->GetReference();
//        }
//        return nullptr;
//    }

    void CombatManager::Update() {
         if (RE::UI::GetSingleton()->GameIsPaused()) {
            return;
        }

        if (!DataManager::GetSingleton().GetDragonActor()) {
            return;
        }

        RestartCombatIfNeeded();

        UpdateAttack();
    }

    void CombatManager::RestartCombatIfNeeded() {

        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            return;
        }

        if (FlyingModeManager::GetSingleton().GetFlyingMode() != FlyingMode::kFlying &&
            _ts_SKSEFunctions::GetFlyingState(dragonActor) != 2 &&
            !_ts_SKSEFunctions::IsFlyingMountPatrolQueued(dragonActor) && 
            !_ts_SKSEFunctions::IsFlyingMountFastTravelling(dragonActor)) 
        {
            if (m_restartCombatPending)
            {
                m_restartCombatPending = false;            

                if (m_storedCombatTarget) {
                    DragonStartCombat(m_storedCombatTarget);
                }
            }

            m_storedCombatTarget = _ts_SKSEFunctions::GetCombatTarget(dragonActor);
        } else {
            m_restartCombatPending = true;

            if (m_shoutTarget) {
                m_storedCombatTarget = m_shoutTarget;
            }
        }
    }

    void CombatManager::UpdateAttack() {
        bool attackStopped = false;
        if (m_shoutTimer > 3.0f) {
            m_shoutActive = false;
            attackStopped = true;
        }
        m_shoutTimer += *g_deltaTimeRealTime;

        if (!m_shoutActive) {
            m_shoutTarget = nullptr;
            m_shoutTimer = 0.0f;
            if (attackStopped) {
                auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
                if (dragonActor) {
                    SKSE::GetTaskInterface()->AddTask([dragonActor]() {
                        // When modifying Game objects, send task to TaskInterface to ensure thread safety
                        _ts_SKSEFunctions::ClearLookAt(dragonActor);
                    });
                }

                FlyingModeManager::GetSingleton().DragonTurnPlayerRiding(0.0f);
            }
        }       
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

        if (m_shoutActive) {
            log::info("IDRC - {}: Shout is ongoing, canceling new attack", __func__);
            return false;
        }

        bool isAlternateAttack = a_alternateAttack;
        m_shoutTimer = 0.0f;
        m_shoutActive = true;
         
        SetStopCombat(false);
        
        m_shoutTarget = nullptr;
        if (APIs::TrueDirectionalMovementV1 && APIs::TrueDirectionalMovementV1->GetTargetLockState()
            && !TargetReticleManager::GetSingleton().IsReticleLocked()) {
            auto currentTDMTarget = APIs::TrueDirectionalMovementV1->GetCurrentTarget();
            if (currentTDMTarget) {
                m_shoutTarget = currentTDMTarget.get()->As<RE::Actor>();
            }
        }

        if (!m_shoutTarget) {
            // No TDM target, get the combat target from the reticle (if active)
log::info("IDRC - {}: No TDM target, checking Target Reticle Manager for target", __func__);
            m_shoutTarget = TargetReticleManager::GetSingleton().GetCurrentTarget();
        }
log::info("IDRC - {}: shout target is {}", __func__, m_shoutTarget ? m_shoutTarget->GetName() : "null");
        RE::Actor* currentCombatTarget = _ts_SKSEFunctions::GetCombatTarget(dragonActor);

        if (!m_shoutTarget) {
            // if no target from TDM or Reticle, use current combat target (if any)
            m_shoutTarget = currentCombatTarget;
        } else if (m_shoutTarget != currentCombatTarget) {
log::info("IDRC - {}: Shout target is different from current combat target, starting combat with shout target", __func__);
            DragonStartCombat(m_shoutTarget);
        }

        while (controlsManager.GetIsKeyPressed(IDRCKey::kSneak)) {
            _ts_SKSEFunctions::WaitWhileGameIsPaused();

            if (controlsManager.GetIsKeyPressed(IDRCKey::kRun)) {
                isAlternateAttack = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto* shoutTarget = m_shoutTarget;
            if (shoutTarget) {
                SKSE::GetTaskInterface()->AddTask([shoutTarget, dragonActor]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    _ts_SKSEFunctions::SetLookAt(dragonActor, shoutTarget, true);
                  });
            }
        }

//        UpdateCombatTargetAlias();

        std::string displayMessage = "Commanding Attack";
        if (m_shoutTarget) {
            // Display attack notification
            displayMessage += " on ";
            displayMessage += std::string(m_shoutTarget->GetName());
        }

        if (dragonActor->HasShout(m_unrelentingForceShout) && (isAlternateAttack || controlsManager.GetIsKeyPressed(IDRCKey::kRun))) {
            SetShoutMode(2); // Set to Unrelenting Force
        } else if (m_shoutTarget && _ts_SKSEFunctions::GetDistance(dragonActor, m_shoutTarget) >= m_maxTargetDistance) {
            SetShoutMode(1); // Set to Ball/Storm
        } else {
            SetShoutMode(0); // Set to Breath
        }

        if (m_attackShout == nullptr) {
            log::error("IDRC - {}: Error: No Shout Found", __func__);            
            return false;
        }

        if (displayManager.GetDisplayFlyingMode() && displayManager.GetDisplayMessages()) {
            RE::SendHUDMessage::ShowHUDMessage(displayMessage.c_str());
        }

        auto* shoutTarget = m_shoutTarget;
        SKSE::GetTaskInterface()->AddTask([this, shoutTarget, dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            if (shoutTarget) {
                _ts_SKSEFunctions::SetLookAt(dragonActor, shoutTarget, true);
            }
            StartVoiceShoutCast(static_cast<RE::Character*>(dragonActor), this->m_attackShout, 2, this->m_shoutTarget);
log::info("IDRC - {}: Started Attack with target {}", __func__, shoutTarget ? shoutTarget->GetName() : "null");
        });

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
    
/*
    void CombatManager::UpdateCombatTargetAlias() {
        log::info("IDRC - {}", __func__);
return;
        auto& dataManager = DataManager::GetSingleton();
        auto* dragonActor = dataManager.GetDragonActor();

        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return;
        }

        RE::Actor* currentCombatTarget =  _ts_SKSEFunctions::GetCombatTarget(dragonActor);

if (currentCombatTarget) {
    log::info("IDRC - {}: Updating to {}", __func__, currentCombatTarget->GetFormID());
} else {
    log::info("IDRC - {}: Clearing target", __func__);
}

        SKSE::GetTaskInterface()->AddTask([this, dragonActor, currentCombatTarget]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            this->m_combatTargetAlias->ForceRefTo(currentCombatTarget);
            dragonActor->EvaluatePackage();

if (m_combatTargetAlias) {
auto* checkActor = m_combatTargetAlias->GetActorReference();
if (checkActor) {
log::info("IDRC - {}: CombatTarget now points to {}", __func__, checkActor->GetFormID());
} else {
log::info("IDRC - {}: CombatTarget is now null", __func__);
}
} else {
log::error("IDRC - {}: No CombatTargetAlias!", __func__);
}
        });
    }
*/
} // namespace IDRC

