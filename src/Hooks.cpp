#include "Hooks.h"
#include "TargetReticleManager.h"
#include "CameraLockManager.h"
#include "FlyingModeManager.h"
#include "DataManager.h"
#include "IDRCUtils.h"
#include "APIManager.h"

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
		FlightPathHook::Hook();
		GroundPathHook::Hook();

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


	void FlightPathHook::SetPath(std::uintptr_t  a_subPtr, std::uintptr_t* a_newNode, std::uintptr_t* a_newData) {

	// This hook fires for all dragons in flyingstate == 2 (Flying), when they need to update their pathing
	// Used to keep mounted dragon on a straight path during commanded flight, regardless of the terrain height

		if (_SetPath == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}

		// call the original function
		using FuncType = decltype(&SetPath);
		reinterpret_cast<FuncType>(_SetPath)(a_subPtr, a_newNode, a_newData);

		// Now adjusting the first 4 waypoints' heights in the new path data to consider camera-relative height control.
		// The engine is using a Bezier curve to interpolate between waypoints,
		// so adjusting 4 points to impact the immediate path. 
		// Other waypoints do not be adjusted in this frame,
		// because they do not impact the form of the interpolated path btw dragon and waypoints 0 and 1.  

		if(!IDRC::CameraLockManager::GetSingleton().IsEnabled()) {
			return;
		}

		// Read waypoints from agent+0x48 (pathData_48), which is now set
		// a_subPtr + 0x28 = agent+0x48 = pathData_48
		auto* pathData = reinterpret_cast<std::byte*>(
			*reinterpret_cast<std::uintptr_t*>(a_subPtr + 0x28));

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

		auto* dragonActor = IDRC::DataManager::GetSingleton().GetDragonActor();
		if (!dragonActor) {
			return;
		}

		if (IDRC::FlyingModeManager::GetSingleton().GetFlyingMode() != IDRC::FlyingMode::kFlying) {
			return;
		}

		if (_ts_SKSEFunctions::GetFlyingState(dragonActor) != 2) { // not in flying state
			return;
		}

		// Filter: verify this path belongs to the mounted dragon by checking that
		// waypoint[0] (the agent's current position) is close to the mounted dragon.
		// Any other flying dragon's starting point usually is further away from the mounted dragon.
		// Risk here is that the mounted dragon's first waypoint is further than 1000 units from the mounted dragon,
		// but I've never seen that in my testing (usually distance is about 100).
		// Other risk is that another dragon is closer in the x/y plane than 1000 units.
		// in that case that dragon's height will be adjusted as an unwanted side effect 
		// for the duration of the other dragon being closer than 1000 units.

		auto& firstWaypoint  = *reinterpret_cast<RE::NiPoint3*>(&wayPointBase[0]);
		const RE::NiPoint3 dragonPos = dragonActor->GetPosition();
		float dx  = firstWaypoint.x - dragonPos.x;
		float dy  = firstWaypoint.y - dragonPos.y;
		constexpr float checkDistSq = 1000.f * 1000.f;
		if (dx * dx + dy * dy > checkDistSq) {
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

		const float cameraPitch = _ts_SKSEFunctions::GetPitch(dragonCameraState->rotation);
		const float cameraYaw = _ts_SKSEFunctions::GetYaw(dragonCameraState->rotation);
		const float sinYaw = std::sin(cameraYaw);
		const float cosYaw = std::cos(cameraYaw);
		const float tanPitch = std::tan(cameraPitch);
		const float minHeightAboveGround = 500.f;

		// Each entry is 0x48 bytes; XYZ floats at byte offsets +0, +4, +8
		constexpr std::size_t kStride = 0x48 / sizeof(float);  // = 0x12 floats
		for (std::uint32_t i = 0; i < std::min(wayPointCount - 1, 8u); ++i) {
			// Update the first 8 waypoints in the path data
			// to keep dragon on straight line
			auto& waypointToUpdate = *reinterpret_cast<RE::NiPoint3*>(&wayPointBase[i * kStride]);
			dx = waypointToUpdate.x - dragonPos.x;
			dy = waypointToUpdate.y - dragonPos.y;
			float distanceToUpdate = std::sqrt(dx * dx + dy * dy);

			waypointToUpdate.x = dragonPos.x + distanceToUpdate * sinYaw;
			waypointToUpdate.y = dragonPos.y + distanceToUpdate * cosYaw;
			
			if (i < 4) {
				// only update height for the first 4 waypoints
				// to keep sensitivity for upcoming obstacles
				float cameraZ = dragonPos.z + distanceToUpdate * tanPitch;
				float landHeight = _ts_SKSEFunctions::GetLandHeightWithWater(waypointToUpdate, true);
				waypointToUpdate.z = std::max(cameraZ, landHeight + minHeightAboveGround);
			}

//if (APIs::TrueHUD) {
//	APIs::TrueHUD->DrawPoint(waypointToUpdate, 5.0f, 0.1f, 0xFF0000FF);
//}
		}
	}


	void GroundPathHook::SetPath(std::uintptr_t  a_subPtr, std::uintptr_t* a_newNode, std::uintptr_t* a_newData) {
	// This hook fires whenever an actor needs updated pathing,  including dragons in flyingstate == 0 (Landed)
	// Used to modify pathing for mounted dragon in flyingState == 0 (Landed)
	// This is to improve handling of obstacles, and avoid that the dragon deviates from the straight
	// path that has been directed by the player, if possible.

		if (_SetPath == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;

		}
	
		// call the original function
		using FuncType = decltype(&SetPath);
		reinterpret_cast<FuncType>(_SetPath)(a_subPtr, a_newNode, a_newData);

		// modify waypoints for grounded, mounted dragon

		auto* dragonActor = IDRC::DataManager::GetSingleton().GetDragonActor();
		if (!dragonActor) {
			return;
		} else if (_ts_SKSEFunctions::GetFlyingState(dragonActor) != 0 || 
					IDRC::FlyingModeManager::GetSingleton().GetFlyingMode() != IDRC::FlyingMode::kLanded) {
			// not in landed state
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

		auto& firstWayPoint = *reinterpret_cast<RE::NiPoint3*>(&wayPointBase[0]);
		auto dragonPos = dragonActor->GetPosition();
		float distance2D = std::sqrt((firstWayPoint.x - dragonPos.x) * (firstWayPoint.x - dragonPos.x) + (firstWayPoint.y - dragonPos.y) * (firstWayPoint.y - dragonPos.y));
		bool isDragonPath = (distance2D < 50.f);

		if (!isDragonPath) {
			return;
		}

		// Each entry is 0x48 bytes; XYZ floats at byte offsets +0, +4, +8
		constexpr std::size_t kStride = 0x48 / sizeof(float);  // = 0x12 floats

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
} // namespace Hooks
