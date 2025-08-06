#include "CombatTargetTracer.h"
#include "DataManager.h"
#include "_ts_SKSEFunctions.h"

namespace IDRC {

    RE::BSEventNotifyControl CombatTargetTracer::ProcessEvent(const RE::TESCombatEvent*  a_event, RE::BSTEventSource<RE::TESCombatEvent>*) {
        if (!a_event ) {
            return RE::BSEventNotifyControl::kContinue;
        }

        RE::Actor* dragonActor = DataManager::GetSingleton().GetDragonActor();

        if (!dragonActor) {
            log::warn("IDRC - {}: Dragon actor is nullptr", __func__);
            return RE::BSEventNotifyControl::kContinue;
        }

        int combatState = _ts_SKSEFunctions::GetCombatState(dragonActor);
        RE::Actor* currentCombatTarget = dragonActor->GetActorRuntimeData().currentCombatTarget.get().get();

        if (currentCombatTarget != m_combatTarget) {
            if (m_combatTarget) {
                log::info("IDRC - {}: Previous CombatTarget: {} ({} - {})", __func__, m_combatTarget->GetName(),  m_combatTarget->GetFormID(),  m_combatTarget->GetHandle().native_handle());
            } else {
                log::info("IDRC - {}: No previous CombatTarget", __func__);
            }
            m_combatTarget = currentCombatTarget;
        }
 
        if (combatState != m_combatState) {
            m_combatState = combatState;
            log::info("IDRC - {}: Combat state changed to: {}", __func__, m_combatState);
        }

        if (m_combatTarget) {
            log::info("IDRC - {}: Current CombatTarget: {} ({})", __func__, m_combatTarget->GetName(),  m_combatTarget->GetFormID());
        }
 
        return RE::BSEventNotifyControl::kContinue;
    }

    bool CombatTargetTracer::Register() {
        if (!m_isRegistered) {

            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESCombatEvent>(this);
            m_isRegistered = true;

            RE::Actor* dragonActor = DataManager::GetSingleton().GetDragonActor();
            if (!dragonActor) {
                log::warn("IDRC - {}: Dragon actor is nullptr", __func__);
                m_combatState = 0;
                m_combatTarget = nullptr;
                return true;
            }
            if (!m_breathHitShader) {
                log::warn("IDRC - {}: m_breathHitShader is nullptr", __func__);
                m_combatState = 0;
                m_combatTarget = nullptr;
                return true;
            }

            m_combatState = _ts_SKSEFunctions::GetCombatState(dragonActor);
            m_combatTarget = dragonActor->GetActorRuntimeData().currentCombatTarget.get().get();

            if (m_combatState > 0 && m_combatTarget) {
                SKSE::GetTaskInterface()->AddTask([this]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    this->m_combatTarget->ApplyEffectShader(this->m_breathHitShader);
                });
                log::info("IDRC - {}: Applied shader to target {} ({})", __func__, m_combatTarget->GetName(),  m_combatTarget->GetFormID());
            }
            log::info("IDRC - {}: registered CombatTargetTracer", __func__);
        } else {
            log::warn("IDRC - {}: CombatTargetTracer already registered", __func__);
        }
        return true;
    }

    bool CombatTargetTracer::Unregister() {
        if (m_isRegistered) {
            RE::ScriptEventSourceHolder::GetSingleton()->RemoveEventSink<RE::TESCombatEvent>(this);
            
            if (m_combatTarget) {
                SKSE::GetTaskInterface()->AddTask([this]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    this->m_combatTarget->ApplyEffectShader(nullptr);
                });
                log::info("IDRC - {}: Removed shader from target {} ({})", __func__, m_combatTarget->GetName(),  m_combatTarget->GetFormID());
                m_combatTarget = nullptr;
            }

            m_combatState = 0;

            m_isRegistered = false;
            log::info("IDRC - {}: Unregistered CombatTargetTracer", __func__);

        } else {
            log::warn("IDRC - {}: CombatTargetTracer was not registered", __func__);
        }
        return true;
    }

    void CombatTargetTracer::InitializeData(RE::TESEffectShader* a_breathHitShader) {
        log::info("IDRC - {}", __func__);
        m_breathHitShader = a_breathHitShader;
    }

    RE::Actor* CombatTargetTracer::GetCombatTarget() const {
        return m_combatTarget;
    }
} // namespace IDRC
