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
        m_registeredForTargetSync = false;
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

        SetStopCombat(false);
        bool bAttackNotificationDisplayed = false;
        bool bTargetFromTDM = false;
        
        if (APIs::TrueDirectionalMovementV1 && APIs::TrueDirectionalMovementV1->GetTargetLockState()
            && !TargetReticleManager::GetSingleton().IsReticleLocked()) {
            auto currentTarget = APIs::TrueDirectionalMovementV1->GetCurrentTarget();
            if (currentTarget) {
                log::info("IDRC - {}: Getting target from TDM: {} ({})", __func__, currentTarget.get()->GetName(), currentTarget.get()->GetFormID());
                if (displayManager.GetDisplayFlyingMode() && displayManager.GetDisplayMessages()) {
                    RE::SendHUDMessage::ShowHUDMessage((std::string("Commanding Attack on ") + std::string(currentTarget.get()->GetName())).c_str());
                    bAttackNotificationDisplayed = true;
                }

//if (APIs::TrueDirectionalMovementV4) {
//bool isBehind = APIs::TrueDirectionalMovementV4->IsTargetLockBehindTarget();
//log::info("IDRC - {}: TDM Target Lock is behind target: {}", __func__, isBehind);
//}
                DragonStartCombat(currentTarget.get()->As<RE::Actor>());
                bTargetFromTDM = true;
            }
        }

        // get the combat target from the reticle (if active), and ensure dragon is in combat with that target
        RE::Actor* combatTarget = TargetReticleManager::GetSingleton().GetCurrentTarget();
        if (combatTarget && !bTargetFromTDM) {
            if (!bAttackNotificationDisplayed && displayManager.GetDisplayFlyingMode() && displayManager.GetDisplayMessages()) {
                RE::SendHUDMessage::ShowHUDMessage((std::string("Commanding Attack on ") + std::string(combatTarget->GetName())).c_str());
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

        if (!SyncCombatTarget()) {
            // don't start the attack if the CombatTarget alias cannot be sync'ed!
            return false;
        }

        // Final update in case the dragon has switched the combat target again (or target died) while resetting fast travel...
        target = _ts_SKSEFunctions::GetCombatTarget(dragonActor);

        std::string displayMessage = "Commanding Attack";
        if (target) {
            // Display attack notification
            displayMessage += " on ";
            displayMessage += std::string(target->GetName());
        }

        if (dragonActor->HasShout(m_unrelentingForceShout) && (isAlternateAttack || controlsManager.GetIsKeyPressed(IDRCKey::kRun))) {
            SetShoutMode(2); // Set to Unrelenting Force
        } else if (target && _ts_SKSEFunctions::GetDistance(dragonActor, target) >= m_maxTargetDistance) {
            SetShoutMode(1); // Set to Ball/Storm
        } else {
            SetShoutMode(0); // Set to Breath
        }

        if (m_attackShout == nullptr) {
            log::error("IDRC - {}: Error: No Shout Found", __func__);            
            return false;
        }


        if (!bAttackNotificationDisplayed && displayManager.GetDisplayFlyingMode() && displayManager.GetDisplayMessages()) {
            RE::SendHUDMessage::ShowHUDMessage(displayMessage.c_str());
            bAttackNotificationDisplayed = true;
        }

        SKSE::GetTaskInterface()->AddTask([this, dragonActor, target]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            if (target) {
                _ts_SKSEFunctions::SetLookAt(dragonActor, target, true);
            }
            StartVoiceShoutCast(static_cast<RE::Character*>(dragonActor), this->m_attackShout, 2, target);
log::info("IDRC - {}: Started Attack with target {}", __func__, target ? target->GetName() : "null");
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
    

    bool CombatManager::SyncCombatTarget() {
        log::info("IDRC - {}: {}", __func__, m_registeredForTargetSync);

//return true; // TODO - temp, to test attack without syncing the alias first

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

} // namespace IDRC

