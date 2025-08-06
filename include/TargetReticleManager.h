#pragma once

#include "CombatTargetReticle.h"


namespace IDRC {
    class TargetReticleManager {
        public:
            enum TargetMode : std::uint32_t {
                kNone = 0,
                kSelectedActor = 1,
                kCombatTarget = 2
            };

            static TargetReticleManager& GetSingleton() {
                static TargetReticleManager instance;
                return instance;
            }
            TargetReticleManager(const TargetReticleManager&) = delete;
            TargetReticleManager& operator=(const TargetReticleManager&) = delete;

            void Initialize();

            void SetPrimaryTargetMode(TargetMode a_targetMode);
            void SetMaxTargetDistance(float a_maxReticleDistance);
            void SetDistanceMultiplierSmall(float a_distanceMultiplierSmall);
            void SetDistanceMultiplierLarge(float a_distanceMultiplierLarge);
            void SetDistanceMultiplierExtraLarge(float a_distanceMultiplierExtraLarge);
            void SetMaxTargetScanAngle(float a_maxTargetScanAngle);

            void Update();

            void Enable(bool a_enable) { m_isEnabled = a_enable; }

            void ToggleLockReticle();

            void TogglePrimaryTargetMode();

            void DisposeReticle();

            RE::Actor* GetCurrentTarget() const;
    
        private:
            TargetReticleManager() = default;

            void UpdateReticle();

            bool IsTDMLocked();

//            void ShowReticle(bool a_show);

            RE::Actor* GetSelectedActor() const;

            RE::Actor* GetCombatTarget() const;

            TargetMode GetTargetMode(bool a_hasSelectedActor, bool a_hasCombatTarget) const;

            void SetReticleTarget(CombatTargetReticle::ReticleStyle a_reticleStyle);

            RE::NiPointer<RE::NiAVObject> GetTargetPoint(RE::Actor* a_actor) const;

            CombatTargetReticle::ReticleStyle GetReticleStyle(TargetMode a_targetMode) const;

            float GetDistanceRaceSizeMultiplier(RE::TESRace* a_race) const;

            std::weak_ptr<CombatTargetReticle> m_TargetReticle;
            bool m_isInitialized = false;
            bool m_isEnabled = true;
            bool m_isReticleLocked = false;
            bool m_isWidgetActive = false;
            int m_combatState = 0;
            float m_maxReticleDistance = 8000.f;
            float m_distanceMultiplierSmall = 1.0f;
            float m_distanceMultiplierLarge = 2.0f;
            float m_distanceMultiplierExtraLarge = 4.0f;
            float m_maxTargetScanAngle = 7.0f;
            RE::Actor* m_reticleTarget = nullptr;
            TargetMode m_primaryTargetMode = TargetMode::kCombatTarget;
            TargetMode m_currentTargetMode = TargetMode::kNone;
    }; // class TargetReticleManager
}  // namespace IDRC
