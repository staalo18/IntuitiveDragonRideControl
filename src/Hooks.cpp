#include "Hooks.h"
#include "TargetReticleManager.h"

namespace Hooks
{
	void Install()
	{
		log::info("Hooking...");

		MainUpdateHook::Hook();

		ReadyWeaponHook::Hook();
		ExtraInteractionHook::Hook();
//		GetMountHook::Hook();

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

		IDRC::TargetReticleManager::GetSingleton().Update();
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


} // namespace Hooks
