#include "ModAPI.h"
#include "DataManager.h"
#include "_ts_SKSEFunctions.h"

Messaging::IDRCInterface::IDRCInterface() noexcept {
	apiTID = GetCurrentThreadId();
}

Messaging::IDRCInterface::~IDRCInterface() noexcept {}

unsigned long Messaging::IDRCInterface::GetIDRCThreadId() const noexcept {
	return apiTID;
}

RE::ActorHandle Messaging::IDRCInterface::GetCurrentTarget() const noexcept  {
    auto* dragonActor = IDRC::DataManager::GetSingleton().GetDragonActor();
    if (dragonActor && dragonActor->GetActorRuntimeData().currentCombatTarget) {
        return dragonActor->GetActorRuntimeData().currentCombatTarget;
 	}

	return RE::ActorHandle();
}

RE::ActorHandle Messaging::IDRCInterface::GetDragon() const noexcept  {
    auto* dragonActor = IDRC::DataManager::GetSingleton().GetDragonActor();
    if (dragonActor) {
        return dragonActor->GetHandle();
 	}

	return RE::ActorHandle();
}

