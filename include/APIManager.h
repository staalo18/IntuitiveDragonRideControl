#pragma once

#include "API/TrueDirectionalMovementAPI.h"

struct APIs
{
	static inline TDM_API::IVTDM1* TrueDirectionalMovement = nullptr;

	static void RequestAPIs();
};
