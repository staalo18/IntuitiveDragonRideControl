#include "DisplayManager.h"
#include "FlyingModeManager.h"
#include "DataManager.h"
#include "IDRCUtils.h"
#include "_ts_SKSEFunctions.h"


namespace IDRC {

    void DisplayManager::InitializeData() {
        m_displayHoverStatus = true;
        m_hoverStatusCount = 0;
        SetRegisteredForDisplayUpdate(false);
    }

    bool DisplayManager::GetDisplayFlyingMode() {
        return m_displayFlyingMode;
    }

    void DisplayManager::SetDisplayFlyingMode(const bool a_display) {
        m_displayFlyingMode = a_display;
    }

    bool DisplayManager::GetDisplayMessages() {
        return m_displayMessages;
    }
     
    void DisplayManager::SetDisplayMessages(const bool a_display) {
        m_displayMessages = a_display;
    }

    bool DisplayManager::GetRegisteredForDisplayUpdate() {
        return m_registeredForDisplayUpdate;
    }

    void DisplayManager::SetRegisteredForDisplayUpdate(bool a_registered) {
        log::info("IDRC - {}: {}", __func__, a_registered);
        m_registeredForDisplayUpdate = a_registered;
    }

    void DisplayManager::DisplayFlyingMode(const std::string& a_addMessage) {
        if (m_displayFlyingMode) {
            // Retrieve the current flying mode as a string
            auto mode = FlyingModeManager::GetSingleton().GetFlyingMode();
            std::string sMessage = "Commanding " + GetFlyingModeName(mode) + " Mode";
    
            // Append the additional message if provided
            if (!a_addMessage.empty()) {
                sMessage += " - " + a_addMessage;
            }
    
            // Display the notification
            RE::DebugNotification(sMessage.c_str());
        }
    }

    std::string DisplayManager::GetFlyingModeName(FlyingMode a_mode) {
        // Map the flying mode to a string representation
        switch (a_mode) {
            case FlyingMode::kHovering:
                return "Hovering";
            case FlyingMode::kOrbiting:
                return "Orbiting";
            case FlyingMode::kPerching:
                return "Perching";
            case FlyingMode::kLanded:
                return "Land";
            case FlyingMode::kFlying:
                return "Flying";
            default:
                return "Unknown";
        }
    }

    void DisplayManager::DisplayDragonHealth() {
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::error("IDRC - {}: Dragon actor is null", __func__);
            return;
        }

        float healthPercentage = _ts_SKSEFunctions::GetHealthPercentage(dragonActor) * 100.0f;
        int roundedHealth = static_cast<int>(std::round(healthPercentage));
    
        RE::DebugNotification((DataManager::GetSingleton().GetDragonName() + " Health: " + std::to_string(roundedHealth) + "%").c_str());
    }

    void  DisplayManager::DisplayHoverStatus(const bool a_displayMode) {
        Utils::RegisterForSingleUpdate(0.5f);
        SetRegisteredForDisplayUpdate(true);

        if (a_displayMode) {
            m_hoverStatusCount = 0;
            m_displayHoverStatus = true;
        }
    }

    void DisplayManager::UpdateDisplay() {    

        if (GetRegisteredForDisplayUpdate()) {
            auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
            if (!dragonActor) {
                log::error("IDRC - {}: Dragon actor is null", __func__);
                return;
            }
    
            int dragonFlyingState = _ts_SKSEFunctions::GetFlyingState(dragonActor);

            auto mode = FlyingModeManager::GetSingleton().GetFlyingMode();
            // Check flying state transitions
            if ((mode == FlyingMode::kFlying && dragonFlyingState == 2) || 
                (mode == FlyingMode::kOrbiting && dragonFlyingState == 2) ||
                (mode == FlyingMode::kHovering && dragonFlyingState == 3) ||
                (mode == FlyingMode::kLanded && dragonFlyingState == 0) ||
                (mode == FlyingMode::kPerching && dragonFlyingState == 5)) {

                if (GetDisplayFlyingMode()) {
                    RE::DebugNotification(("Entered " + GetFlyingModeName(mode) + " Mode").c_str());
                }

                auto* turnMarker = FlyingModeManager::GetSingleton().GetDragonTurnMarker();

                if (mode == FlyingMode::kHovering) {
                    // workaround to ensure that the dragon does not start turning when starting to hover
                    // this frequently happens when using the thumbstick control to
                    // trigger hover mode from flying - reason unknown.
                    // turnMarker has been put ahead of the OrbitMarker position when hovering is triggered
                    // (in FlyingModeManager::DragonHoverPlayerRiding() )
                    SKSE::GetTaskInterface()->AddTask([turnMarker, dragonActor]() {
                        // When modifying Game objects, send task to TaskInterface to ensure thread safety
                        if (turnMarker) {
                            _ts_SKSEFunctions::SetLookAt(dragonActor, turnMarker, true);
                            dragonActor->EvaluatePackage();
                        }
                    });
                }
    
                SetRegisteredForDisplayUpdate(false);
                m_displayHoverStatus = false;
                ControlsManager::GetSingleton().SetControlBlocked(false);
    
                if (mode == FlyingMode::kPerching && dragonFlyingState == 5) {
                    FlyingModeManager::GetSingleton().SetRegisteredForPerch(false);
                }
            } else {
                if (m_displayHoverStatus) {
                    m_hoverStatusCount++;
                    if (mode == FlyingMode::kFlying && dragonFlyingState != 3 && m_hoverStatusCount > 10) {
                        if (GetDisplayFlyingMode()) {
                            RE::DebugNotification("The dragon is still approaching hover position");
                        }
                        m_hoverStatusCount = 0;
                    }
                }
    
                Utils::RegisterForSingleUpdate(0.5f);
            }
        }
    }
} // namespace IDRC