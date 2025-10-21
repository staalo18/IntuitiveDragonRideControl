#include <string>
#include <filesystem>
#include <SimpleIni.h>
#include <thread>
#include "SKSE/API.h"
#include "RE/N/NativeLatentFunction.h"
#include "SKSE/Interfaces.h"

#include "_ts_SKSEFunctions.h"
#include "FastTravelManager.h"
#include "FlyingModeManager.h"
#include "DataManager.h"
#include "DisplayManager.h"
#include "CombatManager.h"
#include "ControlsManager.h"
#include "IDRCUtils.h"
#include "APIManager.h"
#include "ModAPI.h"
#include "TargetReticleManager.h"
#include "CameraLockManager.h"
#include "Hooks.h"

namespace IDRC {
    namespace Interface {
        int GetIDRCPluginVersion(RE::StaticFunctionTag*) {
            return 5;
        }

        void SetINIVars_SKSE(RE::StaticFunctionTag*) {
            Utils::SetINIVars();
        }

        void InitializeData_SKSE(RE::StaticFunctionTag*, 
                                RE::TESQuest* a_rideQuest,
                                RE::TESQuest* a_findPerchQuest,
                                RE::BGSRefAlias* a_dragonAlias, 
                                RE::TESObjectREFR* a_orbitMarker, 
                                RE::BGSRefAlias* a_combatTargetAlias,
                                RE::BGSRefAlias* a_wordWallPerch,
                                RE::BGSRefAlias* a_towerPerch,
                                RE::BGSRefAlias* a_rockPerch,
                                RE::BGSRefAlias* a_perchTarget,
                                RE::BGSListForm* a_fastTravelPackageList, 
                                RE::BGSListForm* a_combatTargetPackageList, 
                                RE::BGSListForm* a_breathList, 
                                RE::BGSListForm* a_ballList,
                                RE::TESShout* a_unrelentingForceShout,
                                RE::TESShout* a_attackShout,
                                RE::TESObjectREFR* a_dragonTurnMarker,
                                RE::TESObjectREFR* a_dragonTravelToMarker,
                                RE::TESObjectREFR* a_flyToTargetMarker,
                                RE::SpellItem* a_noFlyAbility,
                                std::string a_dragonName,
                                bool cameraLockInitiallyEnabled) {
            log::info("IDRC - {}", __func__);
            DataManager::GetSingleton().InitializeData(a_rideQuest, a_orbitMarker, a_dragonAlias, a_dragonName, a_findPerchQuest, 
                                            a_wordWallPerch, a_towerPerch, a_rockPerch, a_perchTarget);
            DisplayManager::GetSingleton().InitializeData();
            FlyingModeManager::GetSingleton().InitializeData(a_dragonTurnMarker, 
                                            a_dragonTravelToMarker, a_flyToTargetMarker, a_noFlyAbility);
            FastTravelManager::GetSingleton().InitializeData(a_fastTravelPackageList);
            CombatManager::GetSingleton().InitializeData(a_combatTargetPackageList, 
                a_breathList, a_ballList, a_unrelentingForceShout, a_attackShout,
                a_combatTargetAlias);  
            ControlsManager::GetSingleton().InitializeData();
            TargetReticleManager::GetSingleton().Initialize();
            CameraLockManager::GetSingleton().SetInitiallyEnabled(cameraLockInitiallyEnabled);
            CameraLockManager::GetSingleton().ResetEnabled();

            if (!APIs::CheckTDMVersion()) {
                RE::DebugMessageBox("You are using an older version of True Directional Movement (TDM), which does not support all features of Intuitive Dragon Ride Control. Consider updating TDM.");
            }
        }

        void SetContinueFlyTo_SKSE(RE::StaticFunctionTag*, bool a_continue) {
            log::info("IDRC - {}: {}", __func__, a_continue);
            FlyingModeManager::GetSingleton().SetContinueFlyTo(a_continue);
        }  

        void SetDisplayFlyingMode_SKSE(RE::StaticFunctionTag*, bool a_display) {
            log::info("IDRC - {}: {}", __func__, a_display);
            DisplayManager::GetSingleton().SetDisplayFlyingMode(a_display);
        }

        void SetFlyingMode_SKSE(RE::StaticFunctionTag*, int a_flyingState) {
            log::info("IDRC - {}: {}", __func__, a_flyingState);
            FlyingModeManager::GetSingleton().SetFlyingModeFromPapyrus(a_flyingState);
        }

        void SetWaitforShout_SKSE(RE::StaticFunctionTag*, bool a_wait) {
            log::info("IDRC - {}: {}", __func__, a_wait);
            CombatManager::GetSingleton().SetWaitForShout(a_wait, true);
        }

        bool IsAttackOngoing_SKSE(RE::StaticFunctionTag*) {
            return CombatManager::GetSingleton().IsAttackOngoing();
        }

        void UpdateDisplay_SKSE(RE::StaticFunctionTag*) {
            DisplayManager::GetSingleton().UpdateDisplay();
        }

        void SetAttackDisabled_SKSE(RE::StaticFunctionTag*, bool a_disable) {
            log::info("IDRC - {}: {}", __func__, a_disable);
            CombatManager::GetSingleton().SetAttackDisabled(a_disable);
        }

        void SetKeyMapping_SKSE(RE::StaticFunctionTag*, const std::string a_key, int a_value) {
            log::info("IDRC - {}: {} - {}", __func__, a_key, a_value);
            DXScanCode scanCode(static_cast<uint32_t>(a_value));
            ControlsManager::GetSingleton().SetKeyMapping(a_key, scanCode);
        }

        void SetTriggerAttack_SKSE(RE::StaticFunctionTag*, bool a_trigger) {
            log::info("IDRC - {}: {}", __func__, a_trigger);
            CombatManager::GetSingleton().SetTriggerAttack(a_trigger);
        }

        bool RegisterForControls_SKSE(RE::StaticFunctionTag*, bool a_reRegisterOnLoad = false, bool a_registerFromGoTDragonCompanions = false) {
            log::info("IDRC - {}", __func__);
            return ControlsManager::GetSingleton().RegisterForControls(a_reRegisterOnLoad, a_registerFromGoTDragonCompanions);
        }

        bool UnregisterForControls_SKSE(RE::StaticFunctionTag*) {
            log::info("IDRC - {}", __func__);
            return ControlsManager::GetSingleton().UnregisterForControls();
        }

        void SetInitialAutoCombatMode_SKSE(RE::StaticFunctionTag*, bool a_auto) {
            log::info("IDRC - {}: {}", __func__, a_auto);
            ControlsManager::GetSingleton().SetInitialAutoCombatMode(a_auto);
        }

        void SetDragonSpeeds_SKSE(RE::StaticFunctionTag*, float a_speedMult) {
            log::info("IDRC - {}: {}", __func__, a_speedMult);
            DataManager::GetSingleton().SetDragonSpeeds(a_speedMult);
        }

        void SetStopCombat_SKSE(RE::StaticFunctionTag*, bool a_stop) {
            log::info("IDRC - {}: {}", __func__, a_stop);
            CombatManager::GetSingleton().SetStopCombat(a_stop, true);
        }

        void SetAutoCombat_SKSE(RE::StaticFunctionTag*, bool a_auto) {
            log::info("IDRC - {}: {}", __func__, a_auto);
            DataManager::GetSingleton().SetAutoCombat(a_auto);
        }

        void SetBreathShoutList_SKSE(RE::StaticFunctionTag*, RE::BGSListForm* a_breathShoutList) {
            log::info("IDRC - {}", __func__);
            CombatManager::GetSingleton().SetBreathShoutList(a_breathShoutList);
        }

        RE::BGSListForm* GetBreathShoutList_SKSE(RE::StaticFunctionTag*) {
            log::info("IDRC - {}", __func__);
            return CombatManager::GetSingleton().GetBreathShoutList();
        }

        void SetBallShoutList_SKSE(RE::StaticFunctionTag*, RE::BGSListForm* a_ballShoutList) {
            log::info("IDRC - {}", __func__);
            CombatManager::GetSingleton().SetBallShoutList(a_ballShoutList);
        }

        RE::BGSListForm* GetBallShoutList_SKSE(RE::StaticFunctionTag*) {
            log::info("IDRC - {}", __func__);
            return CombatManager::GetSingleton().GetBallShoutList();
        }

        bool GetInitialAutoCombatMode_SKSE(RE::StaticFunctionTag*) {
            log::info("IDRC - {}", __func__);
            return ControlsManager::GetSingleton().GetInitialAutoCombatMode();
        }

        bool IsFlyingMountPatrolQueued(RE::StaticFunctionTag*, RE::Actor* a_actor) {
            return _ts_SKSEFunctions::IsFlyingMountPatrolQueued(a_actor);
        }

        bool IsFlyingMountFastTravelling(RE::StaticFunctionTag*, RE::Actor* a_actor) {
            return _ts_SKSEFunctions::IsFlyingMountFastTravelling(a_actor);
        }

        bool ClearCombatTargets(RE::StaticFunctionTag*, RE::Actor* a_actor) {
            return _ts_SKSEFunctions::ClearCombatTargets(a_actor);
        }

        void SetDisplayMessages_SKSE(RE::StaticFunctionTag*, bool a_display) {
            log::info("IDRC - {}: {}", __func__, a_display);
            DisplayManager::GetSingleton().SetDisplayMessages(a_display);
        }

        void SetTargetReticleMode_SKSE(RE::StaticFunctionTag*, int a_mode) {
            log::info("IDRC - {}: {}", __func__, a_mode);
            TargetReticleManager::ReticleMode mode = TargetReticleManager::ReticleMode::kOff;
            if (a_mode == 1) {
                mode = TargetReticleManager::ReticleMode::kOn;
            } else if (a_mode == 2) {
                mode = TargetReticleManager::ReticleMode::kOnlyCombatTarget;
            }
            TargetReticleManager::GetSingleton().SetReticleMode(mode);
        }
        void SetReticleLockAnimationStyle_SKSE(RE::StaticFunctionTag*, int a_style) {
            log::info("IDRC - {}: {}", __func__, a_style);
            TargetReticleManager::GetSingleton().SetReticleLockAnimationStyle(a_style);
        }
        void SetTDMLock_SKSE(RE::StaticFunctionTag*, int a_lock) {
            log::info("IDRC - {}: {}", __func__, a_lock);
            bool useTarget = (a_lock != 0);
            TargetReticleManager::GetSingleton().SetUseTarget(useTarget);
        }
        void SetPrimaryTargetMode_SKSE(RE::StaticFunctionTag*, int a_primaryTargetMode) {
            log::info("IDRC - {}: {}", __func__, a_primaryTargetMode);
            TargetReticleManager::GetSingleton().SetPrimaryTargetMode(static_cast<TargetReticleManager::TargetMode>(a_primaryTargetMode+1));
        }
        void SetMaxTargetDistance_SKSE(RE::StaticFunctionTag*, float a_distance) {
            log::info("IDRC - {}: {}", __func__, a_distance);
            TargetReticleManager::GetSingleton().SetMaxTargetDistance(a_distance);
        }
        void SetDistanceMultiplierSmall_SKSE(RE::StaticFunctionTag*, float a_multiplier) {
            log::info("IDRC - {}: {}", __func__, a_multiplier);
            TargetReticleManager::GetSingleton().SetDistanceMultiplierSmall(a_multiplier);
        }
        void SetDistanceMultiplierLarge_SKSE(RE::StaticFunctionTag*, float a_multiplier) {  
            log::info("IDRC - {}: {}", __func__, a_multiplier);
            TargetReticleManager::GetSingleton().SetDistanceMultiplierLarge(a_multiplier);
        }
        void SetDistanceMultiplierExtraLarge_SKSE(RE::StaticFunctionTag*, float a_multiplier) {
            log::info("IDRC - {}: {}", __func__, a_multiplier);
            TargetReticleManager::GetSingleton().SetDistanceMultiplierExtraLarge(a_multiplier);
        }
        void SetMaxTargetScanAngle_SKSE(RE::StaticFunctionTag*, float a_angle) {
            log::info("IDRC - {}: {}", __func__, a_angle);
            TargetReticleManager::GetSingleton().SetMaxTargetScanAngle(a_angle);
        }

        void SetCameraLockInitiallyEnabled_SKSE(RE::StaticFunctionTag*, bool a_enabled) {
            log::info("IDRC - {}: {}", __func__, a_enabled);
            CameraLockManager::GetSingleton().SetInitiallyEnabled(a_enabled);
        }

        RE::BSScript::LatentStatus TriggerLand_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*, RE::TESObjectREFR* a_landTarget) {
            std::thread([a_vm, a_stackID, a_landTarget]() {
                bool result = FlyingModeManager::GetSingleton().TriggerLand(a_landTarget);
                a_vm->ReturnLatentResult(a_stackID, result);
            }).detach();
        
            return RE::BSScript::LatentStatus::kStarted;
        }

        RE::BSScript::LatentStatus ForceHover_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*) {
            std::thread([a_vm, a_stackID]() {
                bool result = FlyingModeManager::GetSingleton().ForceHover();
                a_vm->ReturnLatentResult(a_stackID, result);
            }).detach();
        
            return RE::BSScript::LatentStatus::kStarted;
        }

        RE::BSScript::LatentStatus DragonLandPlayerRiding_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*, RE::TESObjectREFR* a_landTarget, bool a_displayMode) {
            std::thread([a_vm, a_stackID, a_landTarget, a_displayMode]() {
                bool result = FlyingModeManager::GetSingleton().DragonLandPlayerRiding(a_landTarget, a_displayMode);
                a_vm->ReturnLatentResult(a_stackID, result);
            }).detach();
        
            return RE::BSScript::LatentStatus::kStarted;
        }

        RE::BSScript::LatentStatus StopAttack_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*, bool a_forceStop) {
            std::thread([a_vm, a_stackID, a_forceStop]() {
                bool result = CombatManager::GetSingleton().StopAttack(a_forceStop);
                a_vm->ReturnLatentResult(a_stackID, result);
            }).detach();

            return RE::BSScript::LatentStatus::kStarted;
        }

        RE::BSScript::LatentStatus StopFastTravel_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*, RE::TESObjectREFR* a_stopFastTravelTarget, float a_height, int a_timeout, std::string a_waitMessage, std::string a_timeoutMessage) {

            // Log threadId of papyrus thread:
//            std::ostringstream threadIdStream;
//            threadIdStream << std::this_thread::get_id();
//            std::string threadIdStr = threadIdStream.str();
//            log::info("IDRC - {} - threadId: {}", __func__, threadIdStr);

std::thread([a_vm, a_stackID, a_stopFastTravelTarget, a_height, a_timeout, a_waitMessage, a_timeoutMessage]() {
                bool result = FastTravelManager::GetSingleton().StopFastTravel(a_stopFastTravelTarget, a_height, a_timeout, a_waitMessage, a_timeoutMessage);
                a_vm->ReturnLatentResult(a_stackID, result);
            }).detach();
        
            return RE::BSScript::LatentStatus::kStarted;
        }

        RE::BSScript::LatentStatus CancelStopFastTravel_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*) {
            std::thread([a_vm, a_stackID]() {
                bool result = FastTravelManager::GetSingleton().CancelStopFastTravel();
                a_vm->ReturnLatentResult(a_stackID, result);
            }).detach();
        
            return RE::BSScript::LatentStatus::kStarted;
        }

        RE::BSScript::LatentStatus SyncCombatTarget_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*, bool a_forceSync) {
            std::thread([a_vm, a_stackID, a_forceSync]() {
                bool result = CombatManager::GetSingleton().SyncCombatTarget(a_forceSync);
                a_vm->ReturnLatentResult(a_stackID, result);
            }).detach();
        
            return RE::BSScript::LatentStatus::kStarted;
        }

        RE::BSScript::LatentStatus DragonAttack_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*, bool a_alternateAttack = false) {
            std::thread([a_vm, a_stackID, a_alternateAttack]() {
                bool result = CombatManager::GetSingleton().DragonAttack(a_alternateAttack);
                a_vm->ReturnLatentResult(a_stackID, result);
            }).detach();
        
            return RE::BSScript::LatentStatus::kStarted;
        }

        RE::BSScript::LatentStatus DragonTakeOffPlayerRiding_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*, RE::TESObjectREFR* a_takeOffTarget, bool a_displayMode = true) {
            std::thread([a_vm, a_stackID, a_takeOffTarget, a_displayMode]() {
                bool result = FlyingModeManager::GetSingleton().DragonTakeOffPlayerRiding(a_takeOffTarget, a_displayMode);
                a_vm->ReturnLatentResult(a_stackID, result);
            }).detach();
        
            return RE::BSScript::LatentStatus::kStarted;
        }

        RE::BSScript::LatentStatus DragonHoverPlayerRiding_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*, RE::TESObjectREFR* a_hoverTarget, bool a_displayMode = true) {
            std::thread([a_vm, a_stackID, a_hoverTarget, a_displayMode]() {
                bool result = FlyingModeManager::GetSingleton().DragonHoverPlayerRiding(a_hoverTarget, a_displayMode);
                a_vm->ReturnLatentResult(a_stackID, result);
            }).detach();
        
            return RE::BSScript::LatentStatus::kStarted;
        }
/*
        bool TEST_SKSE(RE::StaticFunctionTag *, RE::Actor* dragonActor, RE::BGSRefAlias* otherAlias,
             RE::TESObjectREFR* Orbitmarker, bool bTest){
            log::info("IDRC - {}", __func__);


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
            auto* skyrimVM = RE::SkyrimVM::GetSingleton();
            if (!skyrimVM) {
                return false;
            }
            auto VMGameTime = skyrimVM->currentVMGameTime;
            auto VMDaysPassed = skyrimVM->currentVMDaysPassed;
            auto VMMenuModeTime = skyrimVM->currentVMMenuModeTime;
            auto VMTime = skyrimVM->currentVMTime;

            log::info("IDRC - {}: VMGameTime: {}, VMDaysPassed: {}, VMMenuModeTime: {}, VMTime: {}", __func__, VMGameTime, VMDaysPassed, VMMenuModeTime, VMTime);
            std::string message = "Sending RegisterForSingleUpdate: VMGameTime: " + std::to_string(VMGameTime) + 
                ", VMDaysPassed: " + std::to_string(VMDaysPassed) + 
                ", VMMenuModeTime: " + std::to_string(VMMenuModeTime) + 
                ", VMTime: " + std::to_string(VMTime);
            RE::DebugNotification(message.c_str());
            _ts_SKSEFunctions::RegisterForSingleUpdate(handle, 0.5f);

            DataManager::GetSingleton().SetAutoCombat(true);

 /*
            auto* target = CombatManager::GetSingleton().GetCombatTarget();
            if (target) { 
                log::info("IDRC - {}: Before - target: {}", __func__, target->GetFormID());
            } else {
                log::info("IDRC - {}: Before - target is None", __func__);
            }   
            log::info("IDRC - {}: dragonActor: {}", __func__, dragonActor->GetFormID());
 
            CombatManager::GetSingleton().ForceCombatTargetAliasTo(nullptr);

            target = CombatManager::GetSingleton().GetCombatTarget();

            if (target) { 
                log::info("IDRC - {}: after - target: {}", __func__, target->GetFormID());
            } else {
                log::info("IDRC - {}: after - target is None", __func__);
            }   

*//*
//            CombatManager::GetSingleton().SetShoutMode(0);
            int iState = _ts_SKSEFunctions::GetFlyingState(dragonActor);

            log::info("IDRC - {}: FlyingState = {}", __func__, iState);
            int CombatState = _ts_SKSEFunctions::GetCombatState(dragonActor);
            log::info("IDRC - {}: CombatState = {}", __func__, CombatState);
//            _ts_SKSEFunctions::GetCombatMembers(dragonActor);

            if (target) { 
                log::info("IDRC - {}: target: {}", __func__, target->GetFormID());
            } else {
                log::info("IDRC - {}: target is None", __func__);
            }
//            _ts_SKSEFunctions::StartCombat(dragonActor, target);
//            CombatManager::GetSingleton().DragonStartCombat(target);

            int iCount = 0;
            while (true) {
                _ts_SKSEFunctions::WaitWhileGameIsPaused();

                log::info("IDRC - {}: Waiting...({})", __func__, iCount);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                iCount++;
            }
           bool isForwardPressed = ControlsManager::GetSingleton().GetIsKeyPressed(IDRCKey::kForward);
            log::info("IDRC - {}: isForwardPressed = {}", __func__, isForwardPressed);
            return true;
        }


        void TEST_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID,
            RE::Actor* dragonActor, RE::BGSRefAlias* otherAlias, RE::TESObjectREFR* Orbitmarker, bool bTest){
            bool result = TEST_SKSE(nullptr, dragonActor, otherAlias, Orbitmarker, bTest);
            a_vm->ReturnLatentResult(a_stackID, result);    
        }

        RE::BSScript::LatentStatus TEST_SKSE_SKSE_Latent(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, 
            RE::StaticFunctionTag *, RE::Actor* dragonActor, RE::BGSRefAlias* otherAlias, RE::TESObjectREFR* Orbitmarker,  bool bTest){
            std::thread t(TEST_SKSE_Latent, a_vm, a_stackID, dragonActor, otherAlias, Orbitmarker, bTest);
            t.detach();
            return RE::BSScript::LatentStatus::kStarted;  
        }
*/
        bool IDRCFunctions(RE::BSScript::Internal::VirtualMachine * a_vm){

            a_vm->RegisterFunction("GetIDRCPluginVersion", "_ts_DR_PlayerAliasScript", GetIDRCPluginVersion);
            a_vm->RegisterFunction("SetINIVars_SKSE", "_ts_DR_RideControlScript", SetINIVars_SKSE);
            a_vm->RegisterFunction("InitializeData_SKSE", "_ts_DR_RideControlScript", InitializeData_SKSE);
            a_vm->RegisterFunction("SetContinueFlyTo_SKSE", "_ts_DR_RideControlScript", SetContinueFlyTo_SKSE);
            a_vm->RegisterFunction("SetDisplayFlyingMode_SKSE", "_ts_DR_RideControlScript", SetDisplayFlyingMode_SKSE);
            a_vm->RegisterFunction("SetFlyingMode_SKSE", "_ts_DR_RideControlScript", SetFlyingMode_SKSE);
            a_vm->RegisterFunction("SetWaitforShout_SKSE", "_ts_DR_RideControlScript", SetWaitforShout_SKSE);
            a_vm->RegisterFunction("SetAttackDisabled_SKSE", "_ts_DR_RideControlScript", SetAttackDisabled_SKSE);
            a_vm->RegisterFunction("SetKeyMapping_SKSE", "_ts_DR_RideControlScript", SetKeyMapping_SKSE);
            a_vm->RegisterFunction("SetTriggerAttack_SKSE", "_ts_DR_RideControlScript", SetTriggerAttack_SKSE);
            a_vm->RegisterFunction("IsAttackOngoing_SKSE", "_ts_DR_RideControlScript", IsAttackOngoing_SKSE);
            a_vm->RegisterFunction("RegisterForControls_SKSE", "_ts_DR_RideControlScript", RegisterForControls_SKSE);
            a_vm->RegisterFunction("UnregisterForControls_SKSE", "_ts_DR_RideControlScript", UnregisterForControls_SKSE);
            a_vm->RegisterFunction("UpdateDisplay_SKSE", "_ts_DR_RideControlScript", UpdateDisplay_SKSE);
            a_vm->RegisterFunction("SetInitialAutoCombatMode_SKSE", "_ts_DR_RideControlScript", SetInitialAutoCombatMode_SKSE);
            a_vm->RegisterFunction("SetDragonSpeeds_SKSE", "_ts_DR_RideControlScript", SetDragonSpeeds_SKSE);
            a_vm->RegisterFunction("SetStopCombat_SKSE", "_ts_DR_RideControlScript", SetStopCombat_SKSE);
            a_vm->RegisterFunction("SetTargetReticleMode_SKSE", "_ts_DR_RideControlScript", SetTargetReticleMode_SKSE);
            a_vm->RegisterFunction("SetReticleLockAnimationStyle_SKSE", "_ts_DR_RideControlScript", SetReticleLockAnimationStyle_SKSE);
            a_vm->RegisterFunction("SetTDMLock_SKSE", "_ts_DR_RideControlScript", SetTDMLock_SKSE);
            a_vm->RegisterFunction("SetPrimaryTargetMode_SKSE", "_ts_DR_RideControlScript", SetPrimaryTargetMode_SKSE);
            a_vm->RegisterFunction("SetMaxTargetDistance_SKSE", "_ts_DR_RideControlScript", SetMaxTargetDistance_SKSE);
            a_vm->RegisterFunction("SetDistanceMultiplierSmall_SKSE", "_ts_DR_RideControlScript", SetDistanceMultiplierSmall_SKSE);
            a_vm->RegisterFunction("SetDistanceMultiplierLarge_SKSE", "_ts_DR_RideControlScript", SetDistanceMultiplierLarge_SKSE);
            a_vm->RegisterFunction("SetDistanceMultiplierExtraLarge_SKSE", "_ts_DR_RideControlScript", SetDistanceMultiplierExtraLarge_SKSE);
            a_vm->RegisterFunction("SetMaxTargetScanAngle_SKSE", "_ts_DR_RideControlScript", SetMaxTargetScanAngle_SKSE);
            a_vm->RegisterFunction("SetCameraLockInitiallyEnabled_SKSE", "_ts_DR_RideControlScript", SetCameraLockInitiallyEnabled_SKSE);

            // Papyrus access to ts_SKSEFunctions
            a_vm->RegisterFunction("IsFlyingMountPatrolQueued", "_ts_DR_RideControlScript", IsFlyingMountPatrolQueued);
            a_vm->RegisterFunction("IsFlyingMountFastTravelling", "_ts_DR_RideControlScript", IsFlyingMountFastTravelling);
            a_vm->RegisterFunction("ClearCombatTargets", "_ts_DR_RideControlScript", ClearCombatTargets);
        
            // functions for GoTDragonCompanions
            a_vm->RegisterFunction("SetAutoCombat_SKSE", "_ts_DR_RideControlScript", SetAutoCombat_SKSE);
            a_vm->RegisterFunction("SetBreathShoutList_SKSE", "_ts_DR_RideControlScript", SetBreathShoutList_SKSE);
            a_vm->RegisterFunction("SetBallShoutList_SKSE", "_ts_DR_RideControlScript", SetBallShoutList_SKSE);
            a_vm->RegisterFunction("GetBreathShoutList_SKSE", "_ts_DR_RideControlScript", GetBreathShoutList_SKSE);
            a_vm->RegisterFunction("GetBallShoutList_SKSE", "_ts_DR_RideControlScript", GetBallShoutList_SKSE);
            a_vm->RegisterFunction("GetInitialAutoCombatMode_SKSE", "_ts_DR_RideControlScript", GetInitialAutoCombatMode_SKSE);
            a_vm->RegisterFunction("SetDisplayMessages_SKSE", "_ts_DR_RideControlScript", SetDisplayMessages_SKSE);

//            a_vm->RegisterFunction("TEST_SKSE", "_ts_DR_RideControlScript", TEST_SKSE);

            // Latent native functions are used in situations where the function execution takes
            // a non-negligible amount of time.
            // The calling Papyrus function will wait for the result of the latent function, but 
            // the function does not block the main thread (ie Skyrim does not freeze), as long as the
            // function is executed on a separate thread.
            //
            /* NOTE: NativeLatentFunction.h needs to be patched from the downloaded version to get it compiled:
                    
                Patch in the NativeLatentFunction constructor:
                    from original: this->_retType = GetRawType<latentR>();
                    to patched:  this->_retType = GetRawType<latentR>()();

                Downloaded version is:
                    "repository": "https://gitlab.com/colorglass/vcpkg-colorglass",
                    "baseline": "6309841a1ce770409708a67a9ba5c26c537d2937",
                    "packages": ["commonlibsse-ng"]
            */
            a_vm->RegisterLatentFunction<bool>("TriggerLand_SKSE", "_ts_DR_RideControlScript", TriggerLand_SKSE_Latent);
            a_vm->RegisterLatentFunction<bool>("ForceHover_SKSE", "_ts_DR_RideControlScript", ForceHover_SKSE_Latent);
            a_vm->RegisterLatentFunction<bool>("DragonLandPlayerRiding_SKSE", "_ts_DR_RideControlScript", DragonLandPlayerRiding_SKSE_Latent);
            a_vm->RegisterLatentFunction<bool>("StopAttack_SKSE", "_ts_DR_RideControlScript", StopAttack_SKSE_Latent);
            a_vm->RegisterLatentFunction<bool>("CancelStopFastTravel_SKSE", "_ts_DR_RideControlScript", CancelStopFastTravel_SKSE_Latent);
            a_vm->RegisterLatentFunction<bool>("StopFastTravel_SKSE", "_ts_DR_RideControlScript", StopFastTravel_SKSE_Latent);
            a_vm->RegisterLatentFunction<bool>("SyncCombatTarget_SKSE", "_ts_DR_RideControlScript", SyncCombatTarget_SKSE_Latent); 
            // functions for GoTDragonCompanions
            a_vm->RegisterLatentFunction<bool>("DragonAttack_SKSE", "_ts_DR_RideControlScript", DragonAttack_SKSE_Latent);
            a_vm->RegisterLatentFunction<bool>("DragonTakeOffPlayerRiding_SKSE", "_ts_DR_RideControlScript", DragonTakeOffPlayerRiding_SKSE_Latent);
            a_vm->RegisterLatentFunction<bool>("DragonHoverPlayerRiding_SKSE", "_ts_DR_RideControlScript", DragonHoverPlayerRiding_SKSE_Latent);
            
            return true;
        }
    } // namespace Interface
} // namespace IDRC

/******************************************************************************************/
void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	// Try requesting APIs at multiple steps to try to work around the SKSE messaging bug
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		APIs::RequestAPIs();
		break;
	case SKSE::MessagingInterface::kPostLoad:
		APIs::RequestAPIs();
		break;
	case SKSE::MessagingInterface::kPostPostLoad:
		APIs::RequestAPIs();
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		break;
	case SKSE::MessagingInterface::kPostLoadGame:
	case SKSE::MessagingInterface::kNewGame:
		APIs::RequestAPIs();
		break;
	}
}

SKSEPluginInfo(
    .Version = Plugin::VERSION,
    .Name = Plugin::NAME,
    .Author = "Staalo",
    .RuntimeCompatibility = SKSE::PluginDeclaration::RuntimeCompatibility(SKSE::VersionIndependence::AddressLibrary),
    .MinimumSKSEVersion = { 2, 2, 3 } // or 0 if you want to support all
)

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* skse) {
    long logLevel = _ts_SKSEFunctions::GetValueFromINI(nullptr, 0, "LogLevel:Log", "SKSE/Plugins/IntuitiveDragonRideControl.ini", 3L);
    bool isLogLevelValid = true;
    if (logLevel < 0 || logLevel > 6) {
        logLevel = 2L; // info
        isLogLevelValid = false;
    }

	_ts_SKSEFunctions::InitializeLogging(static_cast<spdlog::level::level_enum>(logLevel));
    if (!isLogLevelValid) {
        log::warn("IDRC - {}: LogLevel in INI file is invalid. Defaulting to info level.", __func__);
    }
    log::info("IDRC - {}: IDRC Plugin version: {}", __func__, IDRC::Interface::GetIDRCPluginVersion(nullptr));

/* currently not used
    log::info("IDRC - {}: Loading worldspaces...", __func__);
    auto worldspaces = IDRC::Utils::LoadWorldspaceIniData("SKSE/Plugins/IntuitiveDragonRideControl.ini");

    for (const auto& ws : worldspaces) {
        log::info("IDRC - {}: Worldspace Name = {}, Center = ({}, {}), Size = {}", 
            __func__, ws.name, ws.center_x, ws.center_y, ws.size);
    }
*/
    Init(skse);
    
    auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		return false;
	}

    if (!SKSE::GetPapyrusInterface()->Register(IDRC::Interface::IDRCFunctions)) {
        log::error("IDRC - {}: Failed to register Papyrus functions.", __func__);
        return false;
    } else {
        log::info("IDRC - {}: Registered Papyrus functions", __func__);
    }

    log::info("IDRC - {}: Calling Install Hooks", __func__);

    SKSE::AllocTrampoline(64);

    Hooks::Install();

    return true;
}

extern "C" DLLEXPORT void* SKSEAPI RequestPluginAPI(const IDRC_API::InterfaceVersion a_interfaceVersion)
{
	auto api = Messaging::IDRCInterface::GetSingleton();

	log::info("IntuitiveDragonRideControl::RequestPluginAPI called, InterfaceVersion {}", static_cast<uint8_t>(a_interfaceVersion));

	switch (a_interfaceVersion) {
	case IDRC_API::InterfaceVersion::V1:
		log::info("IntuitiveDragonRideControl::RequestPluginAPI returned the API singleton");
		return static_cast<void*>(api);
	}

	log::info("IntuitiveDragonRideControl::RequestPluginAPI requested the wrong interface version");
	return nullptr;
}