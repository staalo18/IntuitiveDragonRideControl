#include "CrosshairTracer.h"

namespace IDRC {

    RE::BSEventNotifyControl CrosshairTracer::ProcessEvent(const SKSE::CrosshairRefEvent*  a_event, RE::BSTEventSource<SKSE::CrosshairRefEvent>*) {
        if (!a_event ) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto* crosshairEvent = a_event;
        if (crosshairEvent && crosshairEvent->crosshairRef) {
            m_crosshairTarget = crosshairEvent->crosshairRef.get();
log::info("IDRC - {}: CrosshairTarget = {}", __func__, m_crosshairTarget->GetBaseObject()->GetName());
        } else {
            m_crosshairTarget = nullptr;
log::info("IDRC - {}: CrosshairTarget = nullptr", __func__);
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    bool CrosshairTracer::Register() {
        if (!m_isRegistered) {
            auto* crosshairRefEventSource = SKSE::GetCrosshairRefEventSource();
            if (!crosshairRefEventSource) {
                log::error("IDRC - {}: crosshairRefEventSource is null", __func__);
                return false;
            }
            crosshairRefEventSource->AddEventSink(this);

            ForceDisplayCrosshair();
            
            m_isRegistered = true;
            log::info("IDRC - {}: Registered CrosshairTracer", __func__);
        } else {
            log::warn("IDRC - {}: CrosshairTracer already registered", __func__);
        }
        return true;
    }

    bool CrosshairTracer::Unregister() {
        if (m_isRegistered) {
            auto* crosshairRefEventSource = SKSE::GetCrosshairRefEventSource();
            if (!crosshairRefEventSource) {
                log::error("IDRC - {}: crosshairRefEventSource is null", __func__);
                return false;
            }
            crosshairRefEventSource->RemoveEventSink(this);
            
            m_isRegistered = false;
            log::info("IDRC - {}: Unregistered CrosshairTracer", __func__);
        } else {
            log::warn("IDRC - {}: CrosshairTracer was not registered", __func__);
        }
        return true;
    }

	void CrosshairTracer::InitializeData() {
        m_crosshairTarget = nullptr;
    }

    RE::TESObjectREFR* CrosshairTracer::GetCrosshairTarget() const {
        return m_crosshairTarget;
    }

    // experimental functionality below to enable crosshair while riding a dragon
    // essentially copied from "contextual crosshair" mod (author: doodlum)

    [[nodiscard]] RE::GFxValue GetGFxValue(const char* a_pathToVar)
    {
        RE::GFxValue object;

        auto ui = RE::UI::GetSingleton();
        auto hud = ui ? ui->GetMenu<RE::HUDMenu>() : nullptr;
        auto view = hud ? hud->uiMovie : nullptr;
        if (view)
            view->GetVariable(std::addressof(object), a_pathToVar);

        return object;
    }

    void CrosshairTracer::ForceDisplayCrosshair()
    // This is not working as expected yet...
    {
        log::info("IDRC - {}: Forcing display of crosshair", __func__);
        auto crosshairInstance = GetGFxValue("_root.HUDMovieBaseInstance.CrosshairInstance");
        if (crosshairInstance != nullptr) {
            log::info("IDRC - {}: CrosshairInstance found", __func__);
            RE::GFxValue::DisplayInfo displayInfo;
            crosshairInstance.GetDisplayInfo(std::addressof(displayInfo));
            displayInfo.SetAlpha(0.0);
            crosshairInstance.SetDisplayInfo(displayInfo);
        }
    }

} // namespace IDRC
