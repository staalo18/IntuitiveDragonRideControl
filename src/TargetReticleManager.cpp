#include "TargetReticleManager.h"
#include "APIManager.h"
#include "Offsets.h"
#include "_ts_SKSEFunctions.h"
#include "DataManager.h"
#include "ControlsManager.h"
#include "IDRCUtils.h"

namespace IDRC {
    void TargetReticleManager::Initialize()
    {
        m_isReticleLocked = false;
        m_isWidgetActive = false;
        m_combatState = 0;
        m_reticleTarget = nullptr;
        m_currentTargetMode = TargetMode::kNone;
        
        if (m_isInitialized) {
            log::info("IDRC - {}: CombatTargetReticle already initialized", __func__);
            return;
        }

        if (APIs::TrueHUD) {
            APIs::TrueHUD->LoadCustomWidgets(SKSE::GetPluginHandle(), "IntuitiveDragonRideControl/IDRC_Widgets.swf"sv, [this](TRUEHUD_API::APIResult a_apiResult) {
                if (a_apiResult == TRUEHUD_API::APIResult::OK) {
                    log::info("IDRC - TrueHUD API: CombatTargetReticle loaded successfully.");
                    APIs::TrueHUD->RegisterNewWidgetType(SKSE::GetPluginHandle(), 'IDRC');
                    this->m_isInitialized = true;
                }
            });
        }
    }

    void TargetReticleManager::SetPrimaryTargetMode(TargetMode a_targetMode) {
        m_primaryTargetMode = a_targetMode;
    }

    void TargetReticleManager::SetMaxTargetDistance(float a_maxReticleDistance) {
        m_maxReticleDistance = a_maxReticleDistance;
    }

    void TargetReticleManager::SetDistanceMultiplierSmall(float a_distanceMultiplierSmall) {
        m_distanceMultiplierSmall = a_distanceMultiplierSmall;
    }

    void TargetReticleManager::SetDistanceMultiplierLarge(float a_distanceMultiplierLarge) {
        m_distanceMultiplierLarge = a_distanceMultiplierLarge;
    }

    void TargetReticleManager::SetDistanceMultiplierExtraLarge(float a_distanceMultiplierExtraLarge) {
        m_distanceMultiplierExtraLarge = a_distanceMultiplierExtraLarge;
    }
    void TargetReticleManager::SetMaxTargetScanAngle(float a_maxTargetScanAngle) {
        m_maxTargetScanAngle = a_maxTargetScanAngle;
    }

    void TargetReticleManager::Update()
    {
        if (RE::UI::GetSingleton()->GameIsPaused()) {
            return;
        }

        if (!APIs::TrueHUD) {
            return;
        }

        if (!DataManager::GetSingleton().GetDragonActor()) {
            return;
        }

        if (!m_isInitialized) {
            log::warn("IDRC - {}: CombatTargetReticle not initialized", __func__);
            return;
        }

        UpdateReticle();
    }

    void TargetReticleManager::UpdateReticle() {
        if (!APIs::TrueHUD) {
            log::warn("IDRC - {}: TrueHUD API not available", __func__);
            return;
        }

        if (!m_isInitialized) {
            return;
        }

        if (!m_isEnabled) {
            DisposeReticle();
            return;
        }

        if (IsTDMLocked()) {
            DisposeReticle();
            if (APIs::TrueDirectionalMovement) {
               auto targetHandle = APIs::TrueDirectionalMovement->GetCurrentTarget();
                if (targetHandle) {
                     m_reticleTarget = targetHandle.get().get();
                } else {
                    m_reticleTarget = nullptr;
                }
            }
            return;
        }

        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::warn("IDRC - {}: No dragon actor found", __func__);
            DisposeReticle();
            return;
        }

        if (m_isReticleLocked) {
            if (m_reticleTarget && m_reticleTarget->GetDistance(dragonActor) <= m_maxReticleDistance * GetDistanceRaceSizeMultiplier(m_reticleTarget->GetRace()) && !m_reticleTarget->IsDead()) {
                if(!m_isWidgetActive) {
                    // re-enable reticle in case it was disposed during TDM target lock
                    SetReticleTarget(GetReticleStyle(m_currentTargetMode));
                }
                return;
            } else {
                m_isReticleLocked = false;
                std::string sMessage = "Target reticle unlocked.";
            }
        }

        RE::Actor* currentTarget = m_reticleTarget;        
        int combatState = _ts_SKSEFunctions::GetCombatState(dragonActor);
        auto* selectedActor = GetSelectedActor();        
        RE::Actor* combatTarget = GetCombatTarget();
        
        // Get new target mode after updates
        TargetMode newTargetMode = GetTargetMode((selectedActor != nullptr), (combatTarget != nullptr));
        
        // Determine appropriate target for the new target mode
        RE::Actor* newTarget = nullptr;
        if (newTargetMode == TargetMode::kCombatTarget) {
            newTarget = combatTarget;
            m_combatState = combatState;
        } else if (newTargetMode == TargetMode::kSelectedActor) {
            newTarget = selectedActor;
        } else {
            if (currentTarget != nullptr) {
                DisposeReticle();
            }
            return;
        }

        if ((!m_isWidgetActive && newTarget) ||
                m_currentTargetMode != newTargetMode || 
                currentTarget != newTarget ||
                (newTargetMode == TargetMode::kCombatTarget && m_combatState != combatState))
        {
            m_currentTargetMode = newTargetMode;
            m_reticleTarget = newTarget;
            
            SetReticleTarget(GetReticleStyle(m_currentTargetMode));
        }
    }


    bool TargetReticleManager::IsTDMLocked() {
        bool isTDMLocked = false;
        if (APIs::TrueDirectionalMovement && APIs::TrueDirectionalMovement->GetTargetLockState()) {
            isTDMLocked = true;
        }
        return isTDMLocked;
    }

    RE::Actor* TargetReticleManager::GetCurrentTarget() const {
        RE::Actor* currentTarget = m_reticleTarget;

        if (currentTarget) {
            return currentTarget;
        }

        // In case reticle logic is not active (eg TrueHUD not installed), fall back to selected/combat targets
        auto* selectedActor = GetSelectedActor();
        auto* combatTarget = GetCombatTarget();
        TargetMode targetMode = GetTargetMode((selectedActor != nullptr), (combatTarget != nullptr));

        // Use primary target
        if (targetMode == TargetMode::kCombatTarget) {
            currentTarget = combatTarget;
        }
        else if (targetMode == TargetMode::kSelectedActor) {
            currentTarget = selectedActor;
        }

        // If no primary target, use secondary target
        if (!currentTarget) {
            if (targetMode == TargetMode::kCombatTarget) {
                currentTarget = selectedActor;
            }
            else if (targetMode == TargetMode::kSelectedActor) {
                currentTarget = combatTarget;
            }
        }

        return currentTarget; 
    }

    RE::Actor* TargetReticleManager::GetSelectedActor() const {
        auto* playerActor = RE::PlayerCharacter::GetSingleton();
        auto* processLists = RE::ProcessLists::GetSingleton();
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();

        if (!playerActor) {
            log::error("IDRC - {}: PlayerActor is null", __func__);
            return nullptr;
        }
        if (!processLists) {
            log::error("IDRC - {}: ProcessLists is null", __func__);
            return nullptr;
        }
        if (!playerCamera) {
            log::error("IDRC - {}: PlayerCamera is null", __func__);
            return nullptr;
        }
        if (!dragonActor) {
            log::error("IDRC - {}: DragonActor is null", __func__);
            return nullptr;
        }

        auto cameraPos = playerCamera->cameraRoot->world.translate;
        auto playerPos = playerActor->GetPosition();

        auto root = playerCamera->cameraRoot;
        const auto& worldTransform = root->world; // RE::NiTransform

        // The forward vector is the third column of the rotation matrix
        RE::NiPoint3 cameraForward = worldTransform.rotate * RE::NiPoint3{ 0.0f, 1.0f, 0.0f };
        float cameraToPlayerDistance = playerPos.GetDistance(cameraPos);

        std::vector<RE::Actor*> excludeActors;
        excludeActors.push_back(dragonActor);

        RE::Actor* selectedActor = nullptr;
        for (auto handle : processLists->highActorHandles) {
            auto actor = handle.get().get();
            if (actor
                && std::find(excludeActors.begin(), excludeActors.end(), actor) == excludeActors.end() 
                && actor->Get3D() 
                && !actor->IsDead()
//                && !(actor->GetFactionReaction(playerActor) == RE::FIGHT_REACTION::kAlly)
                && (m_maxReticleDistance <= 0.0f || actor->GetPosition().GetDistance(playerPos) <= m_maxReticleDistance * GetDistanceRaceSizeMultiplier(actor->GetRace()))) {

                RE::NiPoint3 actorPos = actor->GetPosition();
                float fAngleForward = _ts_SKSEFunctions::GetAngleBetweenVectors(actorPos - cameraPos, cameraForward);
                float fAngleBackwards = _ts_SKSEFunctions::GetAngleBetweenVectors(playerPos - actorPos, -cameraForward);
                float distanceToPlayer = actorPos.GetDistance(playerPos);
                
                // forward cone: starting at camerapos, opening towards playerpos with angle m_maxTargetScanAngle
                bool inForwardCone = (fAngleForward <= m_maxTargetScanAngle);
                // backwards cone: starting at player, opening towards camerapos with angle m_maxTargetScanAngle
                bool inBackwardCone = (fAngleBackwards >= 180.f - m_maxTargetScanAngle) && (distanceToPlayer < cameraToPlayerDistance);
                if (inForwardCone || inBackwardCone || m_maxTargetScanAngle <= 0.0f) {
                    if (selectedActor) {
                        if (actor->GetPosition().GetDistance(playerPos) < selectedActor->GetPosition().GetDistance(playerPos)) {
                            selectedActor = actor;
                        }
                    } else {
                        selectedActor = actor;
                    }
                }
            }
        }

        return selectedActor;
    }

    RE::Actor* TargetReticleManager::GetCombatTarget() const {
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            return nullptr;
        }
        RE::Actor* currentCombatTarget = nullptr;
        if (dragonActor->GetActorRuntimeData().currentCombatTarget) {
            currentCombatTarget = dragonActor->GetActorRuntimeData().currentCombatTarget.get().get();
        }
        return currentCombatTarget;
    }

    TargetReticleManager::TargetMode TargetReticleManager::GetTargetMode(bool a_hasSelectedActor, bool a_hasCombatTarget) const {
        TargetMode targetMode = TargetMode::kNone;

        // check for primary target, as defined by m_primaryTargetMode
        if (m_primaryTargetMode == TargetMode::kCombatTarget) {
            targetMode = a_hasCombatTarget ? TargetMode::kCombatTarget : TargetMode::kNone;
        }
        else if (m_primaryTargetMode == TargetMode::kSelectedActor) {
            targetMode = a_hasSelectedActor ? TargetMode::kSelectedActor : TargetMode::kNone;
        }

        // If no primary target found, check for secondary target
        if (targetMode == TargetMode::kNone) {
            if (m_primaryTargetMode == TargetMode::kCombatTarget) {
                targetMode = a_hasSelectedActor ? TargetMode::kSelectedActor : TargetMode::kNone;
            }
            else if (m_primaryTargetMode == TargetMode::kSelectedActor) {
                targetMode = a_hasCombatTarget ? TargetMode::kCombatTarget : TargetMode::kNone;
            }
        }

        return targetMode;
    }

    void TargetReticleManager::ToggleLockReticle() {
        if (IsTDMLocked()) {
            return;
        }
        if (m_reticleTarget || m_isReticleLocked) {
            m_isReticleLocked = !m_isReticleLocked;
            std::string sMessage = "Target reticle " + std::string(m_isReticleLocked ? "locked on " + std::string(m_reticleTarget->GetName()) + "." : "unlocked.");
            RE::DebugNotification(sMessage.c_str());
        } else {
            std::string sMessage = "No target to lock target reticle on.";
            RE::DebugNotification(sMessage.c_str());
        }
    }

    void TargetReticleManager::TogglePrimaryTargetMode() {
        if (m_primaryTargetMode == TargetMode::kCombatTarget) {
            m_primaryTargetMode = TargetMode::kSelectedActor;
            RE::DebugNotification("Primary target: picked from screen center.");
        } else if (m_primaryTargetMode == TargetMode::kSelectedActor) {
            m_primaryTargetMode = TargetMode::kCombatTarget;
            RE::DebugNotification("Primary target: the dragon's current combat target.");
        } else {
            m_primaryTargetMode = TargetMode::kSelectedActor;
        }
    }    

    void TargetReticleManager::DisposeReticle() {
        if (!APIs::TrueHUD) {
            log::warn("IDRC - {}: TrueHUD API not available", __func__);
            return;
        }

        m_reticleTarget = nullptr;

        auto widget = m_TargetReticle.lock();
        if (!widget) {
            return;
        }
        
        APIs::TrueHUD->RemoveWidget(SKSE::GetPluginHandle(), 'IDRC', 0, TRUEHUD_API::WidgetRemovalMode::Normal);
        m_isWidgetActive = false;
    }
/*
    void TargetReticleManager::ShowReticle(bool a_show) {
        if (!APIs::TrueHUD) {
            log::info("IDRC - {}: TrueHUD API not available", __func__);
            return;
        }

        auto widget = m_TargetReticle.lock();
        if (!widget) {
            log::info("IDRC - {}: Reticle does not exist", __func__);
            return;
        }

log::info("IDRC - {}: Show: {}", __func__, a_show);
        widget->SetVisible(a_show);
    }
*/
    void TargetReticleManager::SetReticleTarget(CombatTargetReticle::ReticleStyle a_reticleStyle) {
        if (!APIs::TrueHUD) {
            log::warn("IDRC - {}: TrueHUD API not available", __func__);
            return;
        }

        if (!m_reticleTarget) {
            DisposeReticle();
            return;
        }

        auto actorHandle = m_reticleTarget->GetHandle();
        if (!actorHandle) {
            log::warn("IDRC - {}: Actor handle is invalid", __func__);
            return;
        }

        auto targetPoint = GetTargetPoint(m_reticleTarget);
        if (!targetPoint) {
            log::warn("IDRC - {}: Target point is nullptr", __func__);
            return;
        }

        auto widget = m_TargetReticle.lock();
        if (widget) {
            widget->UpdateReticleStyle(a_reticleStyle);
            widget->ChangeTarget(actorHandle, targetPoint);
        } else {
            widget = std::make_shared<CombatTargetReticle>(actorHandle.native_handle(), actorHandle, targetPoint, a_reticleStyle);
            m_TargetReticle = widget;
            APIs::TrueHUD->AddWidget(SKSE::GetPluginHandle(), 'IDRC', 0, "TDM_TargetLockReticle", widget);
        }
        m_isWidgetActive = true;
    }

    RE::NiPointer<RE::NiAVObject> TargetReticleManager::GetTargetPoint(RE::Actor* a_actor) const {
        RE::NiPointer<RE::NiAVObject> targetPoint = nullptr;

        if (!a_actor) {
            return nullptr;
        }

        auto race = a_actor->GetRace();
        if (!race) {
            return nullptr;
        }

        RE::BGSBodyPartData* bodyPartData = race->bodyPartData;
        if (!bodyPartData) {
            return nullptr;
        }

        auto actor3D = a_actor->Get3D2();
        if (!actor3D) {
            return nullptr;
        }
    
        RE::BGSBodyPart* bodyPart = bodyPartData->parts[RE::BGSBodyPartDefs::LIMB_ENUM::kHead];
        if (!bodyPart) {
            bodyPart = bodyPartData->parts[RE::BGSBodyPartDefs::LIMB_ENUM::kTorso];
        }
        if (!bodyPart) {
            bodyPart = bodyPartData->parts[RE::BGSBodyPartDefs::LIMB_ENUM::kTotal];
        }
        if (bodyPart) {
            targetPoint = RE::NiPointer<RE::NiAVObject>(NiAVObject_LookupBoneNodeByName(actor3D, bodyPart->targetName, true));
        }

        return targetPoint;
    }

    CombatTargetReticle::ReticleStyle TargetReticleManager::GetReticleStyle(TargetMode a_targetMode) const {
        CombatTargetReticle::ReticleStyle reticleStyle;
        if (a_targetMode == TargetMode::kSelectedActor) {
            reticleStyle = CombatTargetReticle::ReticleStyle::kSelectedActor;
        } else if (a_targetMode == TargetMode::kCombatTarget) {
            if (m_combatState == 1) {
                reticleStyle = CombatTargetReticle::ReticleStyle::kCombatTargetFound;
           } else {
                reticleStyle = CombatTargetReticle::ReticleStyle::kCombatTargetSearching;
            }
        } else {
            reticleStyle = CombatTargetReticle::ReticleStyle::kSelectedActor;
            log::warn("IDRC - {}: Invalid ReticleType", __func__);
        }
        return reticleStyle;
    }

    // GetDistanceRaceSizeMultiplier() provides consistency with True Directional Movement's settings
    // Corresponding TDM function is DirectionalMovementHandler::GetTargetLockDistanceRaceSizeMultiplier()
    // see https://github.com/ersh1/TrueDirectionalMovement
    float TargetReticleManager::GetDistanceRaceSizeMultiplier(RE::TESRace* a_race) const {
        if (a_race) {
            switch (a_race->data.raceSize.get())
            {
            case RE::RACE_SIZE::kMedium:
            default:
                return 1.f;
            case RE::RACE_SIZE::kSmall:
                return m_distanceMultiplierSmall;
            case RE::RACE_SIZE::kLarge:
                return m_distanceMultiplierLarge;
            case RE::RACE_SIZE::kExtraLarge:
                return m_distanceMultiplierExtraLarge;
            }
        }

        return 1.f;
    }
}  // namespace IDRC
