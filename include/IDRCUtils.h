#pragma once

#include "Offsets.h"
#define PI 3.1415926535f
namespace IDRC {
    namespace Utils{
        
        struct WorldspaceIniData {
            std::string name;
            float center_x;
            float center_y;
            float size;
        };
        
        void SetINIVars();

        bool ForceAliasTo(RE::BGSRefAlias* a_alias, RE::TESObjectREFR* a_reference);

        bool RegisterForSingleUpdate(float a_seconds);

        bool SetAllowFlying(bool a_allowFlying);

        std::vector<WorldspaceIniData> LoadWorldspaceIniData(const std::string& a_iniFilename);

        float GetCameraYaw();

        float GetCameraPitch();

        float GetAngleZ(const RE::NiPoint3& a_from, const RE::NiPoint3& a_to);

        // Below functions are from 'True Directional Movement':
        // https://github.com/ersh1/TrueDirectionalMovement
        // All credits go to the original author Ersh!

        RE::NiPoint3 GetCameraPos();

//        float NormalAbsoluteAngle(float a_angle);

        float NormalRelativeAngle(float a_angle);
        
        [[nodiscard]] inline float InterpEaseIn(const float& A, const float& B, float alpha, float exp)
        {
            float const modifiedAlpha = std::pow(alpha, exp);
            return std::lerp(A, B, modifiedAlpha);
        }

        [[nodiscard]] inline float GetRealTimeDeltaTime()
        {
            return *g_deltaTimeRealTime;
        }
    } // namespace Utils
} // namespace IDRC
