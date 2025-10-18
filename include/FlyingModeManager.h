#pragma once

#include "ControlsManager.h"
#include "IDRCUtils.h"

namespace IDRC {
    class CombatManager;
    class FastTravelManager;

    enum FlyingMode {
        kFlying = 0,
        kOrbiting = 1,
        kHovering = 2,
        kLanded = 3,
        kPerching = 4
    };
    
    class FlyingModeManager {
    public:
        static FlyingModeManager& GetSingleton() {
            static FlyingModeManager instance;
            return instance;
        }

        FlyingModeManager(const FlyingModeManager&) = delete;
        FlyingModeManager& operator=(const FlyingModeManager&) = delete;

        void InitializeData(RE::TESObjectREFR* a_dragonTurnMarker, 
                            RE::TESObjectREFR* a_dragonTravelToMarker,
                            RE::TESObjectREFR* a_flyToTargetMarker,
                            RE::SpellItem* a_noFlyAbility);

        float GetMinHeight();

        RE::TESObjectREFR* GetDragonTurnMarker();

        void OnKeyDown(IDRCKey a_key);

        void ToggleAutoCombat();

        bool DragonTravelTo(RE::TESObjectREFR* a_directionMarker);

        bool TriggerLand(RE::TESObjectREFR* a_landTarget = nullptr);

        bool DragonHoverPlayerRiding(RE::TESObjectREFR* a_hoverTarget, bool a_displayMode = true);

        bool ForceHover();

        bool DragonTakeOffPlayerRiding(RE::TESObjectREFR* a_takeOffTarget, bool a_displayMode = true);

        bool DragonLandPlayerRiding(RE::TESObjectREFR* a_landTarget, bool a_displayMode = true);

        bool DragonFlyTo(float a_angle, bool a_displayMode = false);

        bool GetContinueFlyTo();

        float GetFlyToAngle();

        FlyingMode GetFlyingMode();

        void SetContinueFlyTo(bool a_continue);

        void SetFlyingModeFromPapyrus(int a_flyingState);

        bool GetRegisteredForLanding();

        bool GetRegisteredForPerch();

        void SetRegisteredForPerch(bool a_registeredForPerch);

        void ResetDragonHeight();

        bool DragonTurnPlayerRiding(float a_turnAngle);
    private:
        FlyingModeManager() = default;
        ~FlyingModeManager() = default;

        RE::TESObjectREFR* m_dragonTurnMarker = nullptr;
        RE::TESObjectREFR* m_dragonTravelToMarker = nullptr;
        RE::TESObjectREFR* m_flyToTargetMarker = nullptr;

        float m_minHeight = 1000.0f;  
        float m_maxHeight = 1000.0f;
        float m_arrivalHeight = 1000.0f;
        bool m_continueFlyTo = false;
        float m_flyToAngle = 0.0;
        float m_turnSpeed = 40.0;
        bool m_toggleAlwaysRun = true;
        bool m_registeredForLanding = false;
        bool m_registeredForPerch = false;
        bool m_toggledAutoCombatLand = false;
        bool m_skipOrbiting = true; // Always skip orbiting
        bool m_vanillaAttack = false;

        FlyingMode m_mode = kLanded;
        RE::SpellItem* m_noFlyAbility = nullptr;

        class WorldSpaceData {
        public:
            float m_minX;
            float m_maxX;
            float m_minY;
            float m_maxY;
            float m_centerX;
            float m_centerY;
            float m_seaLevel;
            std::string m_borderRegionName;

            WorldSpaceData() = default;
            WorldSpaceData(const RE::TESWorldSpace* a_worldSpace);

            void InitializeData(const std::string& a_regionName);
        };

        void SetMinHeight(float a_minHeight);

        float GetRunFactor(float a_modifier = 1.0f);

        bool FlyingModeUp(IDRCKey a_key);

        bool FlyingModeDown();

        void UpdateFlyingMode();

        bool CancelDragonLandPlayerRiding();

        void SetRegisteredForLanding(bool a_registeredForLanding);

        void PlaceTravelToMarker(RE::TESObjectREFR* a_ref, float a_distance = 0.0f, float a_angle = 0.0f, float a_offsetZ = 0.0f);

        void SetDragonHeight(int a_upDown);

        bool DragonNewDirection(float a_angle, bool a_displayMode = false);

        bool DragonOrbitPlayerRiding(RE::TESObjectREFR* a_orbitTarget);

        bool DragonPerchPlayerRiding();
        
        bool TriggerTurn();
        
        float NormalizeAngle(float a_angle);

        float GetTurnFactor();

        bool CheckForTurn() const;

        float GetAngleToCoordinate(float a_posX, float a_posY);

        float GetWorldSpaceCenterX();

        float GetWorldSpaceCenterY();

        float GetDistanceToRegionBoundingBox(const WorldSpaceData& a_worldspaceData, 
                                             float a_posX, float a_posY, float a_angle);

        void SetFlyingMode(FlyingMode a_mode);
    
    }; // class FlyingModeManager

} // namespace IDRC