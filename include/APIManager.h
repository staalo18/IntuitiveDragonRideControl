#pragma once

#include "API/TrueDirectionalMovementAPI.h"
#include "API/TrueHUDAPI.h"

struct APIs
{
	static inline TDM_API::IVTDM1* TrueDirectionalMovement = nullptr;
    static inline TRUEHUD_API::IVTrueHUD3* TrueHUD = nullptr;

	static void RequestAPIs();
};
