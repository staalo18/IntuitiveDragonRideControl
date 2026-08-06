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

        bool RegisterForSingleUpdate(float a_seconds);

        float GetHorizontalDistance(RE::TESObjectREFR* a_from, RE::TESObjectREFR* a_to);

        std::vector<WorldspaceIniData> LoadWorldspaceIniData(const std::string& a_iniFilename);

        // Version-aware accessor for PlayerCharacter::queuedTargetLoc.
        // CommonLibSSE-NG hardcodes queuedTargetLoc at the SE offset (0x640), which
        // is wrong in AE where the field sits 8 bytes later at 0x648. Using the
        // struct accessor directly compiles to the wrong address in AE for every
        // field. Use this helper instead of player->queuedTargetLoc everywhere.
        //   SE: queuedTargetLoc at 0x640
        //   AE: queuedTargetLoc at 0x648
        inline RE::PLAYER_TARGET_LOC* GetQueuedTargetLoc(RE::PlayerCharacter* a_player) {
            static const std::uintptr_t kOffset = REL::Module::IsAE() ? 0x648ULL : 0x640ULL;
            return reinterpret_cast<RE::PLAYER_TARGET_LOC*>(reinterpret_cast<std::uintptr_t>(a_player) + kOffset);
        }
    } // namespace Utils
} // namespace IDRC
