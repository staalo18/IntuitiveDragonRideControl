#include "CombatTargetReticle.h"
#include "Offsets.h"
#include "IDRCUtils.h"

// The CombatTargetReticle class is basically a copy of the TargetLockReticle implementation from 'True Directional Movement':
// https://github.com/ersh1/TrueDirectionalMovement
// All credits go to the original author Ersh!

namespace IDRC
{
	void CombatTargetReticle::Update(float a_deltaTime)
	{
		if (_interpTimer > 0) {
			_interpTimer -= a_deltaTime;

			if (_interpTimer < 0) {
				_interpTimer = 0;
			}

			_interpAlpha = 1.f - _interpTimer / _interpDuration;
		}

		if (_interpTimer == 0.f && _widgetState != WidgetState::kPendingRemoval) {
			_interpMode = InterpMode::kNone;
		}

		RE::GFxValue result;
		_object.Invoke("isReadyToRemove", &result);
		if (result.GetBool()) {
			_widgetState = WidgetState::kRemoved;
			return;
		}

		UpdatePosition();
		UpdateInfo();
	}

	void CombatTargetReticle::Initialize()
	{
		LoadConfig();

		SetWidgetState(WidgetState::kActive);

		RE::GRectF rect = _view->GetVisibleFrameRect();
		_lastScreenPos.x = rect.right * 0.5f;
		_lastScreenPos.y = rect.bottom * 0.5f;
	}

	void CombatTargetReticle::Dispose()
	{
		_object.Invoke("cleanUp", nullptr, nullptr, 0);
	}

    void CombatTargetReticle::SetVisible(bool a_visible) 
    {
        AddWidgetTask([=]() {
            RE::GFxValue::DisplayInfo displayInfo;
            _object.GetDisplayInfo(&displayInfo);
            displayInfo.SetVisible(a_visible);
            _object.SetDisplayInfo(displayInfo);
        });
    }

	void CombatTargetReticle::SetWidgetState(WidgetState a_widgetState)
	{
		_widgetState = a_widgetState;

		// if the reticle is 'transforming' from vanilla crosshair, do the position interpolation
		if (_reticleStyle == ReticleStyle::kSelectedActor) {
			if (_widgetState == WidgetState::kActive) {
				StartInterpolation(InterpMode::kCrosshairToTarget);
			} else if (_widgetState == WidgetState::kPendingRemoval) {
				StartInterpolation(InterpMode::kTargetToCrosshair);
			}
		}
	}

	void CombatTargetReticle::ChangeTarget(RE::ObjectRefHandle a_refHandle, RE::NiPointer<RE::NiAVObject> a_targetPoint)
	{

		AddWidgetTask([=]() {
			_refHandle = a_refHandle;
			_targetPoint = a_targetPoint;	

			if (_widgetState >= WidgetState::kPendingRemoval) {
				SetWidgetState(WidgetState::kActive);

				RE::GFxValue arg;
				arg.SetBoolean(false);
				_object.Invoke("setReadyToRemove", nullptr, &arg, 1);
			} else {
				StartInterpolation(InterpMode::kTargetToTarget);
				_object.Invoke("playChangeTargetTimeline", nullptr, nullptr, 0);
			}
		});
	}

	void CombatTargetReticle::UpdatePosition()
	{

		if (!_refHandle) {
			_widgetState = WidgetState::kRemoved;
			return;
		}

		RE::GRectF rect = _view->GetVisibleFrameRect();

		if (_interpMode == InterpMode::kTargetToCrosshair) {
			_desiredScreenPos.x = rect.right * 0.5f;
			_desiredScreenPos.y = rect.bottom * 0.5f;
		} else {
			RE::NiPoint3 targetWorldPos = _targetPoint ? _targetPoint->world.translate : _refHandle.get()->GetLookingAtLocation();

			RE::NiCamera::WorldPtToScreenPt3((float(*)[4])g_worldToCamMatrix, *g_viewPort, targetWorldPos, _desiredScreenPos.x, _desiredScreenPos.y, _depth, 1e-5f);

			_desiredScreenPos.y = 1.0f - _desiredScreenPos.y;  // Flip y for Flash coordinate system
			_desiredScreenPos.y = rect.top + (rect.bottom - rect.top) * _desiredScreenPos.y;
			_desiredScreenPos.x = rect.left + (rect.right - rect.left) * _desiredScreenPos.x;
		}

		RE::NiPoint2 screenPos;

		// if we're interpolating, lerp between the positions
		if (_interpTimer > 0) {
			screenPos.x = Utils::InterpEaseIn(_lastScreenPos.x, _desiredScreenPos.x, _interpAlpha, 2);
			screenPos.y = Utils::InterpEaseIn(_lastScreenPos.y, _desiredScreenPos.y, _interpAlpha, 2);
		} else {
			screenPos = _desiredScreenPos;
		}

		float scale = 100.f; // * Settings::fReticleScale;

		RE::GFxValue::DisplayInfo displayInfo;
		displayInfo.SetPosition(screenPos.x, screenPos.y);
		displayInfo.SetScale(scale, scale);
		_object.SetDisplayInfo(displayInfo);
	}

	void CombatTargetReticle::UpdateInfo()
	{
		if (!_refHandle || !_refHandle.get()) {
			_widgetState = WidgetState::kRemoved;
			return;
		}

		auto actor = _refHandle.get()->As<RE::Actor>();
		if (!actor) {
			_widgetState = WidgetState::kRemoved;
			return;
		}

		RE::GFxValue arg;
		arg.SetBoolean(_widgetState == kPendingRemoval ? true : false);

		_object.Invoke("updateData", nullptr, &arg, 1);
	}

	void CombatTargetReticle::LoadConfig()
	{
//log::info("IDRC - {}: Loading config - reticleStyle {}", __func__,  static_cast<uint32_t>(_reticleStyle));
		RE::GFxValue args[2];
// TODO: Use IDRC Reticle Style in the .swf file!
uint32_t reticleStyle = static_cast<uint32_t>(_reticleStyle);
//if (reticleStyle > 0) {
	//TODO - remove once swf file is adapted to IDRC
//	reticleStyle += 1; // convert to TDM reticle style for now (kCrosshair = 0, kCrosshairNoTransform = 1, kDot = 2, kGlow = 3)
//}
if (reticleStyle == 0) {
	reticleStyle = 3; // kSelectedActor -> kGlow
} else if (reticleStyle == 1) {
	reticleStyle = 1; // kCombatTargetFound -> kCrosshairNoTransform
} else if (reticleStyle == 2) {
	reticleStyle = 1; // kCombatTargetSearching -> kCrosshairNoTransform
} else { // fallback, should not get here
	reticleStyle = 2; // unknown -> kDot
}
		args[0].SetNumber(reticleStyle);
//		args[1].SetNumber((Settings::bReticleUseHUDOpacity ? *g_fHUDOpacity : Settings::fReticleOpacity) * 100.f);
		args[1].SetNumber(100.f);
		_object.Invoke("loadConfig", nullptr, args, 2);
	}

	void CombatTargetReticle::StartInterpolation(InterpMode a_interpMode)
	{
		RE::GFxValue::DisplayInfo displayInfo;
		_object.GetDisplayInfo(&displayInfo);
		_lastScreenPos.x = static_cast<float>(displayInfo.GetX());
		_lastScreenPos.y = static_cast<float>(displayInfo.GetY());

		if (_interpMode == InterpMode::kCrosshairToTarget && a_interpMode == InterpMode::kTargetToCrosshair ||
			_interpMode == InterpMode::kTargetToCrosshair && a_interpMode == InterpMode::kCrosshairToTarget) {
			_interpDuration = _fullInterpDuration * _interpAlpha;
		} else {
			_interpDuration = _fullInterpDuration;
		}

		_interpTimer = _interpDuration;
		_interpMode = a_interpMode;
	}

	void CombatTargetReticle::UpdateReticleStyle(ReticleStyle a_reticleStyle)
	{
		if (_reticleStyle != a_reticleStyle) {
			_reticleStyle = a_reticleStyle;
			LoadConfig();
			AddWidgetTask([this]() {
				_object.Invoke("init", nullptr, nullptr, 0);
			});
		}
	}
}
