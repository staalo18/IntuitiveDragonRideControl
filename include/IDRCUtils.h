#pragma once

#include "Offsets.h"

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
    } // namespace Utils
} // namespace IDRC
