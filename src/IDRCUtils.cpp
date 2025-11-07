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

        
        // TODO: Using workaround (send command to papyrus) instead of native C++ function call
        // This is a time-intensive way to solve it because ForceAliasTo waits for the Papyrus function to complete

        // The C++ solution does not work as expected (FILL_TYPE is kForced, but forcedRef is nullptr??):
//                CombatTargetAlias->fillData.forced.forcedRef = actor->As<RE::TESObjectREFR>()->GetHandle();
//             This also does not work:
//                CombatTargetAlias->InitItem(actor->As<RE::TESForm>());

        // workaround for missing native (C++) ForceRefTo: 
        // use custom event in _ts_DR_DragonRideControlScript to pass actor back to Papyrus 
        // and call alias.ForceRefTo(actor) there:
        bool ForceAliasTo(RE::BGSRefAlias* a_alias, RE::TESObjectREFR* a_reference) {
            if (!a_alias) {
                log::error("IDRC - {}: alias is null", __func__);
                return false;
            }
    
            auto* quest = DataManager::GetSingleton().GetRideQuest();
            if (!quest) {
                log::error("IDRC - {}: quest is null", __func__);
                return false;
            }
            auto handle = _ts_SKSEFunctions::GetHandle(quest);
            if(!handle){
                log::error("IDRC - {}: Quest handle is null", __func__);
                return false;
            }
    
            if (a_reference) {
                log::info("IDRC - {}: ForceRefTo {}", __func__, a_reference->GetFormID());
                auto* args = RE::MakeFunctionArguments((RE::BGSRefAlias*)a_alias, (RE::TESObjectREFR*)a_reference);
                SKSE::GetTaskInterface()->AddTask([handle, args]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    _ts_SKSEFunctions::SendCustomEvent(handle, "OnForceRefTo_SKSE", args);
                });
            } else {
                log::info("IDRC - {}: ClearingAlias", __func__);
                auto* args = RE::MakeFunctionArguments((RE::BGSRefAlias*)a_alias);
                SKSE::GetTaskInterface()->AddTask([handle, args]() {
                    // When modifying Game objects, send task to TaskInterface to ensure thread safety
                    _ts_SKSEFunctions::SendCustomEvent(handle, "OnClearAlias_SKSE", args);
                });
            }
      
            // With this workaround, the update of the alias is happening asychronously,
            //  because the SendForceRefToEvent just triggers the Papyrus ForceRefTo(), 
            //  but does not wait for it to complete.
            //  So we need to wait for alias to be updated:
            int count = 0;
            while (count < 100 && a_alias->GetReference() != a_reference) 
            {
                _ts_SKSEFunctions::WaitWhileGameIsPaused();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                count++;
            }
            if (count >= 100) { // waited > 1sec
                log::error("IDRC - {}: ERROR - Timed out while waiting for alias to update!", __func__);
                return false;
            }
            return true;
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
            }
            if (count >= 100) { // waited > 1sec
                log::error("IDRC - {}: ERROR - Timed out while waiting for SetAllowFlying to complete!", __func__);
                return false;
            }
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
        
        float GetAngleZ(const RE::NiPoint3& a_from, const RE::NiPoint3& a_to)
        {
            const auto x = a_to.x - a_from.x;
            const auto y = a_to.y - a_from.y;

            return atan2(x, y);
        }

        RE::NiPoint3 GetCameraPos()
        {
            auto player = RE::PlayerCharacter::GetSingleton();
            auto playerCamera = RE::PlayerCamera::GetSingleton();
            RE::NiPoint3 ret;

            if (playerCamera->currentState->id == RE::CameraState::kFirstPerson ||
                playerCamera->currentState->id == RE::CameraState::kThirdPerson ||
                playerCamera->currentState->id == RE::CameraState::kMount ||
                playerCamera->currentState->id == RE::CameraState::kDragon)
            {
                RE::NiNode* root = playerCamera->cameraRoot.get();
                if (root) {
                    ret.x = root->world.translate.x;
                    ret.y = root->world.translate.y;
                    ret.z = root->world.translate.z;
                }
            } else {
                RE::NiPoint3 playerPos = player->GetLookingAtLocation();

                ret.z = playerPos.z;
                ret.x = player->GetPositionX();
                ret.y = player->GetPositionY();
            }

            return ret;
        }

        float GetCameraYaw() {
            auto playerCamera = RE::PlayerCamera::GetSingleton();
            if (playerCamera && playerCamera->cameraRoot) {
                RE::NiNode* root = playerCamera->cameraRoot.get();
                if (root) {
                    auto forwardVector = root->world.rotate.GetVectorZ();
                    float cameraYaw = NormalRelativeAngle(std::atan2(forwardVector.x, forwardVector.y));

                    float cameraPlayerDirection = GetAngleZ(GetCameraPos(), RE::PlayerCharacter::GetSingleton()->GetPosition());

                    if (fabs(NormalRelativeAngle(cameraPlayerDirection - cameraYaw)) > 0.9*PI) {
                        // in case the camera representation is flipped compared to Skyrim convention, align it with player direction
                        // look up "quaternion double cover" to learn more about this
                        cameraYaw = NormalRelativeAngle(cameraYaw + PI);
                    }

                    return cameraYaw;
                }
            }
            return 0.0f;
        }

        float GetCameraPitch() {
            auto playerCamera = RE::PlayerCamera::GetSingleton();
            if (playerCamera && playerCamera->cameraRoot) {
                RE::NiNode* root = playerCamera->cameraRoot.get();
                if (root) {
                    auto forwardVector = root->world.rotate.GetVectorZ();
                    auto upVector = root->world.rotate.GetVectorY();

                    float cameraPitch = NormalRelativeAngle( 0.5f*PI - std::atan2(forwardVector.z, 
                        std::sqrt(forwardVector.x * forwardVector.x + forwardVector.y * forwardVector.y)));

                    if (upVector.z < 0) {
                        // Camera is upside down due to quaternion flip
                        cameraPitch = -cameraPitch;
                    }

                    return cameraPitch;
                }
            }
            return 0.0f;
        }
/*
        float NormalAbsoluteAngle(float a_angle)
        {
            while (a_angle < 0)
                a_angle += 2.f*PI;
            while (a_angle > 2.f*PI)
                a_angle -= 2.f*PI;
            return a_angle;
        }
*/
        float NormalRelativeAngle(float a_angle)
        {
            while (a_angle > PI)
                a_angle -= 2.f*PI;
            while (a_angle < -PI)
                a_angle += 2.f*PI;
            return a_angle;
        }
    } // namespace Utils
} // namespace IDRC
