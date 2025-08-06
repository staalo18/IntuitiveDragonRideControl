#pragma once

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

        // InterpEaseIn() is a function from 'True Directional Movement':
        // https://github.com/ersh1/TrueDirectionalMovement
        // All credits go to the original author Ersh!
        [[nodiscard]] inline float InterpEaseIn(const float& A, const float& B, float alpha, float exp)
        {
            float const modifiedAlpha = std::pow(alpha, exp);
            return std::lerp(A, B, modifiedAlpha);
        }
    } // namespace Utils
} // namespace IDRC
