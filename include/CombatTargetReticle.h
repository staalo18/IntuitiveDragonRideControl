#pragma once
#include "API/TrueHUDAPI.h"

// The CombatTargetReticle class is basically a copy of the TargetLockReticle implementation from 'True Directional Movement':
// https://github.com/ersh1/TrueDirectionalMovement
// All credits go to the original author Ersh!

namespace IDRC
{
	class CombatTargetReticle : public TRUEHUD_API::WidgetBase
	{
	public:
		enum WidgetStateMode : std::uint8_t
		{
			kAdd = 0,
			kRemove = 3,
		};

		enum InterpMode : std::uint8_t
		{
			kNone,
			kCrosshairToTarget,
			kTargetToTarget,
			kTargetToCrosshair
		};

		enum class ReticleStyle : std::uint32_t
		{
			kSelectedActor = 0,
			kCombatTargetFound = 1,
			kCombatTargetSearching = 2
		};

		CombatTargetReticle(uint32_t a_widgetID, RE::ObjectRefHandle a_refHandle, RE::NiPointer<RE::NiAVObject> a_targetPoint) :
			WidgetBase(a_widgetID),
			_refHandle(a_refHandle),
			_targetPoint(a_targetPoint)
		{}
		CombatTargetReticle(uint32_t a_widgetID, RE::ObjectRefHandle a_refHandle, RE::NiPointer<RE::NiAVObject> a_targetPoint, ReticleStyle a_reticleStyle) :
			WidgetBase(a_widgetID),
			_refHandle(a_refHandle),
			_targetPoint(a_targetPoint),
			_reticleStyle(a_reticleStyle)
		{}

		virtual void Update(float a_deltaTime) override;
		virtual void Initialize() override;
		virtual void Dispose() override;
		void SetVisible(bool a_visible);
		void UpdateReticleStyle(ReticleStyle a_reticleStyle);

		virtual void SetWidgetState(WidgetState a_widgetState);
		virtual void ChangeTarget(RE::ObjectRefHandle a_refHandle, RE::NiPointer<RE::NiAVObject> a_targetPoint);

		RE::ObjectRefHandle _refHandle;
		RE::NiPointer<RE::NiAVObject> _targetPoint;

	protected:
		virtual void UpdatePosition();
		virtual void UpdateInfo();
		virtual void LoadConfig();
		virtual void StartInterpolation(InterpMode a_interpMode);

	private:
		float _interpTimer = 0.f;
		float _interpDuration = 0.f;
		float _interpAlpha = 0.f;

		RE::NiPoint2 _lastScreenPos;
		RE::NiPoint2 _desiredScreenPos;

		InterpMode _interpMode = kNone;

		static constexpr float _fullInterpDuration = 0.5f;
		ReticleStyle _reticleStyle = ReticleStyle::kSelectedActor;
	};
}
