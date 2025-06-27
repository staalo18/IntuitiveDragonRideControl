#pragma once

#include <string>
#include "FlyingModeManager.h"

namespace IDRC {
    class DisplayManager {
        public:
            static DisplayManager& GetSingleton() {
                static DisplayManager instance;
                return instance;
            }
            DisplayManager(const DisplayManager&) = delete;
            DisplayManager& operator=(const DisplayManager&) = delete;

            void InitializeData();

            bool GetDisplayFlyingMode();

            void SetDisplayFlyingMode(bool a_display);

            bool GetDisplayMessages();
            
            void SetDisplayMessages(bool a_display);

            void SetRegisteredForDisplayUpdate(bool a_registered);

            void DisplayFlyingMode(const std::string& a_addMessage = "");

            void DisplayDragonHealth();

            void UpdateDisplay();
            
            void DisplayHoverStatus(const bool a_displayMode);
        private:
            DisplayManager() = default;
            ~DisplayManager() = default;

            bool m_displayFlyingMode = true;
            bool m_displayMessages = true;
            bool m_registeredForDisplayUpdate = false;
            bool m_displayHoverStatus = true;
            int m_hoverStatusCount = 0;

            bool GetRegisteredForDisplayUpdate();

            std::string GetFlyingModeName(FlyingMode a_mode);
    }; // class DisplayManager
} // namespace IDRC
    