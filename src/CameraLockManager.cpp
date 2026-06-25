#include "CameraLockManager.h"
#include "DataManager.h"
#include "FlyingModeManager.h"
#include "FastTravelManager.h"
#include "ControlsManager.h"
#include "_ts_SKSEFunctions.h"
#include "APIManager.h"
#include "DataManager.h"
#include "CombatManager.h"


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

        float currentCameraYaw = _ts_SKSEFunctions::GetYaw(dragonCameraState->rotation);
        bool isCameraBehindTarget = false;
        if (APIs::TrueDirectionalMovementV4) {
            isCameraBehindTarget = APIs::TrueDirectionalMovementV4->IsTargetLockBehindTarget();
        }

        auto& combatManager = CombatManager::GetSingleton();
        if (combatManager.IsShoutActive() && combatManager.GetShoutTarget()) {
            // If a shout target exists, use its position to calculate the camera yaw to keep 
            // the dragon oriented towards the target while shouting.
            // Also see the similar logic in Hooks::UpdateFlightPathData(), 
            // which uses the shout target position to update flight path waypoints.
            auto dragonPos = dragonActor->GetPosition();
            auto shoutTargetPos = combatManager.GetShoutTarget()->GetPosition();
            currentCameraYaw = atan2(shoutTargetPos.x - dragonPos.x, shoutTargetPos.y - dragonPos.y);
        }

        float currentDragonYaw = dragonActor->GetAngleZ();
        float currentDragonYawOffset = currentCameraYaw - currentDragonYaw;
        if (isCameraBehindTarget) {
            currentDragonYawOffset += PI;
        }
        currentDragonYawOffset = _ts_SKSEFunctions::NormalRelativeAngle(currentDragonYawOffset);

        int flyState = _ts_SKSEFunctions::GetFlyingState(dragonActor);
        auto flyMode = flyingModeManager.GetFlyingMode();

        bool isTDMLocked = false;
        if (APIs::TrueDirectionalMovementV1) {
            isTDMLocked = APIs::TrueDirectionalMovementV1->GetTargetLockState();
        }

        bool isDragonTurning = false;
        if (fabs(_ts_SKSEFunctions::NormalRelativeAngle(currentDragonYaw - m_dragonYaw)) > 0.1f * PI / 180.f) {
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
            targetDragonYaw = _ts_SKSEFunctions::GetAngleZ(dragonActor->GetPosition(), turnMarker->GetPosition());
        } else if (flyState == 2) {  // flying
            targetDragonYaw = _ts_SKSEFunctions::GetAngleZ(dragonActor->GetPosition(), orbitMarker->GetPosition());
        }
        float targetDragonYawOffset = currentDragonYaw - targetDragonYaw;
        targetDragonYawOffset = _ts_SKSEFunctions::NormalRelativeAngle(targetDragonYawOffset);

        if (m_turnOngoing && fabs(targetDragonYawOffset) < 2.f * PI / 180.f) {
            m_turnOngoing = false;
        }

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

        if (!m_dragonPosInitialized) {
            m_dragonPos = dragonActor->GetPosition();
            m_dragonPosInitialized = true;
        }

        RE::NiPoint3 travelledVec = dragonActor->GetPosition() - m_dragonPos;
        m_dragonPos = dragonActor->GetPosition();

        if (( (flyState == 3 && flyMode == FlyingMode::kHovering ) ||
              (flyState == 0 && flyMode == FlyingMode::kLanded) ||
              (flyState == 2 && flyMode == FlyingMode::kFlying) )  // only trigger camera-induced movements if dragon is in one of these flying states 
           ) {

            // Turning
            if  ( !m_turnLocked  // don't spam the Turn calls
                  && (isTDMLocked || m_isUserTurning || m_turnOngoing || flyState == 2) // only if user is actively triggering a turn (via mouse or gamepad, or TDM Lock), or such a user-triggered turn is not yet completed
                  && (flyState == 2 ||fabs(currentDragonYawOffset) > 2.f * PI / 180.f) // ignore turn angles smaller than 2 degrees
                  && !controlsManager.GetIsKeyPressed(kStrafeLeft) && !controlsManager.GetIsKeyPressed(kStrafeRight)
                ) {
                // dragon yaw follows user-triggered camera rotation
                flyingModeManager.DragonTurnPlayerRiding(180.f / PI * currentDragonYawOffset);
                m_turnOngoing = true;
                int lockTime = 30;
                // TBD: Probably no longer needed...
                LockTurn(lockTime); // Prevent next DragonTurnPlayerRiding() call for lockTime ms
            } 
        }

        if ((_ts_SKSEFunctions::GetFlyingState(dragonActor) == 2 && combatManager.IsShoutActive() && combatManager.GetShoutTarget()) ||
            (isDragonTurning && !m_isUserTurning && !isTDMLocked && !m_turnLocked)) {
log::info("IDRC - {}: --------------->>>>>>>>>> camera rotation follows dragon yaw", __func__);
            // camera rotation follows dragon yaw
            m_cameraLocked = true;
            float currentCameraRotation = _ts_SKSEFunctions::NormalRelativeAngle(dragonCameraState->freeRotation.x);
            float realTimeDeltaTime = _ts_SKSEFunctions::GetRealTimeDeltaTime() < 0.05f ? _ts_SKSEFunctions::GetRealTimeDeltaTime() : 0.05f;
            float damping = 1.0f - 2.5f * realTimeDeltaTime;
            float newCameraRotation =  _ts_SKSEFunctions::NormalRelativeAngle(damping *currentCameraRotation);

//            SKSE::GetTaskInterface()->AddTask([dragonCameraState, newCameraRotation]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                dragonCameraState->freeRotation.x = newCameraRotation;
//            });
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

    void CameraLockManager::SetIgnoredCameraPitch(float a_pitch) {
        m_ignoredCameraPitch = -a_pitch * PI / 180.f;
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

    void CameraLockManager::LockHeight(int a_lockTime)
    {
        m_heightLocked = true;

        std::thread([this, a_lockTime]() {
            int singleWait = 10;
            int maxCount = a_lockTime / singleWait;
            for (int i = 0; i < maxCount; i++) {
                _ts_SKSEFunctions::WaitWhileGameIsPaused();
                std::this_thread::sleep_for(std::chrono::milliseconds(singleWait));
            }
            this->m_heightLocked = false;
        }).detach();
    }   
}  // namespace IDRC
