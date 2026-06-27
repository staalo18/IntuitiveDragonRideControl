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
#include "IDRCUtils.h"

namespace IDRC {   

    void CombatManager::InitializeData(RE::BGSListForm* a_breathShoutList, 
                                RE::BGSListForm* a_ballShoutList,
                                RE::TESShout* a_unrelentingForceShout,
                                RE::TESShout* a_attackShout) {
        log::info("IDRC - {}", __func__);
        SetBreathShoutList(a_breathShoutList);
        SetBallShoutList(a_ballShoutList);
        m_unrelentingForceShout = a_unrelentingForceShout;
        m_attackShout = a_attackShout;
    }

    float CombatManager::GetMaxTargetDistance() {
        return m_maxTargetDistance;
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

        if (Utils::GetHorizontalDistance(dragonActor, a_target) > m_maxCombatDistance ||
            !a_target->GetParentCell() || !a_target->GetParentCell()->IsAttached()) {
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
    
    void CombatManager::Update() {
         if (RE::UI::GetSingleton()->GameIsPaused()) {
            return;
        }

        if (!DataManager::GetSingleton().GetDragonActor()) {
            return;
        }

        UpdateCombat();

        UpdateAttack();
    }

    void CombatManager::UpdateCombat() {

        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            return;
        }

        // restart combat with stored target when dragon leaves flying state.
        if (FlyingModeManager::GetSingleton().GetFlyingMode() != FlyingMode::kFlying &&
            _ts_SKSEFunctions::GetFlyingState(dragonActor) != 2 &&
            !_ts_SKSEFunctions::IsFlyingMountPatrolQueued(dragonActor) && 
            !_ts_SKSEFunctions::IsFlyingMountFastTravelling(dragonActor)) 
        {
            if (m_restartCombatPending)
            {
                m_restartCombatPending = false;            

                auto* storedTarget = m_storedCombatTarget ? m_storedCombatTarget.get().get() : nullptr;
                if (storedTarget && !dragonActor->IsInCombat()) {
log::info("IDRC - {}: ------------------->>>>>>> Restarting combat with stored target {}", __func__, storedTarget->GetName());
                    DragonStartCombat(storedTarget);
                }
            }

            auto* ct = _ts_SKSEFunctions::GetCombatTarget(dragonActor);
            m_storedCombatTarget = ct ? ct->GetHandle() : RE::ActorHandle{};
        } else {
            m_restartCombatPending = true;

 //           UpdatePlayerCell();

            if (m_shoutTarget) {
                m_storedCombatTarget = m_shoutTarget;
            }
        }

        // Clear combat targets (ie stop combat) if the dragon is too far from the combat target
        if (dragonActor->IsInCombat()) {
            auto* combatTarget = _ts_SKSEFunctions::GetCombatTarget(dragonActor);
    
            if (combatTarget && 
                (
                    Utils::GetHorizontalDistance(dragonActor, combatTarget) > m_maxCombatDistance ||
                    !combatTarget->GetParentCell() || !combatTarget->GetParentCell()->IsAttached()
                )) {
log::info("IDRC - {}: distance: {}, ParentCell: {}, attached: {}", __func__, Utils::GetHorizontalDistance(dragonActor, combatTarget), combatTarget->GetParentCell() ? "Yes" : "null", combatTarget->GetParentCell() ? (combatTarget->GetParentCell()->IsAttached() ? "Yes" : "No") : "null");                
                // In case the dragon is in combat, the game's 3D data  is centered around the dragon's combat target (ie the combat target's cell and its 8 adjacent cells).
                // If the mounted dragon is in a different cell in the landscape, 
                // the 3D area of the player's/dragon's location can get unloaded (player gets "detached" from cell).
                // This then causes a lot of issues, like:
                // 		skyshots, grey screen, "You cannot go that way", frozen player in mid-air (while dragon flys on), 
                // 		teleported dragon, disappearing dragon, only LOD landscape visible, crashes, etc
                // To prevent this, we clear combat targets (stop combat).
                SKSE::GetTaskInterface()->AddTask([dragonActor]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                _ts_SKSEFunctions::ClearCombatTargets(dragonActor);
log::info("IDRC - {}: ------------------->>>>>>> Cleared CombatTargets due to distance or target not loaded", __func__);
                });
            }
        }
    }

    constexpr float CELL_SIZE = 4096.0f;

    void CombatManager::UpdatePlayerCell() { 
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            log::warn("{}: Cannot access player character", __FUNCTION__);
            return;
        } 

        auto* tes = RE::TES::GetSingleton();
        if (!tes) {
            log::warn("{}: Cannot access TES", __FUNCTION__);
            return;
        }

        RE::TESObjectCELL* cell = nullptr;
        auto* worldspace = tes->GetRuntimeData2().worldSpace;
        if (!worldspace) {
            log::warn("{}: Cannot access worldspace", __FUNCTION__);
            return;
        }

        auto playerPos = player->GetPosition();

        // Calculate cell coordinates from position
        std::int16_t targetCellX = static_cast<std::int16_t>(std::floor(playerPos.x / CELL_SIZE));
        std::int16_t targetCellY = static_cast<std::int16_t>(std::floor(playerPos.y / CELL_SIZE));
        
        RE::CellID cellID(targetCellY, targetCellX);

        // First check if cell is already in the cellMap
        const auto& map = worldspace->cellMap;
        const auto it = map.find(cellID);
        if (it != map.end()) {
            cell = it->second;
        }

        // If not in map, load it using TESWorldSpace_LoadCell
        if (!cell) {
            bool loadFromDisk;
            cell = _ts_SKSEFunctions::GetCell(targetCellX, targetCellY, worldspace, loadFromDisk);
        }

        if (cell) {
            player->SetParentCell(cell);
log::info("{}: Updated player cell to {}, {}", __FUNCTION__, targetCellX, targetCellY);
        } else {
            log::error("{}: No cell!", __FUNCTION__);
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
            m_shoutTarget = RE::ActorHandle{};
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

        if (m_shoutActive) {
            log::info("IDRC - {}: Shout is ongoing, canceling new attack", __func__);
            return false;
        }

        bool isAlternateAttack = a_alternateAttack;
        m_shoutTimer = 0.0f;
        m_shoutActive = true;
                 
        m_shoutTarget = RE::ActorHandle{};
        if (APIs::TrueDirectionalMovementV1 && APIs::TrueDirectionalMovementV1->GetTargetLockState()
            && !TargetReticleManager::GetSingleton().IsReticleLocked()) {
            auto currentTDMTarget = APIs::TrueDirectionalMovementV1->GetCurrentTarget();
            if (currentTDMTarget) {
                m_shoutTarget = currentTDMTarget;
            }
        }

        if (!m_shoutTarget) {
            // No TDM target, get the combat target from the reticle (if active)
log::info("IDRC - {}: No TDM target, checking Target Reticle Manager for target", __func__);
            if (auto handle = TargetReticleManager::GetSingleton().GetCurrentTarget()) {
                m_shoutTarget = handle;
            }
        }
        auto* resolvedShoutTarget = m_shoutTarget ? m_shoutTarget.get().get() : nullptr;
log::info("IDRC - {}: shout target is {}", __func__, resolvedShoutTarget ? resolvedShoutTarget->GetName() : "null");
        RE::Actor* currentCombatTarget = _ts_SKSEFunctions::GetCombatTarget(dragonActor);

        if (!m_shoutTarget) {
            // if no target from TDM or Reticle, use current combat target (if any)
            m_shoutTarget = currentCombatTarget ? currentCombatTarget->GetHandle() : RE::ActorHandle{};
            resolvedShoutTarget = currentCombatTarget;
        } else if (resolvedShoutTarget != currentCombatTarget) {
log::info("IDRC - {}: Shout target is different from current combat target, starting combat with shout target", __func__);
            DragonStartCombat(resolvedShoutTarget);
        }

        while (controlsManager.GetIsKeyPressed(IDRCKey::kSneak)) {
            _ts_SKSEFunctions::WaitWhileGameIsPaused();

            if (controlsManager.GetIsKeyPressed(IDRCKey::kRun)) {
                isAlternateAttack = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto* shoutTarget = m_shoutTarget ? m_shoutTarget.get().get() : nullptr;
            if (shoutTarget) {
                SKSE::GetTaskInterface()->AddTask([shoutTarget, dragonActor]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    _ts_SKSEFunctions::SetLookAt(dragonActor, shoutTarget, true);
                  });
            }
        }

        std::string displayMessage = "Commanding Attack";
        if (resolvedShoutTarget) {
            // Display attack notification
            displayMessage += " on ";
            displayMessage += std::string(resolvedShoutTarget->GetName());
        }

        if (dragonActor->HasShout(m_unrelentingForceShout) && (isAlternateAttack || controlsManager.GetIsKeyPressed(IDRCKey::kRun))) {
            SetShoutMode(2); // Set to Unrelenting Force
        } else if (resolvedShoutTarget && _ts_SKSEFunctions::GetDistance(dragonActor, resolvedShoutTarget) >= m_maxTargetDistance) {
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

        auto* shoutTarget = resolvedShoutTarget;
        SKSE::GetTaskInterface()->AddTask([this, shoutTarget, dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            if (shoutTarget) {
                _ts_SKSEFunctions::SetLookAt(dragonActor, shoutTarget, true);
            }
            auto* taskShoutTarget = this->m_shoutTarget ? this->m_shoutTarget.get().get() : nullptr;
            StartVoiceShoutCast(static_cast<RE::Character*>(dragonActor), this->m_attackShout, 2, taskShoutTarget);
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
} // namespace IDRC

