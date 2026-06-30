#include "IDRCUtils.h"

#include <SimpleIni.h>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>

#include "_ts_SKSEFunctions.h"
#include "DataManager.h"

namespace IDRC {
    namespace Utils{

        void SetINIVars(){
            SKSE::GetTaskInterface()->AddTask([]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
                _ts_SKSEFunctions::UpdateIniSetting("fFlyingMountFastTravelDragonSpeed:General", 8500.0000f);
                _ts_SKSEFunctions::UpdateIniSetting("fFlyingMountFastTravelArrivalHeight:General", 100.0000f) ;
                _ts_SKSEFunctions::UpdateIniSetting("fFlyingMountLandingRequestTimer:General", 1.5000f);
                _ts_SKSEFunctions::UpdateIniSetting("fFlyingMountSlowestSpeedMult:General", 1.2500f);
                _ts_SKSEFunctions::UpdateIniSetting("iFlyingMountSlowestQueuedRefCount:General", 250);
                _ts_SKSEFunctions::UpdateIniSetting("fPlayerFlyingMountNothingLoadingMult:General", 10.0000f);
                _ts_SKSEFunctions::UpdateIniSetting("fPlayerFlyingMountFastBaseTargetSpeed:General", 1800.0000f) ;
                _ts_SKSEFunctions::UpdateIniSetting("fPlayerFlyingMountBaseTargetSpeed:General", 1100.0000f) ;
                _ts_SKSEFunctions::UpdateIniSetting("bFlyingMountFastTravelCruiseEnabled:General", true);
                _ts_SKSEFunctions::UpdateIniSetting("fPlayerFlyingMountTravelMaxHeight:General", 100.0000f) ;
                _ts_SKSEFunctions::UpdateIniSetting("fPlayerFlyingMountTravelMinHeight:General", 100.0000f) ; 
            });
        }


        bool RegisterForSingleUpdate(float a_seconds) {
            auto* quest = DataManager::GetSingleton().GetRideQuest();
            if (!quest) {
                log::error("IDRC - {}: RideQuest is null", __func__);
                return false;
            }
            auto handle = _ts_SKSEFunctions::GetHandle(quest);
            if(!handle){
                log::error("IDRC - {}: Quest handle is null", __func__);
                return false;
            }
            SKSE::GetTaskInterface()->AddTask([handle, a_seconds]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
                _ts_SKSEFunctions::RegisterForSingleUpdate(handle, a_seconds);
            });
            return true;
        }

// TODO: Try to implement a plugin function for SetAllowFlyingEx() to avoid this workaround
        bool SetAllowFlying(bool a_allowFlying) {
/*
            auto* dragonActor = DataManager::GetSingleton().GetDragonActor();
            if (!dragonActor) {
                log::error("IDRC - {}: dragonActor is null", __func__);
                return false;
            }
        SKSE::GetTaskInterface()->AddTask([dragonActor, a_allowFlying]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
            dragonActor->AsActorState()->actorState2.allowFlying = static_cast<uint32_t>(a_allowFlying);
            dragonActor->EvaluatePackage();
        });
//            return true;
 */           
            auto& dataManager = DataManager::GetSingleton();

            auto* RideQuest = dataManager.GetRideQuest();
            if (!RideQuest) {
                log::error("IDRC - {}: RideQuest is null", __func__);
                return false;
            }

            // Workaround: Send request to execute SetAllowFlying to Papyrus
            auto* args = RE::MakeFunctionArguments(bool(a_allowFlying));
            auto handle = _ts_SKSEFunctions::GetHandle(DataManager::GetSingleton().GetRideQuest());
            if(!handle){
                log::error("IDRC - {}: Quest handle is null", __func__);
                return false;
            }
            SKSE::GetTaskInterface()->AddTask([handle, args]() {
                // When modifying Game objects, send task to TaskInterface to ensure thread safety
                _ts_SKSEFunctions::SendCustomEvent(handle, "OnSetAllowFlying_SKSE", args);
            });

            // Now wait for the Papyrus SetAllowFlying() command to be completed
            int count = 0;
            while (count < 100 && dataManager.GetDragonActor()->AsActorState()->actorState2.allowFlying != static_cast<uint32_t>(a_allowFlying)) {
                _ts_SKSEFunctions::WaitWhileGameIsPaused();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                count++;
//log::info("IDRC - {}: Waiting for SetAllowFlying to complete... ({} ms)", __func__, count * 10);
            }
            if (count >= 100) { // waited > 1sec
                log::error("IDRC - {}: ERROR - Timed out while waiting for SetAllowFlying to complete!", __func__);
                return false;
            }
//        SKSE::GetTaskInterface()->AddTask([dragonActor, a_allowFlying]() {
            // When modifying Game objects, send task to TaskInterface to ensure thread safety
//            dragonActor->EvaluatePackage();
//        });

            return true;
        }
        

        /* loads worldspace data from ini file. ini file format:
                [Worldspace]
                name = Skyrim
                center_x = 0.0
                center_y = 0.0
                size = 300000.0
                name = Solstheim
                center_x = 10000.0
                center_y = 10000.0
                size = 200000.0
        */
        std::vector<WorldspaceIniData> LoadWorldspaceIniData(const std::string& a_iniFilename) {
            std::vector<WorldspaceIniData> worldspaces;
        
            std::filesystem::path iniPath = std::filesystem::current_path() / "Data" / a_iniFilename;
        
            CSimpleIniA ini;
            ini.SetMultiKey(true); // Enable support for duplicate sections
            if (ini.LoadFile(iniPath.string().c_str()) != SI_OK) {
                log::error("IDRC - {}: Failed to load INI file: {}", __func__, iniPath.string());
                return worldspaces;
            }
        
            CSimpleIniA::TNamesDepend sections;
            ini.GetAllSections(sections);
        
            for (const auto& section : sections) {
                if (std::string(section.pItem) == "Worldspace") {
                    CSimpleIniA::TNamesDepend worldspaceNames, worldspaceCentersX, worldspaceCentersY, worldspaceSizes;
                    ini.GetAllValues(section.pItem, "name", worldspaceNames);
                    ini.GetAllValues(section.pItem, "center_x", worldspaceCentersX);
                    ini.GetAllValues(section.pItem, "center_y", worldspaceCentersY);
                    ini.GetAllValues(section.pItem, "size", worldspaceSizes);
        
                    // Ensure all keys have the same number of values
                    if (worldspaceNames.size() != worldspaceCentersX.size() ||
                        worldspaceNames.size() != worldspaceCentersY.size() ||
                        worldspaceNames.size() != worldspaceSizes.size()) {
                        log::error("IDRC - {}: Error - Mismatched number of values in section '{}'", __func__, section.pItem);
                        continue;
                    }
        
                    auto nameIt = worldspaceNames.begin();
                    auto centerXIt = worldspaceCentersX.begin();
                    auto centerYIt = worldspaceCentersY.begin();
                    auto sizeIt = worldspaceSizes.begin();
        
                    while (nameIt != worldspaceNames.end()) {
                        WorldspaceIniData data;
                        data.name = nameIt->pItem;
        
                        try {
                            data.center_x = std::stof(centerXIt->pItem);
                            data.center_y = std::stof(centerYIt->pItem);
                            data.size = std::stof(sizeIt->pItem);
                        } catch (const std::exception& e) {
                            log::error("IDRC - {}: Error - Failed to parse values in section '{}': {}", __func__, section.pItem, e.what());
                            ++nameIt;
                            ++centerXIt;
                            ++centerYIt;
                            ++sizeIt;
                            continue;
                        }
        
                        worldspaces.push_back(data);
        
                        ++nameIt;
                        ++centerXIt;
                        ++centerYIt;
                        ++sizeIt;
                    }
                }
            }
        
            return worldspaces;
        }

        float GetHorizontalDistance(RE::TESObjectREFR* a_from, RE::TESObjectREFR* a_to) {
            if (!a_from || !a_to) {
                log::error("IDRC - {}: One or both of the provided TESObjectREFRs are null", __func__);
                return 0.0f;
            }

            auto fromPos = a_from->GetPosition();
            auto toPos = a_to->GetPosition();

            float deltaX = toPos.x - fromPos.x;
            float deltaY = toPos.y - fromPos.y;

            return std::sqrt(deltaX * deltaX + deltaY * deltaY);
        }
    } // namespace Utils
} // namespace IDRC
