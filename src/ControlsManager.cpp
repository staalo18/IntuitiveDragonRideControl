#include "ControlsManager.h"
#include "FlyingModeManager.h"
#include "DataManager.h"
#include "_ts_SKSEFunctions.h"
#include "ThumbstickTracer.h"
#include "CombatTargetTracer.h"
//#include "CrosshairTracer.h"
//#include "MagicEffectTracer.h"


namespace IDRC {
    
    RE::BSEventNotifyControl ControlsManager::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) {

        if (!a_event || RE::UI::GetSingleton()->GameIsPaused()) {
            return RE::BSEventNotifyControl::kContinue;
        }

        for (auto* event = *a_event; event; event = event->next) {
            if (event->eventType == RE::INPUT_EVENT_TYPE::kButton) {
                auto* buttonEvent = static_cast<RE::ButtonEvent*>(event);
                if (!buttonEvent || !buttonEvent->IsDown()) {
                    continue;
                }

                IDRCKey idrcKey = GetMappedIDRCKey(buttonEvent);
                if (idrcKey == kInvalid) {
                    // a button which is not used in IDRC has been pressed
                    continue;
                }

                if (buttonEvent->device.get() == RE::INPUT_DEVICE::kGamepad) {
                    // this is needed to ensure that GetIsKeyPressed() recognizes the buttonpress immediately
                    TouchGamepadButton(buttonEvent);
                } else if (buttonEvent->device.get() == RE::INPUT_DEVICE::kMouse) {
                    // this is needed to ensure that GetIsKeyPressed() recognizes the buttonpress immediately
                    TouchMouseButton(buttonEvent);
                }
                std::thread([idrcKey]() {
                    // handling the dragon control on a separate thread is required
                    // to avoid blocking Skyrim's main thread (freezing the game).
                    // In ALL functions used within the OnKeyDown() logic we need to ensure that 
                    // ANY change to one of the Skyrim objects is executed on Skyrim's main thread.
                    // This is achieved by passing the corresponding statements to the main thread
                    // via SKSE::GetTaskInterface()->AddTask()
                    FlyingModeManager::GetSingleton().OnKeyDown(idrcKey);
                }).detach();
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    void ControlsManager::InitializeData() {
        m_controlBlocked = false;
        m_isRegistered = false;
        m_initialAutoCombatMode = false;
        m_useSneakKey = true;
    }

    bool ControlsManager::RegisterForControls(bool a_reRegisterOnLoad, bool a_registerFromGoTDragonCompanions) {

        if (!m_isRegistered) {
            auto* inputManager = RE::BSInputDeviceManager::GetSingleton();
            if (!inputManager) {
                log::error("IDRC - {}: BSInputDeviceManager is null", __func__);
                return false;
            }
            // start keyboard tracing (via ProcessEvent())
            inputManager->AddEventSink(this);
            
            // start gamepad thumbstick tracing
            ThumbstickTracerHook::Install();
            CombatTargetTracer::GetSingleton().Register();
//            CrosshairTracer::GetSingleton().Register();
//            MagicEffectTracer::GetSingleton().Register();
  

            m_isRegistered = true;
            log::info("IDRC - {}: Registered for controls", __func__);
        } else {
            log::warn("IDRC - {}: Already registered for controls", __func__);
        }

        FlyingModeManager::GetSingleton().ResetDragonHeight();
        
        if (!a_reRegisterOnLoad){
            DataManager::GetSingleton().SetAutoCombat(m_initialAutoCombatMode);
        }

        SetUseSneakKey(!a_registerFromGoTDragonCompanions);

        return m_isRegistered;
    }

    bool ControlsManager::UnregisterForControls() {
        if (m_isRegistered) {

            // stop thumbstick tracing
            ThumbstickTracerHook::Uninstall();

            auto* inputManager = RE::BSInputDeviceManager::GetSingleton();
            if (!inputManager) {
                log::error("IDRC - {}: BSInputDeviceManager is null", __func__);
                return false;
            }

            // stop keyboard tracing
            inputManager->RemoveEventSink(this);
            CombatTargetTracer::GetSingleton().Unregister();
 //           CrosshairTracer::GetSingleton().Unregister();
 //            MagicEffectTracer::GetSingleton().Unregister();


            m_isRegistered = false;
            log::info("IDRC - {}: Unregistered for controls", __func__);
        } else {
            log::warn("IDRC - {}: Not registered for controls", __func__);
        }
        return m_isRegistered;
    }

    bool ControlsManager::GetControlBlocked() const {
        return m_controlBlocked;
    }

    void ControlsManager::SetControlBlocked(bool a_block) {
        m_controlBlocked = a_block;
    }

    bool ControlsManager::GetIsKeyPressed(IDRCKey a_key) const {

        DXScanCode scanCode = GetMappedDXScanCode(a_key);
        if (scanCode == InputMap::kMaxMacros) {
            log::error("IDRC - {}: DXScanCode not found", __func__);
            return false;
        }

        auto* deviceManager = RE::BSInputDeviceManager::GetSingleton();
        if (!deviceManager) {
            log::error("IDRC - {}: BSInputDeviceManager is null", __func__);
            return false;
        }

        bool isPressed = false;

        if (deviceManager->IsGamepadConnected()) {
            if (a_key == IDRCKey::kForward || a_key == IDRCKey::kBack || a_key == IDRCKey::kStrafeLeft || a_key == IDRCKey::kStrafeRight) {
                // is the check for a direction key? if so, check thumbstick
                isPressed = ThumbstickTracer::GetSingleton().IsThumbstickKeyPressed(a_key);
            } else {
                // is the key mapped to one of the other gamepad controls?
                GamepadButton gamepadButton(InputMap::GamepadKeycodeToMask(static_cast<uint32_t>(scanCode)));

                isPressed = IsGamepadButtonPressed(gamepadButton);
            }
        }

        if (!isPressed) {
            // No mapped gamepad control found, or no gamepad key pressed. 
            // Check if a keyboard key is pressed
            auto* keyboard = deviceManager->GetKeyboard();
            if (!keyboard) {
                log::error("IDRC - {}: Keyboard is null", __func__);
                return false;
            }
            
            if (keyboard->curState[static_cast<uint32_t>(scanCode)]) {
                isPressed = (static_cast<uint32_t>(scanCode) < 256) && ((keyboard->curState[static_cast<uint32_t>(scanCode)] & 0x80) != 0);
            }
        }
        if (!isPressed) {
            // Check if the key is pressed on the mouse
            auto* mouse = deviceManager->GetMouse();
            if (!mouse) {
                return false;
            }
            auto it = mouse->deviceButtons.find(static_cast<uint32_t>(static_cast<uint32_t>(scanCode) - InputMap::kMacro_MouseButtonOffset));
            if (it != mouse->deviceButtons.end()) {
                const auto& button = it->second;
                isPressed = button && (button->heldDownSecs > 0.0); // Check if the button is held down
            } 
        }

        return isPressed;
    }

    float  ControlsManager::GetThumbstickAngle() const {
        return ThumbstickTracer::GetSingleton().GetThumbstickAngle();
    }

    bool ControlsManager::IsThumbstickPressed() const {
        auto* deviceManager = RE::BSInputDeviceManager::GetSingleton();
        if (!deviceManager) {
            log::error("IDRC - {}: BSInputDeviceManager is null", __func__);
            return false;
        }

        if (!deviceManager->IsGamepadConnected()) {
            return false; // No gamepad connected
        }

        return ThumbstickTracer::GetSingleton().IsThumbstickPressed();
    }

    void ControlsManager::SetKeyMapping(const std::string& a_key, const DXScanCode& a_mappedScanCode) {
        static const std::unordered_map<std::string, IDRCKey> keyMapping = {
            {"Forward", IDRCKey::kForward},
            {"Back", IDRCKey::kBack},
            {"Sprint", IDRCKey::kSprint},
            {"StrafeLeft", IDRCKey::kStrafeLeft},
            {"StrafeRight", IDRCKey::kStrafeRight},
            {"DisplayHealth", IDRCKey::kDisplayHealth},
            {"DragonUp", IDRCKey::kUp},
            {"DragonDown", IDRCKey::kDown},
            {"Run", IDRCKey::kRun},
            {"Sneak", IDRCKey::kSneak},
            {"Jump", IDRCKey::kJump},
            {"ToggleAlwaysRun", IDRCKey::kToggleAlwaysRun},
            {"ToggleAutoCombat", IDRCKey::kToggleAutoCombat},
            {"Activate", IDRCKey::kActivate}
        };
    
        if (a_mappedScanCode == InputMap::kMaxMacros) {
            log::error("IDRC - {}: Invalid DXScanCode", __func__);
            return;
        }
    
        auto it = keyMapping.find(a_key);
        if (it != keyMapping.end()) {
            m_keyMap[it->second] = a_mappedScanCode;
            log::info("Key mapping updated: IDRCKey = {} ({}), MappedValue = {}", a_key, static_cast<int>(it->second), a_mappedScanCode.key);
        } else {
            log::warn("IDRC - {}: Unsupported key string '{}'", __func__, a_key);
        }
    }

    DXScanCode ControlsManager::GetMappedDXScanCode(IDRCKey a_key) const {
        auto it = m_keyMap.find(a_key);
        if (it != m_keyMap.end()) {
            return it->second; // Return the mapped value if the key exists
        }
        log::warn("Key mapping not found for IDRCKey: {}", static_cast<int>(a_key));
        DXScanCode invalid(InputMap::kMaxMacros);
        return invalid;
    }

    DXScanCode ControlsManager::GetMappedDXScanCode(const RE::ButtonEvent* a_buttonEvent) const {
        if (!a_buttonEvent) {
            log::error("IDRC - {}: ButtonEvent is null", __func__);
            return DXScanCode(InputMap::kMaxMacros);
        }

        auto* deviceManager = RE::BSInputDeviceManager::GetSingleton();
        if (!deviceManager) {
            log::error("IDRC - {}: BSInputDeviceManager is null", __func__);
            return DXScanCode(InputMap::kMaxMacros);
        }

        DXScanCode scanCode(a_buttonEvent->GetIDCode());
    
        if (deviceManager->IsGamepadConnected() && a_buttonEvent->device.get() == RE::INPUT_DEVICE::kGamepad) {
            // the buttonEvent is triggered by a gamepad button - convert IDCode to DXScanCode
            uint32_t buttonID = InputMap::GamepadMaskToKeycode(a_buttonEvent->GetIDCode());
            if (buttonID == InputMap::kMaxMacros) {
                log::warn("IDRC - {}: Gamepad button ID {} not found by GamepadMaskToKeycode", __func__, a_buttonEvent->GetIDCode());
                return DXScanCode(InputMap::kMaxMacros);
            }

            scanCode = buttonID;
        } else if (a_buttonEvent->device.get() == RE::INPUT_DEVICE::kMouse) {
            // the buttonEvent is triggered by a mouse button - convert IDCode to DXScanCode
            uint32_t buttonID = InputMap::kMacro_MouseButtonOffset + a_buttonEvent->GetIDCode();
            if (buttonID == InputMap::kMaxMacros) {
                log::warn("IDRC - {}: Mouse button ID {} not found", __func__, a_buttonEvent->GetIDCode());
                return DXScanCode(InputMap::kMaxMacros);
            }

            scanCode = buttonID;
        }

        return scanCode;
    }

    IDRCKey ControlsManager::GetMappedIDRCKey(const RE::ButtonEvent* a_buttonEvent) {

        if (!a_buttonEvent) {
            log::error("IDRC - {}: ButtonEvent is null", __func__);
            return IDRCKey::kInvalid;
        }

        auto* deviceManager = RE::BSInputDeviceManager::GetSingleton();
        if (!deviceManager) {
            log::error("IDRC - {}: BSInputDeviceManager is null", __func__);
            return IDRCKey::kInvalid;
        }

        DXScanCode scanCode = GetMappedDXScanCode(a_buttonEvent);
        if (scanCode == InputMap::kMaxMacros) {
            log::warn("IDRC - {}: DXScanCode not found", __func__);
            return IDRCKey::kInvalid;
        }

        // Jump, Sneak and Activate are always tied to the same control
        if (a_buttonEvent->QUserEvent() == "Jump") {
            // Update keyMap (actual key value depends on device, and control mapping)
            SetKeyMapping("Jump", scanCode);
            return IDRCKey::kJump;
        } else if (a_buttonEvent->QUserEvent() == "Sneak" && m_useSneakKey) {
            // Update keyMap (actual key value depends on device, and control mapping)
            // Sneak key can be disabled via SetUseSneakKey(false)
            SetKeyMapping("Sneak", scanCode);
            return IDRCKey::kSneak;
        } else if (a_buttonEvent->QUserEvent() == "Activate") {
            // Update keyMap (actual key value depends on device, and control mapping)
            SetKeyMapping("Activate", scanCode);
            return IDRCKey::kActivate;
        } else {
            // the other keys can be remapped by the user in the MCM
            // the mapping is stored in the keyMap

            for (const auto& [idrcKey, scanCodeValue] : m_keyMap) {
                if (scanCodeValue == scanCode) {
                    return idrcKey; // Return the IDRCKey if the value matches
                }
            }
        }     
        return IDRCKey::kInvalid;
    }

    bool ControlsManager::IsGamepadButtonPressed(const GamepadButton& a_gamepadButton) const {
        auto* deviceManager = RE::BSInputDeviceManager::GetSingleton();
        if (!deviceManager) {
            log::error("IDRC - {}: BSInputDeviceManager is null", __func__);
            return false;
        }
        auto* gamepad = deviceManager->GetGamepad();
        if (!gamepad) {
            log::warn("IDRC - {}: gamepad is null", __func__);
            return false;
        }

        bool isPressed = false;
        auto it = gamepad->deviceButtons.find(static_cast<uint32_t>(a_gamepadButton));
        if (it != gamepad->deviceButtons.end()) {
            const auto& button = it->second;
            isPressed = button && (button->heldDownSecs > 0.0); // Check if the button is held down
        } 

        return isPressed;
    }

    void ControlsManager::TouchGamepadButton(const RE::ButtonEvent* a_buttonEvent) {
        if (!a_buttonEvent) {
            log::error("IDRC - {}: ButtonEvent is null", __func__);
            return;
        }

        if (a_buttonEvent->device.get() != RE::INPUT_DEVICE::kGamepad) {
            return;
        }

        auto* deviceManager = RE::BSInputDeviceManager::GetSingleton();
        if (!deviceManager) {
            log::error("IDRC - {}: BSInputDeviceManager is null", __func__);
            return;
        }

        auto* gamepad = deviceManager->GetGamepad();
        if (!gamepad) {
            log::warn("IDRC - {}: gamepad is null", __func__);
            return;
        }
        if (!deviceManager->IsGamepadConnected()) {
            return;
        }

        auto it = gamepad->deviceButtons.find(a_buttonEvent->GetIDCode());
        if (it != gamepad->deviceButtons.end()) {
            auto& button = it->second;
            //  heldDownSecs > 0 indicates button-touched
            button->heldDownSecs += 0.1f;
        } 
    }

    void ControlsManager::TouchMouseButton(const RE::ButtonEvent* a_buttonEvent) {
        if (!a_buttonEvent) {
            log::error("IDRC - {}: ButtonEvent is null", __func__);
            return;
        }

        if (a_buttonEvent->device.get() != RE::INPUT_DEVICE::kMouse) {
            return;
        }

        auto* deviceManager = RE::BSInputDeviceManager::GetSingleton();
        if (!deviceManager) {
            log::error("IDRC - {}: BSInputDeviceManager is null", __func__);
            return;
        }

        auto* mouse = deviceManager->GetMouse();
        if (!mouse) {
            log::warn("IDRC - {}: mouse is null", __func__);
            return;
        }

        auto it = mouse->deviceButtons.find(a_buttonEvent->GetIDCode());
        if (it != mouse->deviceButtons.end()) {
            auto& button = it->second;
            //  heldDownSecs > 0 indicates button-touched
            button->heldDownSecs += 0.1f;
        } 
    }

    void ControlsManager::PrintAllDeviceMappings(RE::BSInputDevice* a_device, uint32_t a_maxKeyCode) {
        if (!a_device) {
            log::error("IDRC - {}: Input device is null", __func__);
            return;
        }
    
        auto* deviceManager = RE::BSInputDeviceManager::GetSingleton();
        if (!deviceManager) {
            log::error("IDRC - {}: BSInputDeviceManager is null", __func__);
            return;
        }
    
        RE::BSFixedString mapping;
    
        log::info("IDRC - {}: Scanning all mappings for device: {}", __func__, static_cast<int>(a_device->device));
    
        // Iterate over a range of possible keyCodes
        for (uint32_t keyCode = 0; keyCode <= a_maxKeyCode; ++keyCode) {
            if (deviceManager->GetDeviceKeyMapping(a_device->device, keyCode, mapping)) {
                log::info("IDRC - {}: keyCode: {}, Mapping: {}", __func__, keyCode, mapping.c_str());
            }
        }
    
        log::info("IDRC - {}: Finished scanning mappings for device: {}", __func__, static_cast<int>(a_device->device));
    }

    void ControlsManager::SetInitialAutoCombatMode(bool a_auto) {
        m_initialAutoCombatMode = a_auto;
    }

    bool ControlsManager::GetInitialAutoCombatMode() const {
        return m_initialAutoCombatMode;
    }

    void ControlsManager::SetUseSneakKey(bool a_useSneakKey) {
        m_useSneakKey = a_useSneakKey;
    }
 } // namespace IDRC
