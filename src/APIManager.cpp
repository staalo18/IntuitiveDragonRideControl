#include "APIManager.h"

void APIs::RequestAPIs()
{
	if (!TrueDirectionalMovement) {
		TrueDirectionalMovement = reinterpret_cast<TDM_API::IVTDM1*>(TDM_API::RequestPluginAPI(TDM_API::InterfaceVersion::V1));
		if (TrueDirectionalMovement) {
			log::info("Obtained TrueDirectionalMovement API - {0:x}", reinterpret_cast<uintptr_t>(TrueDirectionalMovement));
		} else {
			log::warn("Failed to obtain TrueDirectionalMovement API");
		}
	}
    if (!TrueHUD) {
		TrueHUD = reinterpret_cast<TRUEHUD_API::IVTrueHUD3*>(TRUEHUD_API::RequestPluginAPI(TRUEHUD_API::InterfaceVersion::V3));
		if (TrueHUD) {
			log::info("Obtained TrueHUD API - {0:x}", reinterpret_cast<uintptr_t>(TrueHUD));
		} else {
			log::warn("Failed to obtain TrueHUD API");
		}
	}	
}
