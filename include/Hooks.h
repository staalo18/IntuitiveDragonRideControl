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
	class FlightPathHook
	{
	public:
		static void Hook()
		{
			_SetPath = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(90008, 92493, 0),
				6,
				reinterpret_cast<std::uintptr_t>(SetPath));
		}

	private:
		static void SetPath(std::uintptr_t  a_subPtr, std::uintptr_t* a_newNode, std::uintptr_t* a_newData);
		static inline std::uintptr_t _SetPath{ 0 };
	};


	class GroundPathHook
	{
	public:
		static void Hook()
		{
			_SetPath = _ts_SKSEFunctions::WriteFunctionHook(
				REL::VariantID(88302, 90713, 0),
				5,
				reinterpret_cast<std::uintptr_t>(SetPath));
		}

	private:
		static void SetPath(std::uintptr_t  a_subPtr, std::uintptr_t* a_newNode, std::uintptr_t* a_newData);
		static inline std::uintptr_t _SetPath{ 0 };
	};

/*  UNUSED HOOKS:
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

