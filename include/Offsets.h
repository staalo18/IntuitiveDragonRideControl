#pragma once
// Offsets are from 'True Directional Movement':
// https://github.com/ersh1/TrueDirectionalMovement
// All credits go to the original author Ersh!

// variables
static uintptr_t g_worldToCamMatrix = RELOCATION_ID(519579, 406126).address();                       // 2F4C910, 2FE75F0
static RE::NiRect<float>* g_viewPort = (RE::NiRect<float>*)RELOCATION_ID(519618, 406160).address();  // 2F4DED0, 2FE8B98
static float* g_deltaTimeRealTime = (float*)RELOCATION_ID(523661, 410200).address();                 // 2F6B94C, 30064CC

// functions
typedef RE::NiAVObject*(__fastcall* tNiAVObject_LookupBoneNodeByName)(RE::NiAVObject* a_this, const RE::BSFixedString& a_name, bool a3);
static REL::Relocation<tNiAVObject_LookupBoneNodeByName> NiAVObject_LookupBoneNodeByName{ RELOCATION_ID(74481, 76207) };

typedef void(__fastcall* tNiQuaternion_SomeRotationManipulation)(RE::NiQuaternion& a1, float a2, float a3, float a4);
static REL::Relocation<tNiQuaternion_SomeRotationManipulation> NiQuaternion_SomeRotationManipulation{ RELOCATION_ID(69466, 70843) };

typedef bool*(__fastcall* StartVoiceShoutCast_t)(
    RE::Character* a_caster,
    RE::TESShout* a_shout,
    std::uint32_t a_wordIndex,
    RE::Actor* a_target);
static REL::Relocation<StartVoiceShoutCast_t> StartVoiceShoutCast{ RELOCATION_ID(37850, 38804) };

typedef bool*(__fastcall* GetFlyingMountFastTravelStateFlag_t)();
static REL::Relocation<GetFlyingMountFastTravelStateFlag_t> GetFlyingMountFastTravelStateFlag{ RELOCATION_ID(39634, 40720) };

typedef bool*(__fastcall* GetFlyingMountPatrolQueuedStateFlag_t)();
static REL::Relocation<GetFlyingMountPatrolQueuedStateFlag_t> GetFlyingMountPatrolQueuedStateFlag{ RELOCATION_ID(39635, 40721) };

typedef bool*(__fastcall* StartCombat_t)(RE::Actor* a_this, RE::Actor* a_target, RE::CombatGroup* a_combatGroup);
static REL::Relocation<StartCombat_t> StartCombat{ RELOCATION_ID(37608, 38561) };
