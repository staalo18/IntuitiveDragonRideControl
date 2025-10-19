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

            bool const IsCameraLocked() const;

            void SetUserTurning(bool a_moved);

        private:
            CameraLockManager() = default;

            void LockTurn(int a_lockTime);

            bool m_turnLocked = false;
            bool m_turnOngoing = false;
            bool m_cameraLocked = false;
            bool m_isUserTurning = false;
            float m_dragonYaw = 0.0f;
            int m_flyState = -1;
    }; // class CameraLockManager
}  // namespace IDRC
