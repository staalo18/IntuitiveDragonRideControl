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

        private:
            CameraLockManager() = default;

            void LockTurn(int a_lockTime);

            bool m_turnLocked = false;
            bool m_cameraLocked = false;
            bool m_enterHover = false;
            int m_flyState = -1;
            float m_dragonYaw = 0.0f;
            float m_cameraYaw = 0.0f;
    }; // class CameraLockManager
}  // namespace IDRC
