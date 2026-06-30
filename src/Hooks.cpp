#include "Hooks.h"
#include "TargetReticleManager.h"
#include "CameraLockManager.h"
#include "FlyingModeManager.h"
#include "DataManager.h"
#include "CombatManager.h"
#include "FastTravelManager.h"
#include "IDRCUtils.h"
#include "APIManager.h"
#include "REL/RuntimeDataAccessors.h"

namespace Hooks
{
	void Install()
	{
		log::info("Hooking...");

		MainUpdateHook::Hook();

//		ReadyWeaponHook::Hook();
//		ExtraInteractionHook::Hook();
		LookHook::Hook();
		DragonCameraStateHook::Hook();
//		GetMountHook::Hook();
//		FlightGoalHook::Hook();
		PathingHook::Hook();

//		ActivateHandlerHook::Hook();
//		UpdateFlyingMountFastTravelHook::Hook();
/* UNUSED HOOKS:
		ExecuteTeleportHook::Hook();
		PapyrusFastTravelHook::Hook();
		UpdateFlyingMountFastTravelHook::Hook();
		CombatHook::Hook();
		VoiceSpellCastHook::Hook();
		VoiceShoutCastHook::Hook();
		StartCastHook::Hook();
		CastSpellImmediateHook::Hook();
		ApplyCastHook::Hook();
		TestCastHook::Hook();
		ProcedureShoutHook::Hook();
		StartVoiceShoutCastHook::Hook();
*/
		log::info("...success");
	}

	void ParseAnimationGraph() {
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			log::error("IDRC - {}: PlayerCharacter is null", __func__);
			return;
		}
		log::info("IDRC - {}: start parsing AnimationGraph", __func__);

		RE::BSAnimationGraphManagerPtr animGraph;
		if (player->GetAnimationGraphManager(animGraph)) {
			if (animGraph) {
				auto* mgr = animGraph.get();
				if (mgr) {
					// Iterate all graphs in the manager
					for (auto& graphPtr : mgr->graphs) {
						auto* graph = graphPtr.get();
						if (graph) {
							// Dump all variables in the graph's variableCache
							for (auto& varInfo : mgr->variableCache.variableCache) {
								if (varInfo.variableName.data() && varInfo.variable) {
									// Print as bool/int/float if you know the type, otherwise just print pointer value
//									log::info("AnimVar: {} = [ptr: {}]", varInfo.variableName.c_str(), static_cast<void*>(varInfo.variable));
									if ((varInfo.variableName == "iState" ||
										varInfo.variableName == "iLeftHandType" ||
										varInfo.variableName == "iRightHandType" ||
										varInfo.variableName == "iSyncSprintState")
										&& varInfo.variable) {
										int32_t value = *reinterpret_cast<int32_t*>(varInfo.variable);
										log::info("AnimVar: {} = {}", varInfo.variableName.c_str(), value);
									} else if ((varInfo.variableName == "Speed" ||
										varInfo.variableName == "Direction" ||
										varInfo.variableName == "TurnDelta" ||
										varInfo.variableName == "TDM_VelocityX" ||
										varInfo.variableName == "TDM_VelocityY" ||
										varInfo.variableName == "TDM_Pitch" ||
										varInfo.variableName == "TDM_Roll" ||
										varInfo.variableName == "HorseSpeedSampled" ||
										varInfo.variableName == "SpeedSampled")
										&& varInfo.variable) {
										float value = *reinterpret_cast<float*>(varInfo.variable);
										log::info("AnimVar: {} = {}", varInfo.variableName.c_str(), value);
									} else {								
										bool value = *reinterpret_cast<bool*>(varInfo.variable);
										log::info("AnimVar: {} = {}", varInfo.variableName.c_str(), value ? "true" : "false");
									} 
								} else {
									log::error("IDRC - {}: Variable name or pointer is null", __func__);
								}
							}
						}
						else {
							log::error("IDRC - {}: GraphPtr is null", __func__);
						}
					}
				} else {
					log::error("IDRC - {}: BSAnimationGraphManagerPtr is null", __func__);
				}
			} else {
				log::error("IDRC - {}: AnimationGraphManagerPtr is null", __func__);
			}
		} else {
			log::error("IDRC - {}: GetAnimationGraphManager failed", __func__);
		}
		log::info("IDRC - {}: Finished parsing AnimationGraph", __func__);		
	}

	void MainUpdateHook::Nullsub()
	{
		_Nullsub();

		IDRC::CameraLockManager::GetSingleton().Update();
		IDRC::TargetReticleManager::GetSingleton().Update();
		IDRC::CombatManager::GetSingleton().Update();
		IDRC::FastTravelManager::GetSingleton().Update();
		bool fastTravelFlag = GetFlyingMountFastTravelStateFlag();
		bool patrolQueuedFlag = GetFlyingMountPatrolQueuedStateFlag();
		auto* dragonActor = IDRC::DataManager::GetSingleton().GetDragonActor();
		if (dragonActor) {
			auto combatState = _ts_SKSEFunctions::GetCombatState(dragonActor);
//            dragonActor->EvaluatePackage();
//log::info("IDRC - {}: FastTravelFlag={}, PatrolQueuedFlag={}, combatState={}, AV3= {}, package: {:0x}", __func__, 
//	fastTravelFlag, patrolQueuedFlag, combatState, 
//	dragonActor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kVariable03), 
//	dragonActor->GetCurrentPackage()->GetFormID());
		}
/* For debugging
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			log::error("IDRC - {}: PlayerCharacter is null", __func__);
			return;
		}

		UpdateRotationMatrixDisplay(player->Get3D());

        auto* dragonActor = IDRC::DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            return;
        }		
		UpdateRotationMatrixDisplay(dragonActor->Get3D());
*/
	}


    void MainUpdateHook::UpdateRotationMatrixDisplay(RE:: NiAVObject* m_reference3D) {

        if (m_reference3D) {
            // Extract rotation from body part node
            const RE::NiMatrix3& rotMatrix = m_reference3D->world.rotate;


            auto rotX = rotMatrix.GetVectorX();
            auto rotY = rotMatrix.GetVectorY();
            auto rotZ = rotMatrix.GetVectorZ();

            if (APIs::TrueHUD) {
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate, m_reference3D->world.translate + 20.f * rotX, 0.1f, 0xFF0000FF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 20.f * rotX, m_reference3D->world.translate + 40.f * rotX, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 40.f * rotX, m_reference3D->world.translate + 60.f * rotX, 0.1f, 0xFF0000FF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 60.f * rotX, m_reference3D->world.translate + 80.f * rotX, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 80.f * rotX, m_reference3D->world.translate + 100.f * rotX, 0.1f, 0xFF0000FF);

                APIs::TrueHUD->DrawLine(m_reference3D->world.translate, m_reference3D->world.translate + 20.f * rotY, 0.1f, 0x00FF00FF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 20.f * rotY, m_reference3D->world.translate + 40.f * rotY, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 40.f * rotY, m_reference3D->world.translate + 60.f * rotY, 0.1f, 0x00FF00FF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 60.f * rotY, m_reference3D->world.translate + 80.f * rotY, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 80.f * rotY, m_reference3D->world.translate + 100.f * rotY, 0.1f, 0x00FF00FF);

                APIs::TrueHUD->DrawLine(m_reference3D->world.translate, m_reference3D->world.translate + 20.f * rotZ, 0.1f, 0x0000FFFF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 20.f * rotZ, m_reference3D->world.translate + 40.f * rotZ, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 40.f * rotZ, m_reference3D->world.translate + 60.f * rotZ, 0.1f, 0x0000FFFF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 60.f * rotZ, m_reference3D->world.translate + 80.f * rotZ, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(m_reference3D->world.translate + 80.f * rotZ, m_reference3D->world.translate + 100.f * rotZ, 0.1f, 0x0000FFFF);

            } else {
                log::info("{}: TrueHUD API not available for debug drawing", __FUNCTION__);
            }
        } else {
            log::info("{}: No target point found", __FUNCTION__);
        }
    }


	static bool bTraceGetType = false;

	void ReadyWeaponHook::ProcessButton(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
	{
		
		// call original function so other plugins can hook this vfunc properly
		bTraceGetType = true; // reset trace flag
        _ProcessButton(a_this, a_event, a_data);
		bTraceGetType = false; // reset trace flag

		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			log::error("IDRC - {}: PlayerCharacter is null", __func__);
			return;
		}
/*
		if(!actor) {
			player->lastRiddenHorseHandle = 0;
		} else {
			player->lastRiddenHorseHandle = actor->CreateRefHandle();
		}

		bool wasOnDragon = false;
		bool wasOnMount = false;

		// Save current state
		player->GetGraphVariableBool("IsOnDragon", wasOnDragon);
		player->GetGraphVariableBool("IsOnMount", wasOnMount);
*/
//		auto has_kInteraction = player->extraList.HasType(RE::ExtraDataType::kInteraction);
//		log::info("IDRC - {}: before: has kInteraction: {}", __func__, has_kInteraction ? "true" : "false");
//		player->extraList.RemoveByType(RE::ExtraDataType::kInteraction);
//		has_kInteraction = player->extraList.HasType(RE::ExtraDataType::kInteraction);
//		log::info("IDRC - {}: after: has kInteraction: {}", __func__, has_kInteraction ? "true" : "false");
//		log::info("IDRC - {}: Player is on Dragon: {}, on Mount: {}", __func__, wasOnDragon ? "true" : "false", wasOnMount ? "true" : "false");	
//		player->NotifyAnimationGraph("BeginWeaponDraw");
//ParseAnimationGraph();
log::info("IDRC - {}: ReadyWeaponHook-ProcessButton called with event IDCode = {}", __func__, a_event->GetIDCode());
//		player->DrawWeaponMagicHands(true);
//log::info("IDRC - {}: DrawWeaponMagicHands called", __func__);
//ParseAnimationGraph();

//		log::info("IDRC - {}: ProcessButton called with event IDCode = {}", __func__, a_event->GetIDCode());
	}

		RE::ExtraDataType ExtraInteractionHook::GetType(const RE::ExtraInteraction* a_this)
        {
			if (bTraceGetType) {
				log::info("IDRC - {}: GetType called, will return {}", __func__, static_cast<uint32_t>(_GetType(a_this)));
			}
            return _GetType(a_this);
        }
        bool ExtraInteractionHook::IsNotEqual(const RE::ExtraInteraction* a_this, const RE::BSExtraData* a_rhs)
        {
			log::info("IDRC - {}: IsNotEqual called, will return {}", __func__, _IsNotEqual(a_this, a_rhs) ? "true" : "false");
            return _IsNotEqual(a_this, a_rhs);
        }

	void LookHook::ProcessThumbstick(RE::LookHandler* a_this, RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto& cameraLockManager = IDRC::CameraLockManager::GetSingleton();
		if (a_event && a_event->IsRight() && cameraLockManager.IsCameraLocked())
		{
			return;
		}
		else
		{
			cameraLockManager.SetUserTurning(true);
			_ProcessThumbstick(a_this, a_event, a_data);
		}
	}

	void LookHook::ProcessMouseMove(RE::LookHandler* a_this, RE::MouseMoveEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto& cameraLockManager = IDRC::CameraLockManager::GetSingleton();
		if (a_event && cameraLockManager.IsCameraLocked())
		{
			return;
		}
		else
		{
			cameraLockManager.SetUserTurning(true);
			_ProcessMouseMove(a_this, a_event, a_data);
		}
	}

	void DragonCameraStateHook::OnEnterState(RE::DragonCameraState* a_this)
	{
		_OnEnterState(a_this);

		RE::Actor* dragon = nullptr;
		dragon = static_cast<RE::Actor*>(a_this->dragonHandle.get().get());

		if (dragon) {
			reinterpret_cast<float&>(a_this->unkEC) = dragon->GetHeading(false); //dragonCurrentDirection

			// TODO: also update pitch? What is the best pitch at the end of the mount animation...
		}
	}

	void DragonCameraStateHook::UpdateRotation(RE::DragonCameraState* a_this)
	{
		// This is copied from True Directional Movement. All credits go to the original author Ersh!

//		auto directionalMovementHandler = DirectionalMovementHandler::GetSingleton();
//		if (directionalMovementHandler->GetFreeCameraEnabled() && !directionalMovementHandler->IFPV_IsFirstPerson() && !directionalMovementHandler->ImprovedCamera_IsFirstPerson()) {
			float dragonCurrentDirection = reinterpret_cast<float&>(a_this->unkEC);
			float freeRotationX = a_this->freeRotation.x;

			a_this->freeRotationEnabled = true;

			_UpdateRotation(a_this);

			reinterpret_cast<float&>(a_this->unkEC) = dragonCurrentDirection;
			a_this->freeRotation.x = freeRotationX;

			if (a_this->dragonHandle) {
				RE::Actor* dragon = nullptr;
				dragon = static_cast<RE::Actor*>(a_this->dragonHandle.get().get());
				if (dragon) {
					float heading = dragon->GetHeading(false);

					a_this->freeRotation.x += reinterpret_cast<float&>(a_this->unkEC) - heading;

					NiQuaternion_SomeRotationManipulation(a_this->rotation, -a_this->freeRotation.y, 0.f, heading + a_this->freeRotation.x);
					reinterpret_cast<float&>(a_this->unkEC) = heading;
				}
			}
//		} else {
//			_UpdateRotation(a_this);
//		}
	}

    void DragonCameraStateHook::GetCurrentRotation(RE::DragonCameraState* a_this, RE::NiQuaternion& a_out)
    {
        _GetCurrentRotation(a_this, a_out);

        auto* dragonActor = IDRC::DataManager::GetSingleton().GetDragonActor();
        if (!dragonActor) {
            return;
        }

		float targetRoll = 0.f;
		RE::NiAVObject* reference3D = nullptr;

		if (_ts_SKSEFunctions::GetFlyingState(dragonActor) == 0) {
			// use dragon roll while grounded (player roll is too shaky while grounded)
			reference3D = dragonActor->Get3D();
		} else {
			// when not grounded, dragon3D coordinates are always horizontal (no roll)
			// use player roll instead
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				log::error("IDRC - {}: PlayerCharacter is null", __func__);
				return;
			}

			reference3D = player->Get3D();
		}

		if (reference3D) {
			targetRoll = -asinf(reference3D->world.rotate.GetVectorX().z);
		} else {
			log::warn("IDRC - {}: reference 3D is null", __func__);
			return;
		}

		const float rollSmoothTime    = IDRC::DataManager::GetSingleton().GetRollSmoothTime();
		const float rollAmplitude = IDRC::DataManager::GetSingleton().GetRollAmplitude();
		static float smoothedRoll   = 0.f;
		static float rollVelocity   = 0.f;  // radians/sec

		if (!RE::UI::GetSingleton()->GameIsPaused()) {
			const float dt = _ts_SKSEFunctions::GetRealTimeDeltaTime();
			const float omega = 2.0f / rollSmoothTime;
			const float e = expf(-omega * dt);
			const float c1 = smoothedRoll - targetRoll;
			const float c2 = rollVelocity + omega * c1;
			smoothedRoll = targetRoll + (c1 + c2 * dt) * e;
			rollVelocity = (c2 - omega * (c1 + c2 * dt)) * e;
		}

		float effectiveRoll = smoothedRoll * rollAmplitude;

		if (effectiveRoll == 0.0f) {
			return;
		}

		const float c = cosf(effectiveRoll * 0.5f);
		const float s = sinf(effectiveRoll * 0.5f);

		// Right-multiply by q_roll = (c, 0, s, 0)  [Y-axis rotation]
		// q_new = a_out * q_roll
		const RE::NiQuaternion q = a_out;
		a_out.w =  q.w * c - q.y * s;
		a_out.x =  q.x * c - q.z * s;
		a_out.y =  q.w * s + q.y * c;
		a_out.z =  q.x * s + q.z * c;
	}

/*
	bool FlightGoalHook::SetGoal(std::uintptr_t* a_this, RE::BSPathingRequest** a_request)
	{
///////////
// NOT USED
///////////
		if (_SetGoal == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return false;
		}

		// call the original function
		using FuncType = decltype(&SetGoal);
		auto result = reinterpret_cast<FuncType>(_SetGoal)(a_this, a_request);
		auto* dragonActor = IDRC::DataManager::GetSingleton().GetDragonActor();

		if (dragonActor) {
			if (IDRC::FlyingModeManager::GetSingleton().GetFlyingMode() == IDRC::FlyingMode::kFlying) {
				if (_ts_SKSEFunctions::GetFlyingState(dragonActor) == 2) {
					if (a_request && *a_request) {
						auto& startPos = (*a_request)->start.location.location;
						float distanceToDragon = (dragonActor->GetPosition()).GetDistance(startPos);
						if (distanceToDragon == 0.f) {
							log::info("IDRC - {}: Start position: ({}, {}, {}), distance to Dragon: {}", __func__, startPos.x, startPos.y, startPos.z, (dragonActor->GetPosition()).GetDistance(startPos));
							auto& goalPos = (*a_request)->goal.targetPoint;
							log::info("IDRC - {}: Goal position: ({}, {}, {})", __func__, goalPos.x, goalPos.y, goalPos.z);
							log::info("IDRC - {}: Result: {}", __func__, result ? "true" : "false");
						}
					} else {
						log::warn("IDRC - {}: Pathing request pointer is null", __func__);
					}
				}
			}
		}

		return result;
	}
*/


	void PathingHook::SetFlightPath(std::uintptr_t  a_subPtr, std::uintptr_t* a_newNode, std::uintptr_t* a_newData) {

		if (_SetFlightPath == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}

		// call the original function
		using FuncType = decltype(&SetFlightPath);
		reinterpret_cast<FuncType>(_SetFlightPath)(a_subPtr, a_newNode, a_newData);
log::info("IDRC - {}: SetFlightPath called", __func__);
//		auto agent = reinterpret_cast<std::byte*>(a_subPtr - 0x20);
//		UpdateFlightPathData(agent);
	}

	void PathingHook::FlightPlannerUpdate(std::uintptr_t a_plannerSubPtr,
									float* a_deltaTime,
									float* a_outMovementIntention,
									void* a_context) {
	// This hook fires for all dragons in flyingstate == 2 (Flying), once per frame.
	// Uses UpdateFlightPathData() to keep mounted dragon on a straight path towards the targeted direction
	
		if (!_FlightPlannerUpdate) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}

		auto agent = reinterpret_cast<std::byte*>(a_plannerSubPtr - 0x18);
		UpdateFlightPathData(agent);

		// call the original function
		using FuncType = decltype(&FlightPlannerUpdate);
		reinterpret_cast<FuncType>(_FlightPlannerUpdate)(a_plannerSubPtr, a_deltaTime, a_outMovementIntention, a_context);
	}


	void PathingHook::UpdateFlightPathData(std::byte* a_agent) {
		if (!a_agent) {
log::warn("IDRC - {}: Agent pointer is null", __func__);
			return;
		}

		auto* actorState = *reinterpret_cast<RE::ActorState**>(a_agent + 0x10);
		if (!actorState) {
log::warn("IDRC - {}: ActorState pointer is null", __func__);
			return;
		}

		auto* actor = reinterpret_cast<RE::Actor*>(reinterpret_cast<std::uintptr_t>(actorState) - m_actorOffset);
		auto* dragonActor = IDRC::DataManager::GetSingleton().GetDragonActor();

		if(!IDRC::CameraLockManager::GetSingleton().IsEnabled() || 
		   !dragonActor ||
		   actor != dragonActor ||
		   IDRC::FlyingModeManager::GetSingleton().GetFlyingMode() != IDRC::FlyingMode::kFlying ||
		   _ts_SKSEFunctions::GetFlyingState(dragonActor) != 2) 
		{
			// only update path data for the mounted dragon, while in flying mode and in the correct flying state
log::info("IDRC - {}: Skipping path update. Conditions not met. CameraLockEnabled={}, dragonActor valid={}, actor is dragon={}, flying mode={}, flying state={}", __func__, IDRC::CameraLockManager::GetSingleton().IsEnabled(), dragonActor != nullptr, actor == dragonActor, IDRC::FlyingModeManager::GetSingleton().GetFlyingMode() == IDRC::FlyingMode::kFlying, _ts_SKSEFunctions::GetFlyingState(dragonActor));
			return;
		}

		auto currentIndex = *reinterpret_cast<uint32_t*>(a_agent + 0x58);
		auto* pathData = *reinterpret_cast<std::byte**>(a_agent + 0x48);
		if (!pathData) {
			// Each FastTravel (ExecuteTeleport) triggers re-pathing. 
			// In that track, pathData is set to nullptr until the new pathData is generated. 
			// It can take a few frames until the new pathData is generated, 
			// in particular if many pathing requests occur in parallel, eg during combat. 
			// In this case we skip the next FastTravel request to allow engine to catch up with re-pathing. 
			// Flight pathData will not be updated until new pathData is generated, 
			// The dragon will fly straight forward without path data during these frames (typically 1-3)
			
			IDRC::FastTravelManager::GetSingleton().SkipFastTravelRequest(true);
			return;
		}
		// If pathData is valid, allow FastTravel
		IDRC::FastTravelManager::GetSingleton().SkipFastTravelRequest(false);

		// a_pathData + 0x90 = waypointArray_90 (float* base)
		// a_pathData + 0xA0 = waypointCount_A0 (uint32)
		auto* wayPointBase  = *reinterpret_cast<float**>(pathData + 0x90);
		auto  wayPointCount = *reinterpret_cast<std::uint32_t*>(pathData + 0xA0);

		if (!wayPointBase || wayPointCount == 0) {
log::warn("IDRC - {}: wayPointBase null=? wayPointCount: {}", __func__, wayPointCount);
			return;
		}

		auto* playerCamera = RE::PlayerCamera::GetSingleton();
		RE::ThirdPersonState* dragonCameraState = nullptr;

		if (playerCamera && playerCamera->currentState && (playerCamera->currentState->id == RE::CameraState::kDragon)) {
			dragonCameraState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
			if (!dragonCameraState) {
				log::warn("IDRC - {}: Dragon camera state is null", __func__);
				return;
			}
		} else {
			log::warn("IDRC - {}: Player camera state is not DragonCameraState", __func__);
			return;
		}

		const float targetPitch = _ts_SKSEFunctions::GetPitch(dragonCameraState->rotation);
		float targetYaw = _ts_SKSEFunctions::GetYaw(dragonCameraState->rotation);

		const RE::NiPoint3 dragonPos = dragonActor->GetPosition();
		auto& combatManager = IDRC::CombatManager::GetSingleton();
		if (combatManager.IsShoutActive() && combatManager.GetShoutTarget()) {
			// If a shout target exists, use its position as the yaw target to keep the dragon oriented towards the target while shouting.
			// Also see the similar logic in CameraLockManager::Update(), to place the orbit marker in the direction of the shout target.
			auto orbitMarker = IDRC::DataManager::GetSingleton().GetOrbitMarker();
			if (!orbitMarker) {
				log::warn("IDRC - {}: Orbit marker is null", __func__);
				return;
			}
			auto targetPos = orbitMarker->GetPosition();
			targetYaw = std::atan2f(targetPos.x - dragonPos.x, targetPos.y - dragonPos.y);
		}

		const float sinYaw = std::sin(targetYaw);
		const float cosYaw = std::cos(targetYaw);
		const float tanPitch = std::tan(targetPitch);
		const float minHeightAboveGround = 500.f;

		// Each entry is 0x48 bytes; XYZ floats at byte offsets +0, +4, +8
		constexpr std::size_t kStride = 0x48 / sizeof(float);  // = 0x12 floats
		for (std::uint32_t i = 0; i < std::min(std::max(wayPointCount - 2, 0u), currentIndex + 8u); ++i) {
			// #Adjusting the first waypoints to impact the immediate path.
			// Other waypoints do not be adjusted in this frame,
			// because they do not impact the form of the interpolated path close to the dragon.  
			auto& waypointToUpdate = *reinterpret_cast<RE::NiPoint3*>(&wayPointBase[i * kStride]);
			float dx = waypointToUpdate.x - dragonPos.x;
			float dy = waypointToUpdate.y - dragonPos.y;
			float distanceToUpdate = std::sqrt(dx * dx + dy * dy);
			float cameraZ = dragonPos.z + distanceToUpdate * tanPitch;
			float landZ = _ts_SKSEFunctions::GetLandHeightWithWater(waypointToUpdate, true) + minHeightAboveGround;

			waypointToUpdate.x = dragonPos.x + distanceToUpdate * sinYaw;
			waypointToUpdate.y = dragonPos.y + distanceToUpdate * cosYaw;			
			waypointToUpdate.z = std::max(cameraZ, landZ);
		}
log::info("IDRC - {}: Updated waypoints for flying dragon. CurrentIndex: {}, WaypointCount: {}", __func__, currentIndex, wayPointCount);
for (std::uint32_t i = 0; i < std::min(wayPointCount - 2, currentIndex + 8u); ++i) {
auto& waypointToUpdate = *reinterpret_cast<RE::NiPoint3*>(&wayPointBase[i * kStride]);
if (i == currentIndex) {
APIs::TrueHUD->DrawPoint(waypointToUpdate, 10.0f, 0.0f, 0x00FF00FF);
} else {
APIs::TrueHUD->DrawPoint(waypointToUpdate, 5.0f, 0.0f, 0xFF0000FF);
}
}		

	}


	void PathingHook::SetGroundPath(std::uintptr_t  a_subPtr, std::uintptr_t* a_newNode, std::uintptr_t* a_newData) {
	// This hook fires whenever an actor needs updated pathing,  including dragons in flyingstate == 0 (Landed)
	// Used to modify pathing for mounted dragon in flyingState == 0 (Landed)
	// This is to improve handling of obstacles, and avoid that the dragon deviates from the straight
	// path that has been directed by the player, if possible.

		if (_SetGroundPath == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;

		}
	
		// call the original function
		using FuncType = decltype(&SetGroundPath);
		reinterpret_cast<FuncType>(_SetGroundPath)(a_subPtr, a_newNode, a_newData);

		// modify waypoints for grounded, mounted dragon

		auto* dragonActor = IDRC::DataManager::GetSingleton().GetDragonActor();
		if (!dragonActor) {
			// no dragon mounted
			return;
		} else if (_ts_SKSEFunctions::GetFlyingState(dragonActor) != 0 || 
					IDRC::FlyingModeManager::GetSingleton().GetFlyingMode() != IDRC::FlyingMode::kLanded) {
			// not in landed state
			return;
		}

		auto agent = reinterpret_cast<std::byte*>(a_subPtr - 0x20);
		auto* actorState = *reinterpret_cast<RE::ActorState**>(agent + 0x10);
		if (!actorState) {
			return;
		}

		auto* actor = reinterpret_cast<RE::Actor*>(reinterpret_cast<std::uintptr_t>(actorState) - m_actorOffset);
		if(actor != dragonActor) {
			// not the mounted dragon
			return;
		}

		// Read waypoints from agent+0x48 (pathData_48), which is now set
		// a_subPtr + 0x28 = agent+0x48 = pathData_48
		auto* pathData = reinterpret_cast<std::byte*>(
		*reinterpret_cast<std::uintptr_t*>(a_subPtr + 0x108));
		if (!pathData) {
			return;
		}

		// pathData + 0x90 = waypointArray_90 (float* base)
		// pathData + 0xA0 = waypointCount_A0 (uint32)
		auto* wayPointBase  = *reinterpret_cast<float**>(pathData + 0x90);
		auto  wayPointCount = *reinterpret_cast<std::uint32_t*>(pathData + 0xA0);


		if (!wayPointBase || wayPointCount == 0) {
			return;
		}

		// Each entry is 0x48 bytes; XYZ floats at byte offsets +0, +4, +8
		constexpr std::size_t kStride = 0x48 / sizeof(float);  // = 0x12 floats

		auto& firstWayPoint = *reinterpret_cast<RE::NiPoint3*>(&wayPointBase[0]);
		auto& lastWayPoint = *reinterpret_cast<RE::NiPoint3*>(&wayPointBase[(wayPointCount - 1) * kStride]);

		for (std::uint32_t i = 0; i < wayPointCount; ++i) {
			// modify waypoints -  linearly interpolate between first and last waypoint
			// then adjust z to be above ground
			const float t = (wayPointCount > 1) ?
				(static_cast<float>(i) / static_cast<float>(wayPointCount - 1)) :
				0.0f;

			RE::NiPoint3 currentWayPoint;
			currentWayPoint.x = firstWayPoint.x + (lastWayPoint.x - firstWayPoint.x) * t;
			currentWayPoint.y = firstWayPoint.y + (lastWayPoint.y - firstWayPoint.y) * t;
			currentWayPoint.z = firstWayPoint.z + (lastWayPoint.z - firstWayPoint.z) * t;

			float landHeight = _ts_SKSEFunctions::GetLandHeightWithWater(currentWayPoint, true);
			currentWayPoint.z = landHeight + 20.f;

			// update waypoint position
			wayPointBase[i * kStride] = currentWayPoint.x;
			wayPointBase[i * kStride + 1] = currentWayPoint.y;
			wayPointBase[i * kStride + 2] = currentWayPoint.z;
		}
	}

	void CombatHook::StartCombat(RE::Actor* a_this, RE::Actor* a_target, RE::CombatGroup* a_combatGroup) {
		if (_StartCombat == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}

//log::info("IDRC - {}: StartCombat called for actor {} on target {}", __func__, a_this ? a_this->GetName() : "null", a_target ? a_target->GetName() : "null");
		reinterpret_cast<decltype(&StartCombat)>(_StartCombat)(a_this, a_target, a_combatGroup);
	}

	void CombatHook::StopCombat(RE::Actor* a_actor) {
		if (_StopCombat == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}

		reinterpret_cast<decltype(&StopCombat)>(_StopCombat)(a_actor);
//log::info("IDRC - {}: StopCombat original function returned", __func__);
	}

	void ActivateHandlerHook::sub_140708bf0(RE::ActivateHandler* a_this, std::uint64_t a_param2, std::uint64_t a_param3, bool a_param4)
	{
		if (_sub_140708bf0 == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
//log::info("IDRC - {}: --------------->>>>>>>>>>>>>> sub_140708bf0 called", __func__);
		reinterpret_cast<decltype(&sub_140708bf0)>(_sub_140708bf0)(a_this, a_param2, a_param3, a_param4);
	}

	void ActivateHandlerHook::sub_1406a9f90(RE::PlayerCharacter* a_this)
	{
		
		if (_sub_1406a9f90 == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: --------------->>>>>>>>>>>>>> sub_1406a9f90 (player) called", __func__);
		reinterpret_cast<decltype(&sub_1406a9f90)>(_sub_1406a9f90)(a_this);
	}

	void ActivateHandlerHook::FUN_1406ba8e0(std::uint64_t a_param1, std::uint64_t a_param2, std::uint64_t a_param3, bool a_param4)
	{
		if (_FUN_1406ba8e0 == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: --------------->>>>>>>>>>>>>> FUN_1406ba8e0 called", __func__);
		reinterpret_cast<decltype(&FUN_1406ba8e0)>(_FUN_1406ba8e0)(a_param1, a_param2, a_param3, a_param4);
	}

	bool ActivateHandlerHook::ObjectRefActivate(void* a_this,void* a_activator,void* a_arg2,void* a_object,void* a_count,
               bool a_defaultProcessingOnly)
	{
		if (_ObjectRefActivate == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return false;
		}
log::info("IDRC - {}: --------------->>>>>>>>>>>>>> ObjectRefActivate called", __func__);
		return reinterpret_cast<decltype(&ObjectRefActivate)>(_ObjectRefActivate)(a_this, a_activator, a_arg2, a_object, a_count, a_defaultProcessingOnly);
	}

	void ActivateHandlerHook::FlyingMountTriggerLand(void* a_this, void* a_param2, void* a_param3, void* a_param4)
	{
		if (_FlyingMountTriggerLand == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: --------------->>>>>>>>>>>>>> FlyingMountTriggerLand called", __func__);
		reinterpret_cast<decltype(&FlyingMountTriggerLand)>(_FlyingMountTriggerLand)(a_this, a_param2, a_param3, a_param4);
	}

	void ActivateHandlerHook::FlyingMountActivate(void* a_this, void* a_param2)
	{
		if (_FlyingMountActivate == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: --------------->>>>>>>>>>>>>> FlyingMountActivate called", __func__);
		reinterpret_cast<decltype(&FlyingMountActivate)>(_FlyingMountActivate)(a_this, a_param2);
	}
/* UNUSED HOOKS:	

	bool ExecuteTeleportHook::ExecuteTeleport(
		RE::PlayerCharacter* a_this)
	{
		if (_ExecuteTeleport == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return false;
		}
		if (m_blockExecuteTeleport) {
			if (auto* loc = IDRC::Utils::GetQueuedTargetLoc(a_this); loc->isValid) {
log::info("{}: --------------->>>>>>>>>>>> ExecuteTeleport called - isValid == true", __FUNCTION__);
				loc->isValid = false;
			}
		}
		using FuncType = bool(*)(RE::PlayerCharacter*);
log::info("{}: Calling original ExecuteTeleport", __FUNCTION__);
		return reinterpret_cast<FuncType>(_ExecuteTeleport)(a_this);
	}


	void UpdateFlyingMountFastTravelHook::UpdateFastTravel(float a_deltaTime) {
		if (_UpdateFastTravel == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: UpdateFastTravel called, deltaTime: {}", __FUNCTION__, a_deltaTime);
		reinterpret_cast<decltype(&UpdateFastTravel)>(_UpdateFastTravel)(a_deltaTime);
	}

	void UpdateFlyingMountFastTravelHook::UpdateFastTravelState() {
		if (_UpdateFastTravelState == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: UpdateFlyingMountFastTravelState called", __FUNCTION__);
		reinterpret_cast<decltype(&UpdateFastTravelState)>(_UpdateFastTravelState)();
	}

	void UpdateFlyingMountFastTravelHook::UpdatePatrolQueuedState(uint32_t a_mode) {
		if (_UpdatePatrolQueuedState == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: UpdatePatrolQueuedState called for mode {}", __FUNCTION__, a_mode);
		reinterpret_cast<decltype(&UpdatePatrolQueuedState)>(_UpdatePatrolQueuedState)(a_mode);
	}

	void UpdateFlyingMountFastTravelHook::ApproachTarget(RE::NiPoint3* a_targetPos,
									std::uint64_t a_modeRaw,
									std::uint64_t a3,
									std::uint64_t a4) {
		if (_ApproachTarget == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: ApproachTarget called, ({}, {}, {}), modeRaw: {}, a3: {}, a4: {}", __FUNCTION__, a_targetPos->x, a_targetPos->y, a_targetPos->z, a_modeRaw, a3, a4);
		reinterpret_cast<decltype(&ApproachTarget)>(_ApproachTarget)(a_targetPos, a_modeRaw, a3, a4);
	}

	void UpdateFlyingMountFastTravelHook::ExecuteArrival() {
		if (_ExecuteArrival == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: ExecuteArrival called", __FUNCTION__);
		reinterpret_cast<decltype(&ExecuteArrival)>(_ExecuteArrival)();
	}	

	void PapyrusFastTravelHook::FastTravel(RE::BSScript::IVirtualMachine* a_vm,       // Papyrus VM (for error reporting)
											RE::VMStackID                  a_stackID,  // Calling script's stack ID
											RE::StaticFunctionTag*         a_staticTag, // Static function tag placeholder (unused)
											RE::TESObjectREFR*             a_location ) {
		if (_FastTravel == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: FastTravel called ", __FUNCTION__);
		reinterpret_cast<decltype(&FastTravel)>(_FastTravel)(a_vm, a_stackID, a_staticTag, a_location);
	}


	bool VoiceSpellCastHook::HandleVoiceSpellCast(std::uintptr_t  a_this, RE::Actor* a_caster) {
		if (_HandleVoiceSpellCast == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return false;
		}

log::info("IDRC - {}: HandleVoiceSpellCast called for caster {}", __func__, a_caster ? a_caster->GetName() : "null");

			// call the original function
		auto result = reinterpret_cast<decltype(&HandleVoiceSpellCast)>(_HandleVoiceSpellCast)(a_this, a_caster);

		return result;
	}

	void VoiceShoutCastHook::VoiceShoutCast(RE::Actor* a_caster) {
		if (_VoiceShoutCast == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: VoiceShoutCast called", __func__);

		if(a_caster) {
			log::info("IDRC - {}: Caster is {}", __func__, a_caster->GetFormID());
		} else {
			log::warn("IDRC - {}: Caster is null", __func__);
		}
		// call the original function
		reinterpret_cast<decltype(&VoiceShoutCast)>(_VoiceShoutCast)(a_caster);
log::info("IDRC - {}: VoiceShoutCast original function returned", __func__);
	}

	bool StartCastHook::StartCast(RE::Actor* a_caster, RE::MagicSystem::CastingSource a_source) {
		if (_StartCast == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return false;
		}
log::info("IDRC - {}: StartCast called ...", __func__);
		// call the original function
		auto result = reinterpret_cast<decltype(&StartCast)>(_StartCast)(a_caster, a_source);

		return result;
	}

	void CastSpellImmediateHook::CastSpellImmediate(RE::MagicCaster* _a_magicCaster,
													RE::MagicItem* _a_spell,
													bool _a_loadCast,
													RE::TESObjectREFR* _a_desiredTargetRef,
													float _a_effectivenessMult,
													bool _a_adjustOnlyHostileEffectiveness,
													float _a_magnitudeOverride) {		
		
		if (_CastSpellImmediate == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: CastSpellImmediate called ", __func__);
		// call the original function
		reinterpret_cast<decltype(&CastSpellImmediate)>(_CastSpellImmediate)(_a_magicCaster, _a_spell, _a_loadCast, _a_desiredTargetRef, _a_effectivenessMult, _a_adjustOnlyHostileEffectiveness, _a_magnitudeOverride);
log::info("IDRC - {}: CastSpellImmediate original function returned", __func__);
	}

	bool ApplyCastHook::ApplyCast(RE::MagicCaster* _a_magicCaster,
								float _a_effectivenessMult,
								std::uint32_t* _a_targetCount,
								RE::TESBoundObject* _a_source,
								bool _a_loadCast,
								bool _a_adjustOnlyHostileEffectiveness) {
		if (_ApplyCast == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return false;
		}

if (_a_source) {
	log::info("IDRC - {}: ApplyCast called with source {} ({}) with min ({}, {}, {}), max ({}, {}, {})", __func__, 
		_a_source->GetFormID(), _a_source->GetName(), _a_source->boundData.boundMin.x, _a_source->boundData.boundMin.y, _a_source->boundData.boundMin.z, _a_source->boundData.boundMax.x, _a_source->boundData.boundMax.y, _a_source->boundData.boundMax.z);
} else {
	log::warn("IDRC - {}: ApplyCast called with null source", __func__);
}

bool hasMagicCaster = false;
if (!_a_magicCaster) {
	log::warn("IDRC - {}: ApplyCast called with null magic caster", __func__);
} else {
	log::info("IDRC - {}: ApplyCast called with magic caster {}", __func__, _a_magicCaster->GetCasterAsActor() ? _a_magicCaster->GetCasterAsActor()->GetName() : "null");
	hasMagicCaster = true;
}

		// call the original function
		auto result = reinterpret_cast<decltype(&ApplyCast)>(_ApplyCast)(_a_magicCaster, _a_effectivenessMult, _a_targetCount, _a_source, _a_loadCast, _a_adjustOnlyHostileEffectiveness);
log::info("IDRC - {}: ApplyCast original function returned", __func__);
		return result;
	}

	bool TestCastHook::TestCast(RE::MagicCaster* a_magicCaster,
								RE::MagicItem* a_spell,
								RE::Actor* a_target,
								RE::TESBoundObject* a_source,
								bool a_loadCast) {
		if (_TestCast == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return false;
		}
log::info("IDRC - {}: TestCast called ", __func__);
		// call the original function
auto actorMagicCaster = static_cast<RE::ActorMagicCaster*>(a_magicCaster);
if (actorMagicCaster) {
	auto node = actorMagicCaster->magicNode;
	if (node) {
auto head = _ts_SKSEFunctions::GetTargetPoint(actorMagicCaster->GetCasterAsActor(),  RE::BGSBodyPartDefs::LIMB_ENUM::kHead);

if (!head) {
    log::error("IDRC - {}: Error: Could not get head node for dragonActor", __func__);
    return false;
}
actorMagicCaster->magicNode = head->AsNode(); // for testing, temporarily set magicNode to head node, to see if that fixes the issue with dragon's magic hands not being visible in the shout animation

		log::info("IDRC - {}: Magic caster node set to head!!", __func__);
	} else {
		log::warn("IDRC - {}: Magic caster node is null", __func__);
	}
} else {
	log::warn("IDRC - {}: Magic caster is null", __func__);
}
		auto result = reinterpret_cast<decltype(&TestCast)>(_TestCast)(a_magicCaster, a_spell, a_target, a_source, a_loadCast);
log::info("IDRC - {}: TestCast original function returned", __func__);
		return result;
	}

	void ProcedureShoutHook::Initiate(std::uint64_t*  a_this,     // param_1: BGSProcedureShout*
    							std::uint64_t*  a_context) {
		if (_Initiate == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: ProcedureShoutHook::Initiate called ", __func__);
		// call the original function
		reinterpret_cast<decltype(&Initiate)>(_Initiate)(a_this, a_context);
log::info("IDRC - {}: ProcedureShoutHook::Initiate original function returned", __func__);
	}

	void ProcedureShoutHook::SetupExecState(std::uint64_t*  a_this,     // param_1: BGSProcedureShout*
								std::uint64_t*  a_context) {
		if (_SetupExecState == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("IDRC - {}: ProcedureShoutHook::SetupExecState called ", __func__);
		// call the original function
		reinterpret_cast<decltype(&SetupExecState)>(_SetupExecState)(a_this, a_context);
log::info("IDRC - {}: ProcedureShoutHook::SetupExecState original function returned", __func__);
	}

	bool StartVoiceShoutCastHook::StartVoiceShoutCast(RE::Character* a_caster,
													RE::TESShout* a_shout,
													std::uint32_t a_wordIndex,
													RE::Actor* a_target) {
		if (_StartVoiceShoutCast == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return false;
		}
log::info("IDRC - {}: StartVoiceShoutCast called, caster: {}, shout: {}, wordIndex: {}, target: {}", __func__,
	 a_caster ? a_caster->GetName() : "null", a_shout ? a_shout->GetName() : "null", a_wordIndex, a_target ? a_target->GetName() : "null");
		// call the original function
		auto result = reinterpret_cast<decltype(&StartVoiceShoutCast)>(_StartVoiceShoutCast)(a_caster, a_shout, a_wordIndex, a_target);
log::info("IDRC - {}: StartVoiceShoutCast original function returned {}", __func__, result ? "true" : "false");
		return result; 
	}
*/
} // namespace Hooks
