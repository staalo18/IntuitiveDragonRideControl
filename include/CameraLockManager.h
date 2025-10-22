#pragma once

namespace IDRC {
    class CameraLockManager {
        public:

            static CameraLockManager& GetSingleton() {
                static CameraLockManager instance;
                return instance;
            }
            CameraLockManager(const CameraLockManager&) = delete;
            CameraLockManager& operator=(const CameraLockManager&) = delete;

            void Update();

            void SetInitiallyEnabled(bool a_enabled);
            bool const IsInitiallyEnabled() const;

            void SetEnabled(bool a_enabled);
            bool const IsEnabled() const;

            void ResetEnabled();

            bool const IsCameraLocked() const;

            void SetUserTurning(bool a_moved);

        private:
            CameraLockManager() = default;

            void LockTurn(int a_lockTime);
            void LockHeight(int a_lockTime);

            bool m_initiallyEnabled = true;
            bool m_isEnabled = true;
            bool m_turnLocked = false;
            bool m_heightLocked = false;
            bool m_turnOngoing = false;
            bool m_cameraLocked = false;
            bool m_isUserTurning = false;
            float m_dragonYaw = 0.0f;
            int m_flyState = -1;
std::deque<float> m_pitchHistory;  // Store recent pitch values
float m_lastHeightChange = 0.0f;   // Remember last change applied
    }; // class CameraLockManager
}  // namespace IDRC
