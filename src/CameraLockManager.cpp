#include "CameraLockManager.h"
#include "DataManager.h"
#include "FlyingModeManager.h"
#include "ControlsManager.h"
#include "_ts_SKSEFunctions.h"
#include "APIManager.h"
#include "DataManager.h"
#include "IDRCUtils.h"


namespace IDRC {

    void CameraLockManager::Update()
    {
        if (RE::UI::GetSingleton()->GameIsPaused()) {
            return;
        }

        if (!m_isEnabled) {
            m_cameraLocked = false;
            return;
        }

        auto& controlsManager = ControlsManager::GetSingleton();
        auto& flyingModeManager = FlyingModeManager::GetSingleton();
        auto& dataManager = DataManager::GetSingleton();
        
        auto* dragonActor = dataManager.GetDragonActor();
        if (!dragonActor) {
            m_cameraLocked = false;
            return;
        }

        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        RE::ThirdPersonState* dragonCameraState = nullptr;

        if (playerCamera && playerCamera->currentState && (playerCamera->currentState->id == RE::CameraState::kDragon)) {
            dragonCameraState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
            if (!dragonCameraState) {
                log::warn("IDRC - {}: Dragon camera state is null", __func__);
                m_cameraLocked = false;
                return;
            }
        } else {
            m_cameraLocked = false;
            return;
        }

        float currentCameraYaw = Utils::GetCameraYaw();

        bool isCameraBehindTarget = false;
        if (APIs::TrueDirectionalMovementV4) {
            isCameraBehindTarget = APIs::TrueDirectionalMovementV4->IsTargetLockBehindTarget();
        }

        float currentDragonYaw = dragonActor->GetAngleZ();
        float currentDragonYawOffset = currentCameraYaw - currentDragonYaw;
            if (isCameraBehindTarget) {
                currentDragonYawOffset += PI;
            }
        currentDragonYawOffset = Utils::NormalRelativeAngle(currentDragonYawOffset);

        int flyState = _ts_SKSEFunctions::GetFlyingState(dragonActor);
        auto flyMode = flyingModeManager.GetFlyingMode();

        bool isTDMLocked = false;
        if (APIs::TrueDirectionalMovementV1) {
            isTDMLocked = APIs::TrueDirectionalMovementV1->GetTargetLockState();
        }

        bool isDragonTurning = false;
        if (fabs(Utils::NormalRelativeAngle(currentDragonYaw - m_dragonYaw)) > 0.1f * PI / 180.f) {
            isDragonTurning = true;
        }
        m_dragonYaw = currentDragonYaw;

        auto* orbitMarker = DataManager::GetSingleton().GetOrbitMarker();
        if (!orbitMarker) {
            log::warn("IDRC - {}: Could not obtain OrbitMarker", __func__);
            m_cameraLocked = false;
            return;
        }
        auto* turnMarker = flyingModeManager.GetDragonTurnMarker();
        if (!turnMarker) {
            log::warn("IDRC - {}: Could not obtain TurnMarker", __func__);
            m_cameraLocked = false;
            return;
        }

        float targetDragonYaw = currentDragonYaw;
        if (flyState == 0 || flyState == 3) { // hovering or landed
            targetDragonYaw = Utils::GetAngleZ(dragonActor->GetPosition(), turnMarker->GetPosition());
        } else if (flyState == 2) {  // flying
            targetDragonYaw = Utils::GetAngleZ(dragonActor->GetPosition(), orbitMarker->GetPosition());
        }
        float targetDragonYawOffset = currentDragonYaw - targetDragonYaw;
        targetDragonYawOffset = Utils::NormalRelativeAngle(targetDragonYawOffset);

        if (m_turnOngoing && fabs(targetDragonYawOffset) < 2.f * PI / 180.f) {
            m_turnOngoing = false;
        }

log::info("IDRC - {}: m_turnOngoing: {}, currentDragonYaw: {}, targetDragonYaw: {}, targetDragonYawOffset: {}, currentCameraYaw: {}", __func__,
m_turnOngoing, 180.f/PI * currentDragonYaw, 180.f/PI * targetDragonYaw, 180.f/PI * targetDragonYawOffset, 180.f/PI * currentCameraYaw);

        if (flyState != m_flyState) { // reset turnMarker to avoid dragon rotation after reaching new state
            if (flyState == 0) { // landed
                flyingModeManager.DragonTurnPlayerRiding(180.f / PI * currentDragonYawOffset);
            } else if (flyState == 3) {  // hovering
                SKSE::GetTaskInterface()->AddTask([turnMarker, orbitMarker]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    _ts_SKSEFunctions::MoveTo(turnMarker, orbitMarker, 2500.0f * std::sin(orbitMarker->GetAngleZ()), 2500.0f * std::cos(orbitMarker->GetAngleZ()), 0.0f);
                });
            }
        }
        m_flyState = flyState;

        if ( !m_turnLocked  // don't spam the Turn calls
             && (m_isUserTurning || m_turnOngoing) // only if user is actively triggering a turn (via mouse or gamepad), or such auser-triggered turn is not yet completed
             && (fabs(currentDragonYawOffset) > 2.f * PI / 180.f) // ignore turn angles smaller than 2 degrees
             && ( (flyState == 3 && flyMode == FlyingMode::kHovering ) ||
                  (flyState == 0 && flyMode == FlyingMode::kLanded) ||
                  (flyState == 2 && flyMode == FlyingMode::kFlying) )  // only trigger turns if dragon is in one of these flying states 
             && !controlsManager.GetIsKeyPressed(kStrafeLeft)
             && !controlsManager.GetIsKeyPressed(kStrafeRight)  // movement commands override camera-based turning
           ) {

            // dragon yaw follows user-triggered camera rotation
            flyingModeManager.DragonTurnPlayerRiding(180.f / PI * currentDragonYawOffset);
            m_turnOngoing = true;
            int lockTime = 30;
            if (flyState == 2) {
                lockTime = 300; // longer lock time when flying
            }
            LockTurn(lockTime); // Prevent next DragonTurnPlayerRiding() call for lockTime ms
        }

        if (isDragonTurning && !m_isUserTurning && !isTDMLocked && !m_turnLocked) {
            // camera rotation follows dragon yaw
            m_cameraLocked = true;
            float currentCameraRotation = Utils::NormalRelativeAngle(dragonCameraState->freeRotation.x);
            float realTimeDeltaTime = Utils::GetRealTimeDeltaTime() < 0.05f ? Utils::GetRealTimeDeltaTime() : 0.05f;
            float damping = 1.0f - 2.5f * realTimeDeltaTime;
            float newCameraRotation =  Utils::NormalRelativeAngle(damping *currentCameraRotation);

            SKSE::GetTaskInterface()->AddTask([dragonCameraState, newCameraRotation]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                dragonCameraState->freeRotation.x = newCameraRotation;
            });
        } else {
            m_cameraLocked = false;
        }

        m_isUserTurning = false; // reset flag. Is set to true in LookHook::ProcessMouseMove() in case of user-triggered camera rotation
    }

    void CameraLockManager::SetInitiallyEnabled(bool a_enabled)
    {
        m_initiallyEnabled = a_enabled;
    }

    bool const CameraLockManager::IsInitiallyEnabled() const
    {
        return m_initiallyEnabled;
    }

    void CameraLockManager::SetEnabled(bool a_enabled)
    {
        m_isEnabled = a_enabled;
    }

    bool const CameraLockManager::IsEnabled() const
    {
        return m_isEnabled;
    }

    void CameraLockManager::ResetEnabled()
    {
        m_isEnabled = m_initiallyEnabled;
    }

    bool const CameraLockManager::IsCameraLocked() const
    {
        return m_cameraLocked;
    }

    void CameraLockManager::SetUserTurning(bool a_moved) {
        m_isUserTurning = a_moved;
    }

    void CameraLockManager::LockTurn(int a_lockTime)
    {
        m_turnLocked = true;

        std::thread([this, a_lockTime]() {
            int singleWait = 10;
            int maxCount = a_lockTime / singleWait;
            for (int i = 0; i < maxCount; i++) {
                _ts_SKSEFunctions::WaitWhileGameIsPaused();
                std::this_thread::sleep_for(std::chrono::milliseconds(singleWait));
            }
            this->m_turnLocked = false;
        }).detach();
    }    
}  // namespace IDRC
