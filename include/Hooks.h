#pragma once

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

