#include "FlyingModeManager.h"
#include "_ts_SKSEFunctions.h"
#include "CombatManager.h"
#include "DataManager.h"
#include "DisplayManager.h"
#include "FastTravelManager.h"
#include "IDRCUtils.h"
#include "APIManager.h"

#include "RE/Skyrim.h"
#include "SKSE/API.h"
#include "CLIBUtil/EditorID.hpp"


namespace IDRC {

    void FlyingModeManager::InitializeData(RE::TESObjectREFR* a_dragonTurnMarker, 
                                    RE::TESObjectREFR* a_dragonTravelToMarker,
                                    RE::TESObjectREFR* a_flyToTargetMarker,
                                    RE::SpellItem* a_noFlyAbility) {
        log::info("IDRC - {}", __func__);
        m_dragonTurnMarker = a_dragonTurnMarker;
        m_dragonTravelToMarker = a_dragonTravelToMarker;
        m_flyToTargetMarker = a_flyToTargetMarker;
        m_noFlyAbility = a_noFlyAbility;

        ResetDragonHeight();
        m_continueFlyTo = false;
        m_flyToAngle = 0.0;
        m_turnSpeed = 40.0;
        m_toggleAlwaysRun = true;
        m_registeredForLanding = false;
        m_registeredForPerch = false;
        m_toggledAutoCombatLand = false;
        m_skipOrbiting = true; // Always skip orbiting
        m_vanillaAttack = false;
        m_mode = kLanded;
    }

    void FlyingModeManager::SetMinHeight(float a_minHeight) {
        m_minHeight = a_minHeight;
    }

    float FlyingModeManager::GetMinHeight() {
        return m_minHeight;
    }

    void FlyingModeManager::ResetDragonHeight() {
        m_minHeight = 1000.0f;
        m_maxHeight = 1000.0f;
        m_arrivalHeight = 1000.0f;
    
        SKSE::GetTaskInterface()->AddTask([this]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            _ts_SKSEFunctions::UpdateIniSetting("fPlayerFlyingMountTravelMinHeight:General", this->m_minHeight);
            _ts_SKSEFunctions::UpdateIniSetting("fPlayerFlyingMountTravelMaxHeight:General", this->m_maxHeight);
            _ts_SKSEFunctions::UpdateIniSetting("fFlyingMountFastTravelArrivalHeight:General", this->m_arrivalHeight);
        });    }

    void FlyingModeManager::ChangeDragonHeight(float a_upDown, bool a_isAbsoluteValue) {
//        log::info("IDRC - {}", __func__);
    
        // Calculate the height change
        float changeHeight = a_upDown;
        if (!a_isAbsoluteValue) {
            changeHeight *= 50.0f;
        }
        m_minHeight += changeHeight * GetRunFactor();
    
        // Clamp m_minHeight to the valid range [100, 10000]
        if (m_minHeight < 100.0f) {
            m_minHeight = 100.0f;
        } else if (m_minHeight > 10000.0f) {
            m_minHeight = 10000.0f;
        }
    
        // Synchronize other height-related variables
        m_maxHeight = m_minHeight;
        m_arrivalHeight = m_minHeight;

        SKSE::GetTaskInterface()->AddTask([this]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            _ts_SKSEFunctions::UpdateIniSetting("fPlayerFlyingMountTravelMinHeight:General", this->m_minHeight);
            _ts_SKSEFunctions::UpdateIniSetting("fPlayerFlyingMountTravelMaxHeight:General", this->m_maxHeight);
            _ts_SKSEFunctions::UpdateIniSetting("fFlyingMountFastTravelArrivalHeight:General", this->m_arrivalHeight);
        });
    
        // Get the dragon actor
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: Dragon actor is null", __func__);
            return;
        }
    
        auto* orbitMarker = DataManager::GetSingleton().GetOrbitMarker();

        // Update FastTravel if the dragon is allowed to fly
        if (dragonActor->AsActorState()->actorState2.allowFlying) {
            // Move the orbit marker vertically by the height change
            if (orbitMarker) {
                SKSE::GetTaskInterface()->AddTask([orbitMarker, changeHeight]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
//                    _ts_SKSEFunctions::MoveTo(orbitMarker, orbitMarker, 0.0f, 0.0f, changeHeight);
                });
            }
/*    
            // Update the dragon's direction
            SKSE::GetTaskInterface()->AddTask([this, dragonActor]() {
                this->DragonNewDirection(dragonActor->GetAngleZ());
            }); */
        }
    }

    RE::TESObjectREFR* FlyingModeManager::GetDragonTurnMarker() {
        return m_dragonTurnMarker;
    }

    FlyingMode FlyingModeManager::GetFlyingMode() {
        return m_mode;
    }

    void FlyingModeManager::SetFlyingModeFromPapyrus(int a_flyingState) {
        m_mode = static_cast<FlyingMode>(a_flyingState);
        log::info("IDRC - {}: {}", __func__, a_flyingState);
    }

    void FlyingModeManager::SetFlyingMode(FlyingMode a_mode) {
        m_mode = a_mode;

        auto handle = _ts_SKSEFunctions::GetHandle(DataManager::GetSingleton().GetRideQuest());
        if(!handle){
            log::error("IDRC - {}: Quest handle is null", __func__);
            return;
        }
        int mode = static_cast<int>(m_mode);
        auto* args = RE::MakeFunctionArguments((int)mode);
        SKSE::GetTaskInterface()->AddTask([handle, args, mode]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            DataManager::GetSingleton().SendPropertyUpdateEvent("FlyingState", false, 0.0f, mode);
        });
    }

    float FlyingModeManager::GetRunFactor(float a_modifier) {
        bool run = false;

        // Check if the run key is pressed
        if (ControlsManager::GetSingleton().GetIsKeyPressed(IDRCKey::kRun)) {
            run = true;
        }

        // Toggle the run state if "Always Run" is enabled
        if (m_toggleAlwaysRun) {
            run = !run;
        }

        // Calculate the run factor
        float runFactor = run ? (2.0f * a_modifier) : a_modifier;

        return runFactor;
    }

    void FlyingModeManager::UpdateFlyingMode() {
        log::info("IDRC - {}", __func__);
    
        if (m_registeredForLanding || CombatManager::GetSingleton().IsAttackOngoing()) {
            log::info("IDRC - {}: Registered for Landing or Attack is ongoing - cancel update", __func__);
            return;
        }
    
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: Dragon actor is null", __func__);
            return;
        }
    
        int currentFlyingState = _ts_SKSEFunctions::GetFlyingState(dragonActor);
    
        if (currentFlyingState == 0 || currentFlyingState == 4) { // Landed or landing
            DragonLandPlayerRiding(dragonActor, false);
        } else {
            DragonHoverPlayerRiding(dragonActor, false);
        }
    }

    void FlyingModeManager::ToggleAutoCombat() {
        if (m_registeredForLanding || CombatManager::GetSingleton().IsAttackOngoing()) {
            // Autocombat is always turned off during landing and commanded attacks -  do not allow to toggle
            return;
        }
    
        auto& dataManager = DataManager::GetSingleton();
        dataManager.ToggleAutoCombat();
        log::info("IDRC - {}: AutoCombat toggled to {}", __func__, dataManager.GetAutoCombat());
    
        // Display notification if flying mode display is enabled
        if (DisplayManager::GetSingleton().GetDisplayFlyingMode()) {
            if (dataManager.GetAutoCombat()) {
                RE::DebugNotification("Combat - Auto");
            } else {
                RE::DebugNotification("Combat - Manual");
            }
        }
    
        auto* dragonActor = dataManager.GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: Dragon actor is null", __func__);
            return;
        }
    
        SKSE::GetTaskInterface()->AddTask([dragonActor]() {
        // When modifying Game objects, send task to TaskInterface to ensure thread safety
            dragonActor->EvaluatePackage();
        });
    
        if (!dataManager.GetAutoCombat() && _ts_SKSEFunctions::GetCombatState(dragonActor) > 0 && dragonActor->IsBeingRidden()) {
            UpdateFlyingMode();
        }
    }

    void FlyingModeManager::OnKeyDown(IDRCKey a_key) {
        if (RE::UI::GetSingleton()->GameIsPaused()) {
            return; // Exit if the game is in menu mode
        }
    
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: Dragon actor is null", __func__);
            return;
        }
    
        auto& controlsManager = ControlsManager::GetSingleton();
        auto& combatManager = CombatManager::GetSingleton();
        auto& dataManager = DataManager::GetSingleton();
        auto& displayManager = DisplayManager::GetSingleton();
        log::info("IDRC - {}: OnKeyDown: keycode = {}, controlBlocked = {}, registeredForLanding = {}", 
                  __func__, a_key, controlsManager.GetControlBlocked(), m_registeredForLanding);
    
        // Handle Activate Key
        if (a_key == kActivate && _ts_SKSEFunctions::GetFlyingState(dragonActor) != 0 && 
                 _ts_SKSEFunctions::GetFlyingState(dragonActor) != 5) {
            SetRegisteredForLanding(false);
            SetRegisteredForPerch(false);
            controlsManager.SetControlBlocked(false);
            DragonHoverPlayerRiding(dragonActor, false);
        }
    
        // Handle Toggle Always Run Key
        if (a_key == kToggleAlwaysRun) {
            m_toggleAlwaysRun = !m_toggleAlwaysRun;

            if (displayManager.GetDisplayFlyingMode()) {
                if (m_toggleAlwaysRun) {
                    RE::DebugNotification("Fast Mode - On");
                } else {
                    RE::DebugNotification("Fast Mode - Off");
                }
            }
        } else if (a_key == kToggleAutoCombat) {
            ToggleAutoCombat();
    
            if (dragonActor->IsBeingRidden()) {
                dragonActor->EvaluatePackage();
            }
        } else if (a_key == kJump) {
            m_vanillaAttack = !m_vanillaAttack;
        } else if (a_key == kSneak) {
            if (!combatManager.GetAttackDisabled() || combatManager.GetTriggerAttack()) {
                combatManager.DragonAttack(controlsManager.GetIsKeyPressed(kRun));
            } else {
                log::info("IDRC - {}: Attack Disabled", __func__);
            }
        }
    
        if (a_key == kForward || a_key == kStrafeLeft || a_key == kStrafeRight) {
            SetContinueFlyTo(false);
    
            if (m_registeredForLanding && a_key == kForward) {
                if (CancelDragonLandPlayerRiding()) {
                    RE::DebugNotification("Commanding Hovering Mode - Landing cancelled");
                    controlsManager.SetControlBlocked(false);
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
        }
    
        bool flyingModeNotification = false;
        WorldSpaceData worldSpaceData; 
        worldSpaceData = WorldSpaceData(dragonActor->GetWorldspace()); 
        if (!controlsManager.GetControlBlocked() && IsInBorderRegion(worldSpaceData.m_borderRegionName)) {
            if (dragonActor->IsBeingRidden()) {
                if ((a_key == kForward || a_key == kBack) && 
                        dataManager.GetAutoCombat() && _ts_SKSEFunctions::GetCombatState(dragonActor) > 0) {
                    UpdateFlyingMode();
                }
    
                if (a_key == kForward) {
                    if (m_mode == kLanded && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 0) { // Landed
                        while (controlsManager.GetIsKeyPressed(a_key)) {
                            if (!(controlsManager.GetIsKeyPressed(kStrafeLeft) || controlsManager.GetIsKeyPressed(kStrafeRight))) {
                                RE::NiPoint3 forwardVector{ 0.f, 1.f, 0.f };
                                float angle = _ts_SKSEFunctions::GetAngleZ(dragonActor->GetPosition() - m_dragonTurnMarker->GetPosition(), forwardVector);
                                angle = _ts_SKSEFunctions::NormalRelativeAngle(angle - dragonActor->GetAngleZ());
                                PlaceTravelToMarker(dragonActor, 500.0f, angle, 0.0f);
                                DragonTravelTo(m_dragonTravelToMarker);
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(500));
                        }
                    } else {
                        FlyingModeUp(a_key);
                        flyingModeNotification = true;
                    }
                } else if (a_key == kBack) {
                    if ((m_mode == kLanded && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 0) || 
                        (m_mode == kPerching && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 5)) {
                            FlyingModeUp(a_key);
                    } else {
                        FlyingModeDown();
                    }
                    flyingModeNotification = true;
                } else if (a_key == kStrafeLeft || a_key == kStrafeRight) {
                    flyingModeNotification = TriggerTurn();
                } else if (a_key == IDRCKey::kUp) {
                    if (_ts_SKSEFunctions::GetFlyingState(dragonActor) == 2 && 
                            (m_mode == kFlying || m_mode == kOrbiting)) { // Orbiting or flying
                        while (controlsManager.GetIsKeyPressed(kUp) && m_minHeight < 10000) {
                            ChangeDragonHeight(1.f);
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                    } else if ((m_mode == kLanded && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 0) || 
                               (m_mode == kPerching && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 5)) { // Landed or perching
                        FlyingModeUp(a_key);
                        flyingModeNotification = true;
                    } else if (m_mode == kHovering && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 3) { // Hovering
                        FlyingModeUp(a_key);
                        flyingModeNotification = true;
                    }
                } else if (a_key == kDown && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 2 
                            && (m_mode == kFlying || m_mode == kOrbiting)) { // Orbiting or flying
                    while (controlsManager.GetIsKeyPressed(kDown) && m_minHeight > 100) {
                        ChangeDragonHeight(-1.f);
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                } else if (a_key == kDisplayHealth) {
                    displayManager.DisplayDragonHealth();
                }
            }
        }
    
        _ts_SKSEFunctions::ClearLookAt(dragonActor);
        _ts_SKSEFunctions::SetLookAt(dragonActor, dragonActor, false);
    }


    bool FlyingModeManager::FlyingModeUp(IDRCKey a_key) {
        log::info("IDRC - {}", __func__);
    
        if (!GetRegisteredForLanding() && !GetRegisteredForPerch()) {
            auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
            if (!dragonActor) {
                log::error("IDRC - {}: Dragon actor is null", __func__);
                return false;
            }
    
            auto& controlsManager =ControlsManager::GetSingleton();
            FlyingMode mode = GetFlyingMode();
    
            while (controlsManager.GetIsKeyPressed(a_key)) {
                float wait = 0.75f;
                if (!(controlsManager.GetIsKeyPressed(IDRCKey::kStrafeLeft) || controlsManager.GetIsKeyPressed(IDRCKey::kStrafeRight))) {
                    controlsManager.SetControlBlocked(true);

                    if (mode == FlyingMode::kFlying && !_ts_SKSEFunctions::IsFlyingMountFastTravelling(dragonActor)) {
                        DragonNewDirection(dragonActor->GetAngleZ(), false);
                        wait = 0.1f;
                    } else if (mode == FlyingMode::kOrbiting && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 2) {
                        mode = FlyingMode::kFlying;
                        DragonNewDirection(dragonActor->GetAngleZ(), true);
                        wait = 0.1f;
                    } else if (mode == FlyingMode::kHovering && dragonActor->AsActorState()->actorState2.allowFlying) {
                        if (m_skipOrbiting) {
                            mode = FlyingMode::kFlying;
                            DragonNewDirection(dragonActor->GetAngleZ(), true);
                            wait = 0.1f;
                        } else {
                            mode = FlyingMode::kOrbiting;
                            DragonOrbitPlayerRiding(RE::PlayerCharacter::GetSingleton());
                            wait = 0.75f;
                        }
                    } else if ((mode == FlyingMode::kLanded && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 0) ||
                               (mode == FlyingMode::kPerching && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 5)) {
                        mode = FlyingMode::kHovering;
                        wait = 0.75f;
                        if (!_ts_SKSEFunctions::IsFlying(dragonActor)) {
                            DragonTakeOffPlayerRiding(dragonActor);
                        } else {
                            DragonHoverPlayerRiding(dragonActor, true);
                        }
                    }
                } else {
                    break;
                }
    
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(wait * 1000)));
            }
    
            while (controlsManager.GetIsKeyPressed(IDRCKey::kUp) && mode == FlyingMode::kFlying && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 2 && m_minHeight < 10000) {
                ChangeDragonHeight(1.f);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
    
            controlsManager.SetControlBlocked(false);
        }

        return true;
    }

    bool FlyingModeManager::FlyingModeDown() {
        log::info("IDRC - {}", __func__);
    
        if (!GetRegisteredForLanding() && !GetRegisteredForPerch()) {
            auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
            if (!dragonActor) {
                log::error("IDRC - {}: Dragon actor is null", __func__);
                return false;
            }

            auto& controlsManager = ControlsManager::GetSingleton();
            FlyingMode mode = GetFlyingMode();
   
            while (controlsManager.GetIsKeyPressed(IDRCKey::kBack)) {
                if (!(controlsManager.GetIsKeyPressed(IDRCKey::kStrafeLeft) || controlsManager.GetIsKeyPressed(IDRCKey::kStrafeRight))) {
                    controlsManager.SetControlBlocked(true);
    
                    if (mode == FlyingMode::kFlying && !m_skipOrbiting) {
                        mode = FlyingMode::kOrbiting;
                        DragonOrbitPlayerRiding(RE::PlayerCharacter::GetSingleton());
                    } else if ((mode == FlyingMode::kFlying && m_skipOrbiting) || mode == FlyingMode::kOrbiting) {
                        mode = FlyingMode::kHovering;
    
                        std::thread([this, dragonActor]() {
                            // send to new thread so that FlyingModeDown keeps progressing while Hover is ongoing
                            this->DragonHoverPlayerRiding(dragonActor);
                        }).detach();
                    
                    } else if (mode == FlyingMode::kHovering) {
                        if (controlsManager.GetIsKeyPressed(IDRCKey::kRun)) {
                            DragonPerchPlayerRiding();
                            mode = FlyingMode::kPerching;
                        } else if (DragonLandPlayerRiding(dragonActor)) {
                            mode = FlyingMode::kLanded;
                        }
                    } else if (mode == FlyingMode::kLanded) {
                        // fallback in case FlyingMode logic got derailed
                        // eg if dragon is flying, but FlyingMode is kLanded and IsAllowedToFly() is false
                        DragonLandPlayerRiding(dragonActor);
                    }
                }
    
                std::this_thread::sleep_for(std::chrono::milliseconds(750));
            }
    
        controlsManager.SetControlBlocked(false);
        }
        return true;
    }

    bool FlyingModeManager::TriggerTurn() {
        log::info("IDRC - {}", __func__);

        auto& controlsManager =ControlsManager::GetSingleton();

        bool flyingModeNotification = false;
    
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: Dragon actor is null", __func__);
            return false;
        }
    
        int flyState = _ts_SKSEFunctions::GetFlyingState(dragonActor);
    
        float speedMod = 1.0f;
        if (flyState == 0 || flyState == 3 || flyState == 5) { // dragon is landed, hovering, perching
            speedMod = 0.25f; // Reduce turn speed when grounded or hovering
        }
        
        controlsManager.SetControlBlocked(true);
    
        int flyCount = 0;
        int waitTime = 50; // 50ms
        while (CheckForTurn()) {
            _ts_SKSEFunctions::WaitWhileGameIsPaused();
            if (_ts_SKSEFunctions::GetFlyingState(dragonActor) == 2) { // flying
                // only trigger new turn every 300ms, to allow dragon to react on new direction
                flyCount ++;
                if (flyCount < 6) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
                    continue;
                }
            }

            flyCount = 0;
            DragonTurnPlayerRiding(speedMod * m_turnSpeed * GetTurnFactor());
            std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
        }

        controlsManager.SetControlBlocked(false);

        if (GetFlyingMode() == FlyingMode::kFlying &&
            !controlsManager.GetIsKeyPressed(IDRCKey::kBack) &&
            !controlsManager.GetIsKeyPressed(IDRCKey::kStrafeLeft) &&
            !controlsManager.GetIsKeyPressed(IDRCKey::kStrafeRight)) {
            // Stop ongoing turning by setting the new target  to the current direction.
            DragonTurnPlayerRiding(0.0f);
        }
    
        if (GetFlyingMode() != FlyingMode::kLanded && GetFlyingMode() != FlyingMode::kPerching) {
            // no notificiation when turning while grounded
            flyingModeNotification = true;
        }

        if (GetFlyingMode() == kLanded && _ts_SKSEFunctions::GetFlyingState(dragonActor) == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(6*waitTime));
            // if forward key is (still) pressed, trigger ground travel forward
            if (controlsManager.GetIsKeyPressed(kForward)) {
                OnKeyDown(kForward);
            }
        }
    
        return flyingModeNotification;
    }

    bool FlyingModeManager::CheckForTurn() const {
        auto& controlsManager =ControlsManager::GetSingleton();
        bool turn = false;
        if (controlsManager.IsThumbstickPressed()) {
            if (controlsManager.GetIsKeyPressed(kForward) &&
                !controlsManager.GetIsKeyPressed(kStrafeLeft) && 
                !controlsManager.GetIsKeyPressed(kStrafeRight)) {
                turn = false;
            } else {
                turn = true;
            }
        } else {
            turn = controlsManager.GetIsKeyPressed(IDRCKey::kStrafeLeft) || controlsManager.GetIsKeyPressed(IDRCKey::kStrafeRight);
        }
        return turn;
    }

    float FlyingModeManager::GetTurnFactor() {
        float turnFactor = 0.0f;
        auto& controlsManager = ControlsManager::GetSingleton();

        if (controlsManager.IsThumbstickPressed()) {
            float thumbstickAngle = controlsManager.GetThumbstickAngle();
            // Thumbstick angle convention: right = 0, forward = 90,  left = 180, back = 270 (in radians)
            if (thumbstickAngle > 270.0f * 0.0174533f) {thumbstickAngle -= 360.0f * 0.0174533f;}
            // Forward: turnFactor = 0; Back: turnFactor = 2.0
            // Left: turnfactor negative; Right: turnFactor positive
            turnFactor = ( 90.0f * 0.0174533f - thumbstickAngle) / (90.0f * 0.0174533f);
        }
        else {
            turnFactor = 0.5f;
            if (!controlsManager.GetIsKeyPressed(IDRCKey::kForward)) {
                if (!controlsManager.GetIsKeyPressed(IDRCKey::kBack)) {
                    turnFactor = 1.0f;
                } else {
                    turnFactor = 2.0f;
                }
            }
            float direction = controlsManager.GetIsKeyPressed(kStrafeLeft) ? -1.0f : 1.0f;
            if (APIs::TrueDirectionalMovementV4 && APIs::TrueDirectionalMovementV4->IsTargetLockBehindTarget()) {
                direction *= -1.0f;
            }
            turnFactor *= direction;
        }
        turnFactor *= GetRunFactor();
        return turnFactor;
    }

    bool FlyingModeManager::DragonTravelTo(RE::TESObjectREFR* a_directionMarker) {
        log::info("IDRC - {}", __func__);
    
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return false;
        }
    
        if (!a_directionMarker) {
            log::error("IDRC - {}: Direction marker is null", __func__);
            return false;
        }
    
        // Check if FastTravel can be stopped
        if (!FastTravelManager::GetSingleton().StopFastTravel(a_directionMarker, 0.0, 
                    200, "", "Unexpected error - dragon not grounded?")) {
            log::error("IDRC - {}: Failed to stop FastTravel", __func__);
            return false;
        }
    
        // Clear combat targets if the dragon is far from the combat target
        auto* combatTarget = CombatManager::GetSingleton().GetCombatTarget();
        if (combatTarget && _ts_SKSEFunctions::GetDistance(dragonActor, combatTarget) >= 5000.0f) {
            log::info("IDRC - {}: Clearing CombatTargets", __func__);
            SKSE::GetTaskInterface()->AddTask([dragonActor]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
               _ts_SKSEFunctions::ClearCombatTargets(dragonActor);
            });
            CombatManager::GetSingleton().SetStopCombat(true);
        }
    
        // Switch to travel package
        SKSE::GetTaskInterface()->AddTask([dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 4); // Travel package
            dragonActor->EvaluatePackage();
        });
    
        log::info("IDRC - {}: Switched to Travel Package", __func__);
    
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
        // Switch back to orbit package
        SKSE::GetTaskInterface()->AddTask([dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 0); // Orbit package
            dragonActor->EvaluatePackage();
        });
        
        return true;
    }

    bool FlyingModeManager::TriggerLand(RE::TESObjectREFR* a_landTarget) {
        log::info("IDRC - {}", __func__);
    
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return false;
        }
    
        // Clear combat targets if the dragon is far from the combat target
        auto* combatTarget = CombatManager::GetSingleton().GetCombatTarget();
        if (combatTarget && _ts_SKSEFunctions::GetDistance(dragonActor, combatTarget) >= 5000.0f) {
            log::info("IDRC - {}: Clearing CombatTargets", __func__);
            SKSE::GetTaskInterface()->AddTask([dragonActor]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
               _ts_SKSEFunctions::ClearCombatTargets(dragonActor);
            });
            CombatManager::GetSingleton().SetStopCombat(true);
        }
    
        // If no a_andTarget is provided, use the dragonActor as the target
        if (!a_landTarget) {
            a_landTarget = dragonActor;
        }
    
        // Get the landing height with water and adjust for minimum height
        float zPos = _ts_SKSEFunctions::GetLandHeightWithWater(a_landTarget) + GetMinHeight();
    
        // Move the orbit marker to the landing position
        auto* orbitMarker = DataManager::GetSingleton().GetOrbitMarker();
        
        SKSE::GetTaskInterface()->AddTask([this, dragonActor, orbitMarker, a_landTarget, zPos]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            if (orbitMarker) {
                _ts_SKSEFunctions::MoveTo(orbitMarker, a_landTarget, 0.0f, 0.0f, zPos);
            } else {
                log::warn("IDRC - {}: Orbit marker is null", __func__);
            }
            
            if (this->m_noFlyAbility) {
                dragonActor->AddSpell(this->m_noFlyAbility);
            } else {
                log::warn("IDRC - {}: NoFlyAbility is null", __func__);
            }
        });
        Utils::SetAllowFlying(false);

        SKSE::GetTaskInterface()->AddTask([dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 0); // Orbit package
            dragonActor->EvaluatePackage();
        });
    
        return true;
    }

    bool FlyingModeManager::CancelDragonLandPlayerRiding() {
        log::info("IDRC - {}", __func__);
    
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return false;
        }
    
        // Check if the dragon is already landing or landed
        auto flyingMode = FlyingModeManager::GetSingleton().GetFlyingMode();
        int dragonFlyingState = _ts_SKSEFunctions::GetFlyingState(dragonActor);
    
        if (flyingMode == FlyingMode::kLanded &&
            (dragonFlyingState == 0 || dragonFlyingState == 4) &&
            !_ts_SKSEFunctions::IsFlyingMountFastTravelling(dragonActor) &&
            !_ts_SKSEFunctions::IsFlyingMountPatrolQueued(dragonActor)) {
            log::info("IDRC - {}: Already in Landing or landed. Ignore CancelLand request.", __func__);
            return false;
        }
    
        if (GetRegisteredForLanding()) {
            log::info("IDRC - {}: Trying to stop landing", __func__);
    
            Utils::SetAllowFlying(true);

            SKSE::GetTaskInterface()->AddTask([this, dragonActor]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                if (this->m_noFlyAbility) {
                    dragonActor->RemoveSpell(this->m_noFlyAbility);
                } else {
                    log::warn("IDRC - {}: NoFlyAbility is null", __func__);
                }
            });
    
            auto* orbitMarker = DataManager::GetSingleton().GetOrbitMarker();
            if (orbitMarker) {
                SKSE::GetTaskInterface()->AddTask([orbitMarker, dragonActor]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    _ts_SKSEFunctions::MoveTo(orbitMarker, dragonActor);
                });
            } else {
                log::warn("IDRC - {}: Orbit marker is null", __func__);
            }
    
            if (m_toggledAutoCombatLand) {
                log::info("IDRC - {}: AutoCombat toggled to TRUE", __func__);
                DataManager::GetSingleton().SetAutoCombat(true);
                m_toggledAutoCombatLand = false;
            }   
            SetRegisteredForLanding(false);
   
            DragonTakeOffPlayerRiding(dragonActor, false);
        }
    
        return true;
    }

    bool FlyingModeManager::GetRegisteredForLanding() {
        return m_registeredForLanding;
    }

    bool FlyingModeManager::GetRegisteredForPerch() {
        return m_registeredForPerch;
    }

    void FlyingModeManager::SetRegisteredForLanding(bool a_registeredForLanding) {
        m_registeredForLanding = a_registeredForLanding;
    }

    void FlyingModeManager::SetRegisteredForPerch(bool a_registeredForPerch) {
        m_registeredForPerch = a_registeredForPerch;
    }

    bool FlyingModeManager::DragonHoverPlayerRiding(RE::TESObjectREFR* a_hoverTarget, bool a_displayMode) {
        // Check if the dragon is registered for landing
        if (m_registeredForLanding) {
            log::info("IDRC - {}: Registered for Landing - cancel hover", __func__);
            return false;
        }
    
        auto& dataManager = DataManager::GetSingleton();
        auto& displayManager = DisplayManager::GetSingleton();
        auto* dragonActor = dataManager.GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return false;
        }
    
        log::info("IDRC - {}: DragonHoverPlayerRiding, IsBeingRidden = {}", __func__, dragonActor->IsBeingRidden());
    
        auto flyingMode = GetFlyingMode();

        float injuredHealthPercentage = dragonActor->GetRace()->data.injuredHealthPercent;

        if (_ts_SKSEFunctions::GetHealthPercentage(dragonActor) > injuredHealthPercentage &&
            (flyingMode != FlyingMode::kHovering || _ts_SKSEFunctions::GetFlyingState(dragonActor) != 3)) {
            Utils::SetAllowFlying(true);

            if (!dragonActor->AsActorState()->actorState2.allowFlying) {
                log::info("IDRC - {}: Could not enable Flying - cancel hover", __func__);
                return false;
            }
    
            SetFlyingMode(FlyingMode::kHovering);
            if (a_displayMode) {
                displayManager.DisplayFlyingMode();
            }
    
            SKSE::GetTaskInterface()->AddTask([dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
                dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kWaitingForPlayer, 0);
            });
    
            std::string searchMessage = "";
            if (a_displayMode) {
                searchMessage = "The dragon is still approaching hover position";
            }
    
            // Stop fast travel
            if (!FastTravelManager::GetSingleton().StopFastTravel(dragonActor, 0.0f, 200, "", searchMessage)) {
                return false;
            }
    
            // Check if flying state is still valid
            flyingMode = GetFlyingMode();
            if (flyingMode != FlyingMode::kHovering) {
                // another flying mode change has been triggered since the hover command was initiated
                // (this can happen in case Hover is triggered as event, from FlyingModeDown())
                log::info("IDRC - {}: No longer valid - cancel hover", __func__);
                return false;
            }
    
            auto* orbitMarker = dataManager.GetOrbitMarker();
            if (!orbitMarker) {
                log::error("IDRC - {}: Orbit marker is null", __func__);
                return false;
            }

            // Determine the dragon's current height above ground
            float dragonPosZ = dragonActor->GetPositionZ();
            float heightAboveGround = dragonPosZ - _ts_SKSEFunctions::GetLandHeightWithWater(RE::PlayerCharacter::GetSingleton());
    
            if (heightAboveGround < GetMinHeight()) {
                heightAboveGround = GetMinHeight();
            }
    
            // Adjust the hover target's position
            float angleZ = a_hoverTarget->GetAngleZ();
            float distance = 4600.0f;

            SKSE::GetTaskInterface()->AddTask([this, dragonActor, a_hoverTarget, orbitMarker, 
                                            distance, angleZ, dragonPosZ, heightAboveGround]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                if (_ts_SKSEFunctions::GetFlyingState(dragonActor) != 3) { // not hovering
                    RE::NiPoint3 angle = {0.0f, 0.0f, angleZ};
                    _ts_SKSEFunctions::SetAngle(orbitMarker, angle);

                    // move Hover Target a bit ahead, so that dragon (hopefully) does not need to take a turn to reach the hover target
                    _ts_SKSEFunctions::MoveTo(orbitMarker, a_hoverTarget, distance * std::sin(angleZ), distance * std::cos(angleZ), 0.0f);

                    float markerPosZ = _ts_SKSEFunctions::GetLandHeightWithWater(orbitMarker) + heightAboveGround;

                    if (markerPosZ < dragonPosZ) {
                        markerPosZ = (dragonPosZ + markerPosZ) / 2.0f;
                    }

                    // move the hover target to the calculated height
                    orbitMarker->SetPosition(orbitMarker->GetPositionX(), orbitMarker->GetPositionY(), markerPosZ);
                } else {
                    _ts_SKSEFunctions::MoveTo(orbitMarker, dragonActor);
                }

                // move turn marker ahead of orbit marker to fix new look-at position
                // see comments in DisplayManager::UpdateDisplay() for more information on why this is necessary
                _ts_SKSEFunctions::MoveTo(this->m_dragonTurnMarker, orbitMarker, 2500.0f * std::sin(angleZ), 2500.0f * std::cos(angleZ), 0.0f);
                // Set the dragon to hover mode
                dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 3); // Hover package
                dragonActor->EvaluatePackage();
            });
    
            // Register for updates to track flying state and display hover state
            displayManager.DisplayHoverStatus(a_displayMode);  
        }
    
        return true;
    }

    bool FlyingModeManager::ForceHover() {
        log::info("IDRC - {}", __func__);
    
        auto& dataManager = DataManager::GetSingleton();
        auto* dragonActor = dataManager.GetDragonActor();
    
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return false;
        }
    
        if (!m_flyToTargetMarker) {
            log::error("IDRC - {}: FlyToTargetMarker is null", __func__);
            return false;
        }
    
        ControlsManager::GetSingleton().SetControlBlocked(true);

        float centerX = GetWorldSpaceCenterX();
        float centerY = GetWorldSpaceCenterY();
    
        float angleZ = GetAngleToCoordinate(centerX, centerY);
    
        float offsetX = 6000.0f * std::sin(angleZ);
        float offsetY = 6000.0f * std::cos(angleZ);
    
        // Move the FlyToTargetMarker behind the dragon, directed at the worldspace center
        SKSE::GetTaskInterface()->AddTask([this, dragonActor, offsetX, offsetY, angleZ]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            _ts_SKSEFunctions::MoveTo(this->m_flyToTargetMarker, dragonActor, offsetX, offsetY, 0.0f);
            RE::NiPoint3 angle = { 0.0f, 0.0f, angleZ };
            _ts_SKSEFunctions::SetAngle(this->m_flyToTargetMarker, angle);
        });
    
        DragonHoverPlayerRiding(m_flyToTargetMarker, false);
    
        ControlsManager::GetSingleton().SetControlBlocked(false);

        return true;
    }

    bool FlyingModeManager::DragonTakeOffPlayerRiding(RE::TESObjectREFR* a_takeOffTarget, bool a_displayMode) {
        log::info("IDRC - {}", __func__);
    
        // Check if the dragon is registered for landing
        if (m_registeredForLanding) {
            log::info("IDRC - {}: Registered for Landing - cancel takeoff", __func__);
            return false;
        }
    
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return false;
        }
        
        float injuredHealthPercent = dragonActor->GetRace()->data.injuredHealthPercent;
        if (_ts_SKSEFunctions::GetHealthPercentage(dragonActor) > injuredHealthPercent) {
            // Set flying state to hovering
            SetFlyingMode(FlyingMode::kHovering);
    
            if (a_displayMode) {
               DisplayManager::GetSingleton().DisplayFlyingMode();
            }
    
            auto* orbitMarker = DataManager::GetSingleton().GetOrbitMarker();
            if (!orbitMarker) {
                log::error("IDRC - {}: Orbit marker is null", __func__);
                return false;
            }

            if (!a_takeOffTarget) {
                log::info("IDRC - {}: No TakeOffTarget provided - cancel takeoff", __func__);
                return false;
            }
            
            float angleZ = dragonActor->GetAngleZ();    
            RE::NiPoint3 angle = { dragonActor->GetAngleX(), dragonActor->GetAngleY(), angleZ };
            
            SKSE::GetTaskInterface()->AddTask([this, orbitMarker, angle, a_takeOffTarget, angleZ]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
                _ts_SKSEFunctions::SetAngle(orbitMarker, angle);
                _ts_SKSEFunctions::MoveTo(orbitMarker, a_takeOffTarget, 
                    100.0f * std::sin(angleZ), 100.0f * std::cos(angleZ),  this->GetMinHeight());
            });    

            Utils::SetAllowFlying(true);

            SKSE::GetTaskInterface()->AddTask([this, dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
                if (m_noFlyAbility) {
                    dragonActor->RemoveSpell(this->m_noFlyAbility);
                } else {
                    log::warn("IDRC - {}: NoFlyAbility is null", __func__);
                }
            });    

            // Trigger StopFastTravel just in case - should not be in FastTravel when taking off
            if (!FastTravelManager::GetSingleton().StopFastTravel(orbitMarker)) {
                log::info("IDRC - {}: Failed to stop FastTravel - cancel takeoff", __func__);
                return false;
            }
    
            SKSE::GetTaskInterface()->AddTask([dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
                dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 3); // Hover package
                dragonActor->EvaluatePackage();
            });
    
            // Register for updates to track flying state
            Utils::RegisterForSingleUpdate(0.5f);
            DisplayManager::GetSingleton().SetRegisteredForDisplayUpdate(true);
    
            // Wait for the dragon to take off
            int count = 0;
            while (count < 50 && (_ts_SKSEFunctions::GetFlyingState(dragonActor) == 0 || _ts_SKSEFunctions::GetFlyingState(dragonActor) == 5)) {
                _ts_SKSEFunctions::WaitWhileGameIsPaused();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                count++;
            }
    
            if (count >= 50) {
                log::info("IDRC - {}: Dragon did not take off - cancel takeoff", __func__);
                return false;
            }
        } else if (a_displayMode) {
            log::info("IDRC - {}: Dragon health is low - cannot take off", __func__);
            RE::DebugNotification("Dragon health is low - cannot take off");
            return false;
        }
    
        return true;
    }

    void FlyingModeManager::PlaceTravelToMarker(RE::TESObjectREFR* a_ref, float a_distance, float a_angle, float a_offsetZ) {
        log::info("IDRC - {}", __func__);
    
        if (!a_ref) {
            log::error("IDRC - {}: a_ref is null", __func__);
            return;
        }
    
        if (!m_dragonTravelToMarker) {
            log::error("IDRC - {}: DragonTravelToMarker is null", __func__);
            return;
        }
    
        float markerPosX = a_ref->GetPositionX() + a_distance * std::sin(a_ref->GetAngleZ() + a_angle);
        float markerPosY = a_ref->GetPositionY() + a_distance * std::cos(a_ref->GetAngleZ() + a_angle);
        float markerPosZ = _ts_SKSEFunctions::GetLandHeight(markerPosX, markerPosY, a_ref->GetPositionZ()) + a_offsetZ;
    
        SKSE::GetTaskInterface()->AddTask([this, a_ref, markerPosX, markerPosY, markerPosZ]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            this->m_dragonTravelToMarker->MoveTo(a_ref); // ensures TravelToMarker is in same worldspace as a_ref
            this->m_dragonTravelToMarker->SetPosition(markerPosX, markerPosY, markerPosZ);
        });
    }

    bool FlyingModeManager::DragonLandPlayerRiding(RE::TESObjectREFR* a_landTarget, bool a_displayMode) {
        log::info("IDRC - {}", __func__);
    
        auto& dataManager = DataManager::GetSingleton();
        auto& displayManager = DisplayManager::GetSingleton();
        auto* dragonActor = dataManager.GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return false;
        }
    
        // Check if the dragon is already landing or landed
        int dragonFlyingState = _ts_SKSEFunctions::GetFlyingState(dragonActor);
        if (GetFlyingMode() == FlyingMode::kLanded && (dragonFlyingState == 0 || dragonFlyingState == 4)) {
            log::info("IDRC - {}: Already in Landing or landed. Ignore Land request.", __func__);
            return false;
        }
    
        auto& combatManager = CombatManager::GetSingleton();
        // Check if an attack is ongoing or if combat-related packages are active
        if (combatManager.IsAttackOngoing() || combatManager.GetAttackMode() != 0 ||
            _ts_SKSEFunctions::CheckForPackage(dragonActor, combatManager.GetCombatTargetPackageList())) {
            return false;
        }
    
        if (!GetRegisteredForLanding()) {
            // Set flying state to landed
            SetFlyingMode(FlyingMode::kLanded);
    
            if (a_displayMode) {
                displayManager.DisplayFlyingMode();
            }
    
            SetRegisteredForLanding(true);
    
            std::string searchMessage = "";
            std::string timeoutMessage = "";
            if (a_displayMode) {
                searchMessage = "The dragon is still searching for a place to land";
                timeoutMessage = "Could not find a landing spot - Land cancelled";
            }
    
            // Cancel any fast travel and set target to LandTarget
            if (!FastTravelManager::GetSingleton().StopFastTravel(a_landTarget, 0.0f, 200, searchMessage, timeoutMessage)) {
                CancelDragonLandPlayerRiding();
                return false;
            }
    
            // Handle auto-combat toggling
            m_toggledAutoCombatLand = false;
            if (dataManager.GetAutoCombat() && !combatManager.IsAutoCombatAttackToggled()) {
                log::info("IDRC - {}: AutoCombat toggled to FALSE", __func__);
                dataManager.SetAutoCombat(false);
                m_toggledAutoCombatLand = true;
            }
    
            // Place the travel-to marker below the dragon
            PlaceTravelToMarker(dragonActor);

// Below is papyrus (unconverted) - commented out because currently not used.
//
//		if !PO3_SKSEFunctions.IsRefUnderwater(DragonTravelToMarker) && (BorderRegionScene as _ts_DR_BorderRegionSceneScript).IsPlayerInBorderRegion()
// Only if landing spot is not underwater
// and is within the limits of Skyrim / Solstheim 
// (this is tracked via a Scene that continuously checks against the condition IsPlayerInRegion,
//   with regions "BorderRegionSkyrim" and "DLC2SolstheimBorderRegion")

// SetForcedLandingMarker() helps ensure that the dragon does not start to fly far away to land 
// (sometimes happens if the engine cannot identify a suitable landing spot)
// Do not use SetForcedLandingMarker() for now
// this sometimes leads to the dragon landing at spots that trigger a "skyshot"
//			dragonActor.SetForcedLandingMarker(DragonTravelToMarker)
// don't forget to clear the forced landing marker when the dragon has landed!
//		endIf
    
            // Trigger the landing process
            if (_ts_SKSEFunctions::IsFlyingMountFastTravelling(dragonActor) ||
                _ts_SKSEFunctions::IsFlyingMountPatrolQueued(dragonActor)) {
                log::info("IDRC - {}: Cancelling Land!", __func__);
                CancelDragonLandPlayerRiding();
                return false;
            }
    
            TriggerLand(a_landTarget);
    
            // Wait for the dragon to land
            int count = 0;
            int notificationCount = 0;
            while (_ts_SKSEFunctions::GetFlyingState(dragonActor) != 0 && GetRegisteredForLanding()) {
                _ts_SKSEFunctions::WaitWhileGameIsPaused();
                count++;
                notificationCount++;
    
                if (count >= 200) { // Timeout after 20 seconds
                    log::info("IDRC - {}: Timeout - Trying to cancel Land", __func__);
                    if (CancelDragonLandPlayerRiding()) {
                        if (!timeoutMessage.empty()) {
                            RE::DebugNotification(timeoutMessage.c_str());
                        }
                        log::info("IDRC - {}: Cancelled Land", __func__);
                        return false;
                    }
                }
    
                if (a_displayMode && notificationCount >= 50) {
                    RE::DebugNotification(searchMessage.c_str());
                    notificationCount = 0;
                }
    
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
    
            log::info("IDRC - {}: Landed", __func__);
    
            // Restore auto-combat if it was toggled off
            if (m_toggledAutoCombatLand) {
                log::info("IDRC - {}: AutoCombat toggled to TRUE", __func__);
                DataManager::GetSingleton().SetAutoCombat(true);
                m_toggledAutoCombatLand = false;
            }
    
            // Reset dragon height and place the travel-to marker
            ResetDragonHeight();
            PlaceTravelToMarker(dragonActor);
    
            // Clear landing registration and register for updates
            SetRegisteredForLanding(false);
            Utils::RegisterForSingleUpdate(0.5f);
            displayManager.SetRegisteredForDisplayUpdate(true);
    
            return true;
        }
    
        return false;
    }

    bool FlyingModeManager::DragonOrbitPlayerRiding(RE::TESObjectREFR* a_orbitTarget) {
        log::info("IDRC - {}", __func__);
    
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return false;
        }
    
        // Check if the dragon is allowed to fly
        if (dragonActor->AsActorState()->actorState2.allowFlying) {
            // Set flying state to orbiting
            SetFlyingMode(FlyingMode::kOrbiting);
            DisplayManager::GetSingleton().DisplayFlyingMode();
    
            auto* orbitMarker = DataManager::GetSingleton().GetOrbitMarker();
            if (a_orbitTarget && orbitMarker) {
                SKSE::GetTaskInterface()->AddTask([orbitMarker, a_orbitTarget]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    _ts_SKSEFunctions::MoveTo(orbitMarker, a_orbitTarget);
                });
            } else {
                log::info("IDRC - {}: No OrbitTarget provided - cancel orbit", __func__);
                return false;
            }
    
            // Stop fast travel if currently ongoing
            std::string waitMessage = "The dragon is still approaching orbit position";
            std::string timeoutMessage = "Aborting orbit attempt...";
            if (!FastTravelManager::GetSingleton().StopFastTravel(orbitMarker, 0.0f, 200, waitMessage, timeoutMessage)) {
                log::info("IDRC - {}: Failed to stop FastTravel - cancel orbit", __func__);
                return false;
            }
    
            SKSE::GetTaskInterface()->AddTask([dragonActor]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kWaitingForPlayer, 0);
                dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 0); // Orbiting
                dragonActor->EvaluatePackage();
            });
    
            Utils::RegisterForSingleUpdate(0.5f);
            DisplayManager::GetSingleton().SetRegisteredForDisplayUpdate(true);
    
            return true;
        }
    
        log::info("IDRC - {}: Dragon is not allowed to fly - cancel orbit", __func__);
        return false;
    }

    bool FlyingModeManager::DragonPerchPlayerRiding() {
        log::info("IDRC - {}", __func__);
    
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is null", __func__);
            return false;
        }
    
        // Only trigger perch from hover
        if (_ts_SKSEFunctions::GetFlyingState(dragonActor) == 3 && !m_registeredForPerch) {
            SetRegisteredForPerch(true);
            auto oldState = GetFlyingMode();
    
            // Set flying state to perching
            SetFlyingMode(FlyingMode::kPerching);
            DisplayManager::GetSingleton().DisplayFlyingMode();
            auto& dataManager = DataManager::GetSingleton();

            // Start the perch quest
            auto* findPerchQuest = dataManager.GetFindPerchQuest();
            if (!findPerchQuest) {
                log::error("IDRC - {}: findPerchQuest is null", __func__);
                SetFlyingMode(oldState);
                SetRegisteredForPerch(false);
                return false;
            }

            SKSE::GetTaskInterface()->AddTask([findPerchQuest]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                findPerchQuest->Start();
            });

            int count = 0;
            while (count < 100 && findPerchQuest->IsStopped()) 
            {
                _ts_SKSEFunctions::WaitWhileGameIsPaused();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                count++;
            }
            if (count >= 100) { // waited > 1sec
                log::error("IDRC - {}: ERROR - Timed out while waiting for FindPerchQuest to start!", __func__);
                SetFlyingMode(oldState);
                SetRegisteredForPerch(false);
                return false;
            }

            // Retrieve potential perch targets
            auto* wordWall = dataManager.GetWordWallPerch();
            auto* tower = dataManager.GetTowerPerch();
            auto* rock = dataManager.GetRockPerch();
    
            if (!wordWall && !tower && !rock) {
                // No valid perch targets found
                dataManager.SetPerchTarget(nullptr);
                SetFlyingMode(oldState);
                SetRegisteredForPerch(false);
            } else {
                // Find the closest perch target
                float minDistance = 99999.9f;
                RE::TESObjectREFR* closestPerch = nullptr;
    
                auto* player = RE::PlayerCharacter::GetSingleton();
    
                if (wordWall && _ts_SKSEFunctions::GetDistance(wordWall, player) < minDistance) {
                    minDistance = _ts_SKSEFunctions::GetDistance(wordWall, player);
                    closestPerch = wordWall;
                }
                if (tower && _ts_SKSEFunctions::GetDistance(tower, player) < minDistance) {
                    minDistance = _ts_SKSEFunctions::GetDistance(tower, player);
                    closestPerch = tower;
                }
                if (rock && _ts_SKSEFunctions::GetDistance(rock, player) < minDistance) {
                    minDistance = _ts_SKSEFunctions::GetDistance(rock, player);
                    closestPerch = rock;
                }
                if (closestPerch) {
                    log::info("IDRC - {}: Found perch: {}", __func__, closestPerch->GetFormID());      

                    if (!dataManager.SetPerchTarget(closestPerch)) {
                        log::error("IDRC - {}: Could not set perch target", __func__);
                        SetFlyingMode(oldState);
                        SetRegisteredForPerch(false);
                        return false;
                    }

                    DisplayManager::GetSingleton().SetRegisteredForDisplayUpdate(true);

                    SKSE::GetTaskInterface()->AddTask([dragonActor]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                        dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 1); // Perching
                        dragonActor->EvaluatePackage();
                        Utils::RegisterForSingleUpdate(0.5f);
                    });
                }
            }

            SKSE::GetTaskInterface()->AddTask([findPerchQuest]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                findPerchQuest->Stop();
            });
        }
        return true;
    }


    bool FlyingModeManager::DragonTurnPlayerRiding(float a_turnAngle) {
        log::info("IDRC - {}", __func__);

        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        auto* orbitMarker = DataManager::GetSingleton().GetOrbitMarker();
    
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is None", __func__);
            return false;
        }
    
        if (!m_dragonTurnMarker) {
            log::error("IDRC - {}: DragonTurnMarker is None", __func__);
            return false;
        }

        if (!orbitMarker) {
            log::error("IDRC - {}: OrbitMarker is None", __func__);
            return false;
        }
        float angleZ = dragonActor->GetAngleZ() + a_turnAngle* 0.01745329252f;
    
        int flyingState = _ts_SKSEFunctions::GetFlyingState(dragonActor);
        if (flyingState == 0 || flyingState == 3 || flyingState == 5) { // Landed, hovering, or perching
            SKSE::GetTaskInterface()->AddTask([this, dragonActor, orbitMarker, angleZ]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                _ts_SKSEFunctions::MoveTo(this->m_dragonTurnMarker, dragonActor, 2500.0f * std::sin(angleZ), 2500.0f * std::cos(angleZ), 0.0f);
                _ts_SKSEFunctions::SetLookAt(dragonActor, this->m_dragonTurnMarker, true);
                auto angle = dragonActor->GetAngle();
                _ts_SKSEFunctions::SetAngle(orbitMarker, angle);
                _ts_SKSEFunctions::SetAngleZ(orbitMarker, angleZ);
                dragonActor->EvaluatePackage();
            });    
        } else if (flyingState == 1) { // Taking off
            if (m_mode == kOrbiting || m_mode == kHovering || m_mode == kFlying) {
                return DragonNewDirection(angleZ);
            }
        } else if (flyingState == 2) { // Cruising
            if ((m_mode == kOrbiting || m_mode == kHovering || m_mode == kFlying)  && !m_registeredForLanding && !m_registeredForPerch) {
                return DragonNewDirection(angleZ);
            }
        }
        return true;
    }

    bool FlyingModeManager::DragonNewDirection(float a_angle, bool a_displayMode) {
        log::info("IDRC - {}", __func__);
    
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is None", __func__);
            return false;
        }
    
        if (dragonActor->AsActorState()->actorState2.allowFlying == 0) { // Check if dragon is allowed to fly
            log::info("IDRC - {}: Dragon is not allowed to fly (injured) - cancel new direction", __func__);
            return false;
        }
    
    
        SKSE::GetTaskInterface()->AddTask([dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 0); // Set to orbit
            dragonActor->EvaluatePackage();
        });
    
        return DragonFlyTo(a_angle, a_displayMode);
    }

    bool FlyingModeManager::DragonFlyTo(float a_angle, bool a_displayMode){
        log::info("IDRC - {}", __func__);

        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        auto* orbitMarker = DataManager::GetSingleton().GetOrbitMarker();
        auto& combatManager = CombatManager::GetSingleton();

        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is None", __func__);
            return false;
        }
        if (!orbitMarker) {
            log::error("IDRC - {}: OrbitMarker is None", __func__);
            return false;
        }
        if (dragonActor->AsActorState()->actorState2.allowFlying == 0) {
            log::info("IDRC - {}: Dragon is not allowed to fly - cancel FlyTo", __func__);
            return false;
        }

        SetFlyingMode(kFlying); // flying to target

        if (a_displayMode) {  // only if requested through "Forward", not through "Strafe Left/Right"
            DisplayManager::GetSingleton().DisplayFlyingMode();
        }
        
        bool restartAttack = false;
        if  (combatManager.IsAttackOngoing() && 
            _ts_SKSEFunctions::GetCombatTarget(dragonActor) != nullptr) {   
            if (combatManager.StopAttack()) {
                restartAttack = true;
                log::info("IDRC - {}: Stopped Attack - restarting once FastTravel started", __func__);
            }
        }

        WorldSpaceData worldSpaceData; 
        try {
            worldSpaceData = WorldSpaceData(dragonActor->GetWorldspace()); 
        } catch (const std::invalid_argument& e) {
            log::error("IDRC - {}: {}", __func__, e.what());
            return false;
        }
    
        // Current player coordinates
        auto* player = RE::PlayerCharacter::GetSingleton();
        float posX = player->GetPositionX();
        float posY = player->GetPositionY();
        float angleNorm = NormalizeAngle(a_angle);

        float distance = GetDistanceToRegionBoundingBox(worldSpaceData, posX, posY, angleNorm);

        if(!IsInBorderRegion(worldSpaceData.m_borderRegionName)){
            log::info("IDRC - {}: Player is not in border region {} - cancel FlyTo...", __func__, worldSpaceData.m_borderRegionName);
            return false;
        }  

        if(_ts_SKSEFunctions::IsFlyingMountPatrolQueued(dragonActor) == true){
            // do not trigger FastTravel while dragon is patrolQueue is still ongoing:
            // that would keep the dragon in PatrolQueued state
            log::info("IDRC - {}: in PatrolQueued - cancel FlyTo...", __func__);
            return false;
        }


        if (GetFlyingMode() == kFlying) { // no other flying state triggered yet
            // move dragon orbit marker to the Skyrim borderline which is in the direction the player has defined.
            float markerPosX = posX + distance * std::sin(angleNorm);
            float markerPosY = posY + distance * std::cos(angleNorm);

            float height = worldSpaceData.m_seaLevel + GetMinHeight();
            SKSE::GetTaskInterface()->AddTask([dragonActor, orbitMarker, markerPosX, markerPosY, height]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                orbitMarker->MoveTo(dragonActor); // ensures orbitMarker is in same worldspace as dragonActor
                orbitMarker->SetPosition(markerPosX, markerPosY, height);
                dragonActor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kVariable03, 0);
                dragonActor->EvaluatePackage();
            });
        }

        if (GetFlyingMode() != kFlying) {
            log::info("IDRC - {}: no longer in Flying mode - cancel FlyTo...", __func__);
            return false;
        }

        // Cancel ongoing StopFastTravel requests
        FastTravelManager::GetSingleton().CancelStopFastTravel();

        SKSE::GetTaskInterface()->AddTask([dragonActor]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            _ts_SKSEFunctions::ClearCombatTargets(dragonActor);
        });

        // Start the FastTravel mode
        FastTravelManager::GetSingleton().FastTravel(orbitMarker);
log::info("IDRC - {}: FastTravel started", __func__);
    
        if (restartAttack) {
            combatManager.DragonAttack();
        }

        if (a_displayMode) {
           Utils::RegisterForSingleUpdate(0.5f);
           DisplayManager::GetSingleton().SetRegisteredForDisplayUpdate(true);
       }

        return true;
    }

    bool FlyingModeManager::GetContinueFlyTo() {
        return m_continueFlyTo;
    }

    float FlyingModeManager::GetFlyToAngle() {
        return m_flyToAngle;
    }

    void FlyingModeManager::SetContinueFlyTo(bool a_continue) {
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();

        if (!dragonActor) {
            log::error("IDRC - {}: dragonActor is None", __func__);
            return;
        }

        m_continueFlyTo = a_continue;
        m_flyToAngle = dragonActor->GetAngleZ();
    }

    float FlyingModeManager::GetAngleToCoordinate(float a_posX, float a_posY) {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            log::error("IDRC - {}: Player reference is null", __func__);
            return 0.0f;
        }
    
        // Calculate deltas
        float deltaX = a_posX - player->GetPositionX();
        float deltaY = a_posY - player->GetPositionY();
    
        float angle = 0.0f;
    
        // Avoid division by zero
        if (deltaY != 0.0f) {
            angle = std::atan2(deltaX, deltaY);
            if (angle < 0.0f) {
                angle +=  2.0f * PI;
            }
        } else if (deltaX > 0.0f) {
            angle = 0.5f * PI;  // 90 degrees
        } else {
            angle = 1.5f * PI; // 270 degrees
        }
        return angle;
    }
    
    float FlyingModeManager::GetWorldSpaceCenterX() {
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: Dragon actor is null", __func__);
            return 0.0f;
        }
    
        auto* worldSpace = dragonActor->GetWorldspace();
        if (!worldSpace) {
            log::error("IDRC - {}: Worldspace is null", __func__);
            return 0.0f;
        }
    
        if (strcmp(worldSpace->GetFullName(), "Solstheim") == 0) {
            return 50000.0f;
        } else if (strcmp(worldSpace->GetFullName(), "Skyrim") == 0) {
            return 0.0f;
        }
    
        log::warn("IDRC - {}: Unknown worldspace - returning default center X = 0.0", __func__);
        return 0.0f;
    }
    
    float FlyingModeManager::GetWorldSpaceCenterY() {
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: Dragon actor is null", __func__);
            return 0.0f;
        }
    
        auto* worldSpace = dragonActor->GetWorldspace();
        if (!worldSpace) {
            log::error("IDRC - {}: Worldspace is null", __func__);
            return 0.0f;
        }
    
        if (strcmp(worldSpace->GetFullName(), "Solstheim") == 0) {
            return 60000.0f;
        } else if (strcmp(worldSpace->GetFullName(), "Skyrim") == 0) {
            return 0.0f;
        }
    
        log::warn("IDRC - {}: Unknown worldspace - returning default center Y = 0.0", __func__);
        return 0.0f;
    }

    void FlyingModeManager::WorldSpaceData::InitializeData(const std::string& a_regionName) {
        float sealevel= 0.0f;
        float min_x = 0.0f;
        float min_y = 0.0f;
        float max_x = 0.0f;
        float max_y = 0.0f;
        float center_x = 0.0f;
        float center_y = 0.0f;

/* currently not used: Scan through all regions and finde the BorderRegions for all worldspaces
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            log::error("IDRC - {}: DataHandler is null", __func__);
            return;
        }
        auto* regionList = dataHandler->regionList;
        if (!regionList) {
            log::error("IDRC - {}: RegionList is null", __func__);
            return;
        }
        for (const auto& region : *regionList) {
currently not scanning through all regions of all worldspaces - instead look up region by given name: */         
            auto* form = RE::TESForm::LookupByEditorID(a_regionName); // LookupByEditorID() only works with powerofthree's Tweaks ("Load EditorID" tweak)
            auto* region = form ? form->As<RE::TESRegion>() : nullptr;

            if (region && ((region->GetFormFlags() & RE::TESRegion::RecordFlags::kBorderRegion) != 0)) { 
                // && region->worldSpace == a_worldSpace
log::info("IDRC - {}: Worldspace {} - BorderRegion: {} / {}", __func__, region->worldSpace->GetFullName(), region->GetFormEditorID(), region->GetFormID());
                
                sealevel= region->worldSpace->GetDefaultWaterHeight();

                auto* regionPoints  = region->pointLists;
                if (!regionPoints) {
                    log::info("IDRC - {}: region pointLists is null", __func__);
                } else {
                    for (const auto& pointList : *regionPoints) {
                        if (pointList) {
                            log::info("IDRC - {}: worldspace: {}, PointList minimums: x = {}, y = {}, maximums: x = {}, y = {}", 
                                    __func__, region->worldSpace->GetFullName(), pointList->minimums.x, pointList->minimums.y, pointList->maximums.x, pointList->maximums.y);

                            min_x = pointList->minimums.x - 10000.0f;
                            min_y = pointList->minimums.y - 10000.0f;
                            max_x = pointList->maximums.x + 10000.0f;
                            max_y = pointList->maximums.y + 10000.0f;

                            int iCount = 0;
                            float sumX = 0.0f;
                            float sumY = 0.0f;
                            for (const auto& regionPoint : *pointList) {
                                if (regionPoint) {
                                    sumX += regionPoint->point.x;
                                    sumY += regionPoint->point.y;
                                    iCount++;
                                }
                            }
                            if (iCount > 0) {
                                log::info("IDRC - {}: region FormID {}: avg-center ({}, {}), min/max-center ({}, {})", __func__, 
                                    region->GetFormID(), sumX / iCount, sumY / iCount, 
                                    (pointList->minimums.x + pointList->maximums.x)/2.0f, (pointList->minimums.y + pointList->maximums.y)/2.0f);

                                center_x = sumX / iCount;
                                center_y = sumY / iCount;
                            } else {
                                log::info("IDRC - {}: PointList has no points", __func__);
                            }
                        }
                    }
                }
            } else {
                log::warn("IDRC - {}: Form is not a TESRegion or is not a BorderRegion", __func__);
            }
//        }  end scan through all regions - currently not used.
        m_minX = min_x;
        m_maxX = max_x;
        m_minY = min_y;
        m_maxY = max_y;
        m_seaLevel = sealevel;
        m_borderRegionName = a_regionName;
        m_centerX = center_x;
        m_centerY = center_y;
    }

    FlyingModeManager::WorldSpaceData::WorldSpaceData(const RE::TESWorldSpace* a_worldSpace) {
        if (!a_worldSpace) {
            log::error("IDRC - {}: Worldspace is null", __func__);
            return;
        }
        
        std::string worldspace_EDID = clib_util::editorID::get_editorID(a_worldSpace);

        // The Border region of the worldSpace MUST be completely within the bounding box defined by fMinX, fMaxX, fMinY, fMaxY
        if (strcmp(worldspace_EDID.c_str(), "Tamriel") == 0) {
            m_minX = -220000;
            m_maxX = 240000;
            m_minY = -150000;
            m_maxY = 200000;
            m_seaLevel = -14000;
            m_borderRegionName = "BorderRegionSkyrim";
        }  else if (strcmp(worldspace_EDID.c_str(), "DLC2SolstheimWorld") == 0) {
            m_minX = -20000;
            m_maxX = 140000;
            m_minY = -20000;
            m_maxY = 140000;
            m_seaLevel = 0;
            m_borderRegionName = "DLC2SolstheimBorderRegion";
        } else {
            log::error("IDRC - {}: Trying to use dragon fast travel in invalid worldspace: {}!", __func__, worldspace_EDID.c_str());
       }
    }

    bool FlyingModeManager::IsInBorderRegion(std::string a_regionName) const {
        bool isInBorderRegion = _ts_SKSEFunctions::IsPlayerInRegion(a_regionName);

        if (!isInBorderRegion && a_regionName == "BorderRegionSkyrim") {
            // if in Worldspace Tamriel / Skyrim, also check for Castle Volkihar
            isInBorderRegion = _ts_SKSEFunctions::IsPlayerInRegion("DLC1BorderRegionSkyrim");
        }

        return isInBorderRegion;
    }

    float FlyingModeManager::NormalizeAngle(float a_angle){
        // Normalize angle
        float angleNorm = a_angle;
        if (angleNorm < 0.0f) {
            angleNorm += 2.0f *PI;
        } else if (angleNorm > 2.0f *PI) {
            angleNorm -= 2.0f *PI;
        }
        return angleNorm;
    }


    float FlyingModeManager::GetDistanceToRegionBoundingBox(const WorldSpaceData& a_worldspaceData,
         float a_posX, float a_posY, float a_angle){
        // Calculate distances
        float distance1, distance2;
        float angleDeg = a_angle * 180.0f / PI;
        if (angleDeg < 0.0) {
            log::error("IDRC - {}: Angle smaller than 0 - aborting...", __func__);
            return -1.0;
        } else if (angleDeg < 0.1) {
            // target direction is parallel to western/eastern border - avoid division by 0
            distance1 = (a_worldspaceData.m_maxY - a_posY) / std::cos(a_angle); //  distance to northern border
            distance2 = distance1;
        } else if (angleDeg < 89.9) {
            // target direction is in first quadrant relative to current position - either at northern or eastern border
            distance1 = (a_worldspaceData.m_maxY - a_posY) / std::cos(a_angle); // distance to northern border (infinitely extended - might be outside Skyrim)
            distance2 = (a_worldspaceData.m_maxX - a_posX) / std::sin(a_angle); // distance to eastern border (infinitely extended - might be outside Skyrim)
        } else if (angleDeg < 90.1) {
            // target direction is parallel to northern/southern border - avoid division by 0
            distance1 = (a_worldspaceData.m_maxX - a_posX) / std::sin(a_angle); // distance to eastern border
            distance2 = distance1;
        } else if (angleDeg < 179.9) {
            // target direction is in second quadrant relative to current position - either at southern or eastern border
            distance1 = (a_worldspaceData.m_minY - a_posY) / std::cos(a_angle); // distance to southern border (infinitely extended - might be outside Skyrim)
            distance2 = (a_worldspaceData.m_maxX - a_posX) / std::sin(a_angle); // distance to eastern border (infinitely extended - might be outside Skyrim)
        } else if (angleDeg < 180.1) {
            // target direction is parallel to western/eastern border - avoid division by 0
            distance1 = (a_worldspaceData.m_minY - a_posY) / std::cos(a_angle); // distance to southern border
            distance2 = distance1;
        } else if (angleDeg < 269.9) {
            // target direction is in third quadrant relative to current position - either at southern or western border
            distance1 = (a_worldspaceData.m_minY - a_posY) / std::cos(a_angle); // distance to southern border (infinitely extended - might be outside Skyrim)
            distance2 = (a_worldspaceData.m_minX - a_posX) / std::sin(a_angle); // distance to western border (infinitely extended - might be outside Skyrim)
        } else if (angleDeg < 270.1) {
            // target direction is parallel to northern/southern border - avoid division by 0
            distance1 = (a_worldspaceData.m_minX - a_posX) / std::sin(a_angle); // distance to western border
            distance2 = distance1;
        } else if (angleDeg < 359.9) {
            // target direction is in fourth quadrant relative to current position - either at northern or western border
            distance1 = (a_worldspaceData.m_maxY - a_posY) / std::cos(a_angle); // distance to northern border (infinitely extended - might be outside Skyrim)
            distance2 = (a_worldspaceData.m_minX - a_posX) / std::sin(a_angle); // distance to western border (infinitely extended - might be outside Skyrim)
        } else if (angleDeg <= 360) {
            // target direction is parallel to western/eastern border - avoid division by 0
            distance1 = (a_worldspaceData.m_maxY - a_posY) / std::cos(a_angle); // distance to northern border
            distance2 = distance1;
        } else {
            log::error("IDRC - {}: Angle larger larger than 360 - aborting...", __func__);
            return -1.0;
        }

        // Take smaller of the distances
        return std::min(distance1, distance2);
    }
} // namespace IDRC
