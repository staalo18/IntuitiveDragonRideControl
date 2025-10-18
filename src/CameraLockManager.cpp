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

        auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            log::warn("IDRC - {}: No dragon actor found", __func__);
            return;
        }

//        auto playerCharacter = RE::PlayerCharacter::GetSingleton();
        auto playerCamera = RE::PlayerCamera::GetSingleton();
        RE::ThirdPersonState* dragonCameraState = nullptr;

        if (playerCamera && playerCamera->currentState && (playerCamera->currentState->id == RE::CameraState::kDragon)) {
            dragonCameraState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
            if (!dragonCameraState) {
                log::warn("IDRC - {}: Dragon camera state is null", __func__);
                return;
            }
        } else {
            return;
        }

//        if (!playerCharacter || !dragonCameraState) {
//            log::warn("IDRC - {}: Missing player character or dragon camera state", __func__);
//            return;
//        }

//        RE::NiPoint3 playerPos = playerCharacter->GetPosition();
//        RE::NiPoint3 cameraPos = Utils::GetCameraPos();

//        float currentCameraYaw = Utils::GetAngleZ(cameraPos, playerPos);
        float currentCameraYaw = Utils::GetCameraYaw();

        bool isCameraBehindTarget = false;
        if (APIs::TrueDirectionalMovementV4) {
            isCameraBehindTarget = APIs::TrueDirectionalMovementV4->IsTargetLockBehindTarget();
        }

/*        
SKSE::GetTaskInterface()->AddTask([dragonActor, playerCharacter, currentCameraYaw]() {
// When modifying Game objects, send task to TaskInterface to ensure thread safety
dragonActor->SetHeading(currentCameraYaw);
playerCharacter->SetHeading(currentCameraYaw);
}); */

        float currentDragonYaw = dragonActor->GetAngleZ();
        float currentYawOffset = currentDragonYaw - currentCameraYaw;
            if (isCameraBehindTarget) {
                currentYawOffset += PI;
            }
        currentYawOffset = Utils::NormalRelativeAngle(currentYawOffset);

        auto& controlsManager = ControlsManager::GetSingleton();
        auto& flyingModeManager = FlyingModeManager::GetSingleton();
        int flyState = _ts_SKSEFunctions::GetFlyingState(dragonActor);
        auto flyMode = flyingModeManager.GetFlyingMode();

        auto* orbitMarker = DataManager::GetSingleton().GetOrbitMarker();
        if (!orbitMarker) {
            log::warn("IDRC - {}: Could not obtain OrbitMarker", __func__);
            return;
        }

        float orbitMarkerYaw = orbitMarker->GetAngleZ();
        float orbitMarkerYawOffset = fabs(Utils::NormalRelativeAngle(currentCameraYaw - orbitMarkerYaw));

        bool isTDMLocked = false;
        if (APIs::TrueDirectionalMovementV1) {
            isTDMLocked = APIs::TrueDirectionalMovementV1->GetTargetLockState();
        }

        if (((flyState == 3 && m_flyState != 3) || m_enterHover) && m_cameraLocked) {
            if ((orbitMarkerYawOffset > 2.f * PI / 180.f && !isTDMLocked)) {
                 // this check is needed to ensure that the camera stays locked during
                 // the "enter hover" animation (flyState is already hover at that time)
                m_enterHover = true;
            } else {
                m_enterHover = false;
            }
        } else {
            m_enterHover = false;
        }

        bool isAITurn = false;
        if (m_cameraLocked && fabs(Utils::NormalRelativeAngle(currentDragonYaw - m_dragonYaw)) > 0.1f * PI / 180.f) { //FLT_EPSILON) {
            isAITurn = true;
        }
        m_dragonYaw = currentDragonYaw;

log::info("IDRC - {}: isAITurn: {}, m_cameraLocked: {}, flyState: {}, m_flyState: {}, flyMode: {}, orbitMarkerYawOffset: {}", __func__,
isAITurn, m_cameraLocked, flyState, m_flyState, (int)flyMode, 180.f / PI * orbitMarkerYawOffset);

        if (( (flyState == 3 && flyMode == FlyingMode::kHovering && !m_enterHover) ||
              (flyState == 0 && flyMode == FlyingMode::kLanded) ||
              (flyState == 2 && flyMode == FlyingMode::kFlying) ) &&
            ( !controlsManager.GetIsKeyPressed(kStrafeLeft) && 
              !controlsManager.GetIsKeyPressed(kStrafeRight) ) //&& !isAITurn
            ) {
            // dragon yaw follows camera rotation
            m_flyState = flyState;
            m_cameraLocked = false;
log::info("IDRC - {}: Camera Yaw Diff: {}", __func__, fabs(currentCameraYaw - m_cameraYaw));
            if (fabs(currentCameraYaw - m_cameraYaw) <=  FLT_EPSILON) {
log::info("IDRC - {}: Skipping Turn - NO CAMERA MOVEMENT", __func__);
                return;
            }

            if (fabs(currentYawOffset) > 2.f * PI / 180.f) {
                if (m_turnLocked) { // don't trigger another DragonTurnPlayerRiding() while locked
log::info("IDRC - {}: Skipping Turn - TURN LOCKED", __func__);
                    return;
                }

                flyingModeManager.DragonTurnPlayerRiding(-180.f / PI * currentYawOffset);

                int lockTime = 30;
                if (flyState == 2) {
                    lockTime = 300; // longer lock time when flying
                }
                LockTurn(lockTime); // Prevent next DragonTurnPlayerRiding() call for lockTime ms
            }
else {
log::info("IDRC - {}: Skipping Turn - NO YAW OFFSET CHANGE", __func__);
}
        } else {
            // camera rotation follows dragon yaw
            m_flyState = flyState;
            m_cameraLocked = true;

            if (!isTDMLocked) {
                float currentCameraRotation = Utils::NormalRelativeAngle(dragonCameraState->freeRotation.x);
                float realTimeDeltaTime = Utils::GetRealTimeDeltaTime() < 0.05f ? Utils::GetRealTimeDeltaTime() : 0.05f;
                float damping = 1.0f - 2.5f * realTimeDeltaTime;
                float newCameraRotation =  Utils::NormalRelativeAngle(damping *currentCameraRotation);

log::info("IDRC - {}: LOCKING CAMERA - cameraRotation: {}, currentYawOffset: {}, Delta: {}, freeRotation: {}", __func__,
180.f / PI * currentCameraRotation, 180.f / PI * currentYawOffset, realTimeDeltaTime, 180.f / PI * dragonCameraState->freeRotation.x);

                SKSE::GetTaskInterface()->AddTask([dragonCameraState, newCameraRotation]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    dragonCameraState->freeRotation.x = newCameraRotation;
                });
            } 
        }

        m_cameraYaw = currentCameraYaw;
    }

    bool const CameraLockManager::IsCameraLocked() const
    {
        return m_cameraLocked;
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
