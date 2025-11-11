#include "CameraLockManager.h"
#include "DataManager.h"
#include "FlyingModeManager.h"
#include "FastTravelManager.h"
#include "ControlsManager.h"
#include "_ts_SKSEFunctions.h"
#include "APIManager.h"
#include "DataManager.h"


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

            // Height control
            if (flyState == 2 && 
                 !controlsManager.GetIsKeyPressed(kStrafeLeft) && !controlsManager.GetIsKeyPressed(kStrafeRight) &&
                 !controlsManager.GetIsKeyPressed(kUp) && !controlsManager.GetIsKeyPressed(kDown)) {

                float travelledDistance = travelledVec.Length();
                float travelledZ = travelledVec.z;
                float travelledXY = std::sqrt(travelledVec.x * travelledVec.x + travelledVec.y * travelledVec.y);
                float travelledPitch = std::atan2(travelledZ, travelledXY);
                float cameraPitch = Utils::GetCameraPitch();
                if (cameraPitch < 0) {
                    // ignore camera downward pitch up to m_ignoredCameraPitch, then ramp up cameraPitch smoothly
                    if (cameraPitch > m_ignoredCameraPitch) {
                        cameraPitch = 0.0f;
                    }  else if (cameraPitch > (m_ignoredCameraPitch + m_transitionalPitchRange)) {
                        cameraPitch = (m_ignoredCameraPitch + m_transitionalPitchRange) * (cameraPitch - m_ignoredCameraPitch) / m_transitionalPitchRange;
                    }
                }

                DampenPitch(cameraPitch, travelledPitch);

//                float cameraHeightChange = travelledXY * std::tan(cameraPitch);
                float cameraHeightChange = travelledDistance * std::sin(cameraPitch);
                
                float effectiveHeightChange = cameraHeightChange - 0.5f*travelledZ;

                flyingModeManager.ChangeDragonHeight(effectiveHeightChange, true);
            }

            // Turning
            if  ( !m_turnLocked  // don't spam the Turn calls
                  && (m_isUserTurning || m_turnOngoing || flyState == 2) // only if user is actively triggering a turn (via mouse or gamepad), or such auser-triggered turn is not yet completed
                  && (flyState == 2 ||fabs(currentDragonYawOffset) > 2.f * PI / 180.f) // ignore turn angles smaller than 2 degrees
                  && !controlsManager.GetIsKeyPressed(kStrafeLeft) && !controlsManager.GetIsKeyPressed(kStrafeRight)
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

    void CameraLockManager::SetIgnoredCameraPitch(float a_pitch) {
        m_ignoredCameraPitch = -a_pitch * PI / 180.f;
    }

    void CameraLockManager::DampenPitch(float a_cameraPitch, float a_travelledPitch) {
        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            return;
        }

        RE::NiPoint3 dragonAngle = dragonActor->GetAngle();

        float groundHeight = _ts_SKSEFunctions::GetLandHeightWithWater(dragonActor);
        float groundFactor= std::clamp((dragonActor->GetPosition().z - groundHeight - 1000.f)/2000.f, 0.f, 1.f);

        float targetPitch = 0.3f * dragonAngle.x - 0.6f * a_cameraPitch - 0.1f * a_travelledPitch;
        targetPitch = (1.f -  groundFactor) * dragonAngle.x +  groundFactor * targetPitch;

        dragonAngle.x = targetPitch;
//        SKSE::GetTaskInterface()->AddTask([this, dragonActor, dragonAngle]() {
        // When modifying Game objects, send task to TaskInterface to ensure thread safety
        dragonActor->SetAngle(dragonAngle);
//        });
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
