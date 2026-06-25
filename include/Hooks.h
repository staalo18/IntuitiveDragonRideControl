#pragma once

#include "RE/D/DragonCameraState.h"
#include <_ts_SKSEFunctions.h>
namespace Hooks
{
	class ReadyWeaponHook
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> ReadyWeaponHandlerVtbl{ RE::VTABLE_ReadyWeaponHandler[0] };
			_ProcessButton = ReadyWeaponHandlerVtbl.write_vfunc(0x4, ProcessButton);
		}

	private:
		static void ProcessButton(RE::ReadyWeaponHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data);

		static inline REL::Relocation<decltype(ProcessButton)> _ProcessButton;
	};

    class ExtraInteractionHook
    {
    public:
        static void Hook()
        {
            REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_ExtraInteraction[0] };
            _GetType = vtbl.write_vfunc(0x1, GetType);         // 0x1 = first virtual after destructor
            _IsNotEqual = vtbl.write_vfunc(0x2, IsNotEqual);   // 0x2 = second virtual after destructor
        }

    private:
        static RE::ExtraDataType GetType(const RE::ExtraInteraction* a_this);

        static bool IsNotEqual(const RE::ExtraInteraction* a_this, const RE::BSExtraData* a_rhs);

        static inline REL::Relocation<decltype(GetType)> _GetType;
        static inline REL::Relocation<decltype(IsNotEqual)> _IsNotEqual;
    };

    // MainUpdateHook is from 'True Directional Movement':
    // https://github.com/ersh1/TrueDirectionalMovement
    // All credits go to the original author Ersh!
	class MainUpdateHook
	{
	public:
		static void Hook()
		{
			auto& trampoline = SKSE::GetTrampoline();
			REL::Relocation<uintptr_t> hook{ RELOCATION_ID(35565, 36564) };  // 5B2FF0, 5D9F50, main update
			
			_Nullsub = trampoline.write_call<5>(hook.address() + RELOCATION_OFFSET(0x748, 0xC26), Nullsub);  // 5B3738, 5DAB76
		}

	private:
		static void Nullsub();
		static inline REL::Relocation<decltype(Nullsub)> _Nullsub;		
		static void UpdateRotationMatrixDisplay(RE::NiAVObject* m_reference3D); // for debugging
	};

    class LookHook
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> LookHandlerVtbl{ RE::VTABLE_LookHandler[0] };
			_ProcessThumbstick = LookHandlerVtbl.write_vfunc(0x2, ProcessThumbstick);
			_ProcessMouseMove = LookHandlerVtbl.write_vfunc(0x3, ProcessMouseMove);
		}

	private:
		static void ProcessThumbstick(RE::LookHandler* a_this, RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data);
		static void ProcessMouseMove(RE::LookHandler* a_this, RE::MouseMoveEvent* a_event, RE::PlayerControlsData* a_data);

		static inline REL::Relocation<decltype(ProcessThumbstick)> _ProcessThumbstick;
		static inline REL::Relocation<decltype(ProcessMouseMove)> _ProcessMouseMove;
	};

	class DragonCameraStateHook
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> DragonCameraStateVtbl{ RE::VTABLE_DragonCameraState[0] };
			_OnEnterState = DragonCameraStateVtbl.write_vfunc(0x1, OnEnterState);
			_UpdateRotation = DragonCameraStateVtbl.write_vfunc(0xE, UpdateRotation);
        	_GetCurrentRotation  = DragonCameraStateVtbl.write_vfunc(0x4, GetCurrentRotation);
		}

	private:
		static void OnEnterState(RE::DragonCameraState* a_this);
		static void UpdateRotation(RE::DragonCameraState* a_this);
		static void GetCurrentRotation(RE::DragonCameraState* a_this, RE::NiQuaternion& a_out);

		static inline REL::Relocation<decltype(OnEnterState)> _OnEnterState;
		static inline REL::Relocation<decltype(UpdateRotation)> _UpdateRotation;
		static inline REL::Relocation<decltype(GetCurrentRotation)> _GetCurrentRotation;
	};

/* UNUSED
	class FlightGoalHook
	{
	public:
		static void Hook()
		{
			_SetGoal = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(88429, 90853, 0),
				5,
				reinterpret_cast<std::uintptr_t>(SetGoal));
		}

	private:
		static bool SetGoal(std::uintptr_t* a_this, RE::BSPathingRequest** a_request);
		static inline std::uintptr_t _SetGoal{ 0 };
	};
*/
	class PathingHook
	{
	public:
		static void Hook()
		{
			_SetGroundPath = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(88302, 90713, 0),
				5,
				reinterpret_cast<std::uintptr_t>(SetGroundPath));

			_SetFlightPath = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(90008, 92493, 0),
				6,
				reinterpret_cast<std::uintptr_t>(SetFlightPath));

			_FlightPlannerUpdate = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(90006, 92491, 0),
				6,
				reinterpret_cast<std::uintptr_t>(FlightPlannerUpdate));
		}

	private:
		static void SetGroundPath(std::uintptr_t  a_subPtr, std::uintptr_t* a_newNode, std::uintptr_t* a_newData);
		static void SetFlightPath(std::uintptr_t  a_subPtr, std::uintptr_t* a_newNode, std::uintptr_t* a_newData);
		static void FlightPlannerUpdate(std::uintptr_t a_plannerSubPtr,
									float* a_deltaTime,
									float* a_outMovementIntention,
									void* a_context);
		static void UpdateFlightPathData(std::byte* a_agent);

		static inline std::uintptr_t _SetGroundPath{ 0 };
		static inline std::uintptr_t _SetFlightPath{ 0 };
		static inline std::uintptr_t _FlightPlannerUpdate{ 0 };
		static inline std::uintptr_t m_actorOffset = REL::Module::get().version() >= SKSE::RUNTIME_SSE_1_6_629 ? 0xC0 : 0xB8;
	};


	class CombatHook
	{
	public:
		static void Hook()
		{
			_StartCombat = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(37608, 38561, 0),
				5,
				reinterpret_cast<std::uintptr_t>(StartCombat));	
			_StopCombat = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(37613, 38566, 0),
				5,
				reinterpret_cast<std::uintptr_t>(StopCombat));
		}
	private:
		static void StopCombat(RE::Actor* a_this);
		static void StartCombat(RE::Actor* a_this, RE::Actor* a_target, RE::CombatGroup* a_combatGroup);
		static inline std::uintptr_t _StartCombat{ 0 };
		static inline std::uintptr_t _StopCombat{ 0 };
	};

/*  UNUSED HOOKS:

FastTravel-related hooks:
*************************

/*
	< NOTE: all addresses are for SE version. AE addresses differ. >

- PapyrusFastTravelHook::FastTravel: the papyrus function "FastTravel". This fires ExecuteTeleport via queuedTargetLoc.

- ExecuteTeleport is called once per frame from the main update loop (0x1405b2ff0).
	It fires if queuedTargetLoc is valid, eg via papyrus FastTravel request, COC, and on loadgame / at startup:

		[game startup / BGS load path]
			TeleportPlayerToCell (TeleportPlayertoCellHook)
			└─ ExecuteTeleport (ExecuteTeleportHook) (called once per frame, fires if queuedTargetLoc is valid)
					├─ TeleportToCell  (TeleportHook)              ← queuedTargetLoc has interior cell
					│    └─ SwitchToInteriorCell (SwitchToCellHook)
					└─ TeleportToWorldspacePosition (TeleportHook) ← queuedTargetLoc has worldspace
						└─ TeleportToCell (TeleportHook)
							├─ SwitchToInteriorCell (SwitchToCellHook)  (if target is interior; N/A on this path)
							├─ SwitchToWorldspace (SwitchToCellHook)    (if worldspace differs from current)
							└─ SwitchToExteriorCell (SwitchToCellHook)   (if target is exterior)
								└─ SwitchToWorldspace (SwitchToCellHook)  (conditional: interior-surplus purge path)

			[console: coc <x> <y>]
			ConsoleFunc__CenterOnExterior
			└─ PlayerCharacter__CenterOnCell (CenterOnCellHook)
				└─ ExecuteTeleport (ExecuteTeleportHook)  (same subtree as above)
			
			[papyrus FastTravel]
			└─ ExecuteTeleport (ExecuteTeleportHook)  (same subtree as above)

	See IDRC::Utils::GetQueuedTargetLoc for how to obtain and modify queuedTargetLoc.

- UpdateFlyingMountFastTravelHook::UpdateFastTravel (0x1406badd0, REL::VariantID(39716, 40818, 0)) 
    is called once per frame from the main player update loop (0x14069e580).
	UpdateFastTravel is handling the flying mount logic once airborne. 
	It calls ApproachTarget (REL::VariantID(39705, 40807, 0)), and "DispatchFlyingMountTravelMode" (not hooked here, REL::VariantID(39714, 40816, 0)).
	Those two functions in turn trigger the FastTravelState and PatrolQueuedState updates
	(UpdateFastTravelState -> EnterFlyingMountFastTravelState, UpdatePatrolQueuedState). 
	PatroQueued states are:
		0 = idle
		1 = queue FastTravel    → EnterFlyingMountFastTravelState (REL::VariantID(39703, 40805, 0))
		2 = queue dismount      → FUN_1406ba750
		3 = queue set-injured   → FUN_1406ba940
		4 = queue eval-package  → FUN_1406bac00

- UpdateFlyingMountFastTravelHook::ApproachTarget handles the flying-mount approach logic toward the target.
	It is called by UpdateFastTravel, ExecuteArrival and ExecuteTeleport.

- UpdateFlyingMountFastTravelHook::ExecuteArrival is triggered through these call chains:
	* SneakHandler::ProcessButton (0x1407094c0)->0x1406b6030->TESObjectREFR::sub_1406ba9b0->0x1406ba710-> ExecuteArrival
	* ActivateHandler::sub_140708bf0->PlayerCharacter::sub_1406a9f90->0x1406ba8e0->ExecuteArrival
*/
/*

	class ExecuteTeleportHook
	{
	public:
		static void Hook()
		{
			_ExecuteTeleport = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(39366, 40438, 0),
				5,
				reinterpret_cast<std::uintptr_t>(ExecuteTeleport)
			);
		}

		static bool ExecuteTeleport(RE::PlayerCharacter* a_this);
		static void BlockNextExecuteTeleport(bool a_block) { m_blockExecuteTeleport = a_block; }
		static void SetTeleportPending(bool a_pending) { m_teleportPending = a_pending; }
	private:
		static inline std::uintptr_t _ExecuteTeleport{ 0 };
		static inline bool m_blockExecuteTeleport{ false };
		static inline bool m_teleportPending{ false };
	};

	class UpdateFlyingMountFastTravelHook
	{
	public:
		static void Hook()
		{
			_UpdateFastTravel = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(39716, 40818, 0),
				5,
				reinterpret_cast<std::uintptr_t>(UpdateFastTravel));

			_UpdateFastTravelState = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(39714, 40816, 0),
				15,
				reinterpret_cast<std::uintptr_t>(UpdateFastTravelState));

			_UpdatePatrolQueuedState = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(39702, 40804, 0),
				13,
				reinterpret_cast<std::uintptr_t>(UpdatePatrolQueuedState));

			_ApproachTarget = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(39705, 40807, 0),
				6,
				reinterpret_cast<std::uintptr_t>(ApproachTarget));

			_ExecuteArrival = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(39706, 40808, 0),
				5,
				reinterpret_cast<std::uintptr_t>(ExecuteArrival));
		}

		static void UpdateFastTravel(float a_deltaTime);
		static void UpdateFastTravelState();
		static void UpdatePatrolQueuedState(uint32_t a_mode);
		static void ApproachTarget(RE::NiPoint3* a_targetPos,
									std::uint64_t a_modeRaw,
									std::uint64_t a3,
									std::uint64_t a4);
		static void ExecuteArrival();
	private:
		static inline std::uintptr_t _UpdateFastTravel{ 0 };
		static inline std::uintptr_t _UpdateFlyingMountFastTravelState{ 0 };
		static inline std::uintptr_t _UpdateFastTravelState{ 0 };
		static inline std::uintptr_t _UpdatePatrolQueuedState{ 0 };
		static inline std::uintptr_t _ApproachTarget{ 0 };
		static inline std::uintptr_t _ExecuteArrival{ 0 };
	};


	class PapyrusFastTravelHook
	{
	public:
		static void Hook()
		{
			_FastTravel = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(54824, 55457, 0),
				6,
				reinterpret_cast<std::uintptr_t>(FastTravel));
		}

	private:
		static void FastTravel(RE::BSScript::IVirtualMachine* a_vm,       // Papyrus VM (for error reporting)
								RE::VMStackID                  a_stackID,  // Calling script's stack ID
								RE::StaticFunctionTag*         a_staticTag, // Static function tag placeholder (unused)
								RE::TESObjectREFR*             a_location );
		static inline std::uintptr_t _FastTravel{ 0 };
	};


END FastTravel-related hooks
****************************

	class VoiceSpellCastHook
	{
	public:
		static void Hook()
		{
			_HandleVoiceSpellCast = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(41740, 42821, 0),
				6,
				reinterpret_cast<std::uintptr_t>(HandleVoiceSpellCast));
		}
	private:
		static bool HandleVoiceSpellCast(std::uintptr_t  a_this, RE::Actor* a_caster);
		static inline std::uintptr_t _HandleVoiceSpellCast{ 0 };
	};

	class VoiceShoutCastHook
	{
	public:
		static void Hook()
		{
			_VoiceShoutCast = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(33663, 34443, 0),
				6,
				reinterpret_cast<std::uintptr_t>(VoiceShoutCast));
		}
		static void VoiceShoutCast(RE::Actor* a_caster);
	private:
		static inline std::uintptr_t _VoiceShoutCast{ 0 };
	};

	class StartCastHook
	{
	public:
		static void Hook()
		{
			_StartCast = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(41823, 42904, 0),
				6,
				reinterpret_cast<std::uintptr_t>(StartCast));
		}
		static bool StartCast(RE::Actor* a_caster, RE::MagicSystem::CastingSource a_source);
	private:
		static inline std::uintptr_t _StartCast{ 0 };
	};

	class CastSpellImmediateHook
	{
	public:
		static void Hook()
		{
			_CastSpellImmediate = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(33626, 34404, 0),
				5,
				reinterpret_cast<std::uintptr_t>(CastSpellImmediate));
		}
		static void CastSpellImmediate(
    RE::MagicCaster* _a_magicCaster,
    RE::MagicItem* _a_spell,
    bool _a_loadCast,
    RE::TESObjectREFR* _a_desiredTargetRef,
    float _a_effectivenessMult,
    bool _a_adjustOnlyHostileEffectiveness,
    float _a_magnitudeOverride);
	private:
		static inline std::uintptr_t _CastSpellImmediate{ 0 };
	};

	class ApplyCastHook
	{
	public:
		static void Hook()
		{
			_ApplyCast = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(33632, 34410, 0),
				7,
				reinterpret_cast<std::uintptr_t>(ApplyCast));
		}
		static bool ApplyCast(RE::MagicCaster* _a_magicCaster,
								float _a_effectivenessMult,
								std::uint32_t* _a_targetCount,
								RE::TESBoundObject* _a_source,
								bool _a_loadCast,
								bool _a_adjustOnlyHostileEffectiveness);
	private:
		static inline std::uintptr_t _ApplyCast{ 0 };
	};

	class TestCastHook
	{
	public:
		static void Hook()
		{
			_TestCast = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(33631, 34409, 0),
				5,
				reinterpret_cast<std::uintptr_t>(TestCast));
		}
		static bool TestCast(RE::MagicCaster* a_magicCaster,
								RE::MagicItem* a_spell,
								RE::Actor* a_target,
								RE::TESBoundObject* a_source,
								bool a_loadCast);

	private:
		static inline std::uintptr_t _TestCast{ 0 };
	};

	class ProcedureShoutHook
	{
	public:
		static void Hook()
		{
			_Initiate = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(28332, 29080, 0),
				5,
				reinterpret_cast<std::uintptr_t>(Initiate));
			_SetupExecState = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(28329, 29077, 0),
				5,
				reinterpret_cast<std::uintptr_t>(SetupExecState));
		}
		static void Initiate(std::uint64_t*  a_this,     // param_1: BGSProcedureShout*
    						std::uint64_t*  a_context);
		static void SetupExecState(std::uint64_t*  a_this,     // param_1: BGSProcedureShout*
								std::uint64_t*  a_context);
	private:
		static inline std::uintptr_t _Initiate{ 0 };
		static inline std::uintptr_t _SetupExecState{ 0 };
	};

	class StartVoiceShoutCastHook
	{
	public:
		static void Hook()
		{
			_StartVoiceShoutCast = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(37850, 38804, 0),
				5,
				reinterpret_cast<std::uintptr_t>(StartVoiceShoutCast));
		}
		static bool StartVoiceShoutCast(RE::Character* a_caster,
										RE::TESShout* a_shout,
										std::uint32_t a_wordIndex,
										RE::Actor* a_target);
	private:
		static inline std::uintptr_t _StartVoiceShoutCast{ 0 };
	};
*/
/*
#include <MinHook.h>

class GetMountHook
{
public:
    static void Hook()
    {
        MH_Initialize();
        auto target = REL::Relocation<std::uintptr_t>(RELOCATION_ID(37757, 38702)).address();
        MH_CreateHook(
            reinterpret_cast<LPVOID>(target),
            reinterpret_cast<LPVOID>(GetMount),
            reinterpret_cast<LPVOID*>(&_original_fn)
        );
        MH_EnableHook(reinterpret_cast<LPVOID>(target));
    }

private:
    using GetMount_t = bool(RE::Actor*, RE::NiPointer<RE::Actor>&);
    static bool GetMount(RE::Actor* a_this, RE::NiPointer<RE::Actor>& a_outMount)
    {
        // Call the original function via MinHook's trampoline
        return _original_fn(a_this, a_outMount);
    }

    static inline GetMount_t* _original_fn = nullptr;
}; */
	void Install();
} // namespace Hooks	

