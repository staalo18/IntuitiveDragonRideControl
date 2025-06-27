#include "MagicEffectTracer.h"
#include "DataManager.h"
#include "_ts_SKSEFunctions.h"

namespace IDRC {

    RE::BSEventNotifyControl MagicEffectTracer::ProcessEvent(const RE::TESMagicEffectApplyEvent*  a_event, RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) {
        if (!a_event ) {
            return RE::BSEventNotifyControl::kContinue;
        }
        if (!m_spell) {
            log::info("IDRC - {}: no spell defined for tracing...", __func__);
            return RE::BSEventNotifyControl::kContinue;
        }

        for (const auto* effect : m_spell->effects) {
            if (effect && effect->baseEffect && effect->baseEffect->GetFormID() == a_event->magicEffect) {

                if (a_event->target && a_event->target.get()) {
                        log::info("IDRC - {}: MagicEffect {} applied to {}", __func__, m_spell->GetName(), a_event->target.get()->GetName());
                }
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    bool MagicEffectTracer::Register() {
        if (!m_isRegistered) {

            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESMagicEffectApplyEvent>(this);
            m_isRegistered = true;
            log::info("IDRC - {}: Registered MagicEffectTracer", __func__);
        } else {
            log::warn("IDRC - {}: MagicEffectTracer already registered", __func__);
        }
        return true;
    }

    bool MagicEffectTracer::Unregister() {
        if (m_isRegistered) {
            RE::ScriptEventSourceHolder::GetSingleton()->RemoveEventSink<RE::TESMagicEffectApplyEvent>(this);
            
//            m_spell = nullptr;
            m_isRegistered = false;
            log::info("IDRC - {}: Unregistered MagicEffectTracer", __func__);
        } else {
            log::warn("IDRC - {}: MagicEffectTracer was not registered", __func__);
        }
        return true;
    }

    void MagicEffectTracer::InitializeData(RE::SpellItem* a_spell) {
        m_spell = a_spell;
        m_isRegistered = false;
    }
} // namespace IDRC
