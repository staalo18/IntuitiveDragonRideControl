#include <string>
#include <filesystem>
#include <SimpleIni.h>


void InitializeLogging() {
	auto path = log_directory();
	if (!path) {
		report_and_fail("Unable to lookup SKSE logs directory.");
}
	*path /= PluginDeclaration::GetSingleton()->GetName();
	*path += L".log";

	std::shared_ptr<spdlog::logger> log;
	if (IsDebuggerPresent()) {
		log = std::make_shared<spdlog::logger>(
			"Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
	}
	else {
		log = std::make_shared<spdlog::logger>(
			"Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
	}
	log->set_level({ spdlog::level::level_enum::info });
	log->flush_on({ spdlog::level::level_enum::trace });

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
}



bool CheckDTryKeyUtilSettings(){
    
    std::string DTryKeyUtilsSettingsPath = "Data\\SKSE\\Plugins\\dtryKeyUtil\\config\\settings.ini";
    std::filesystem::path iniPath = std::filesystem::current_path() / DTryKeyUtilsSettingsPath;

    if (!std::filesystem::is_regular_file(iniPath)) {
      	log::info("IDRC - CheckDTryKeyUtilSettings: No such file: {}", iniPath.string());
        return false;
   }

    bool defaultValue = false;    

    bool bIsIDRCSetting;
    bool bToggleMovementInputTrace;
    bool bThumbStickOctodirecitonalTrace;

    try {
        CSimpleIniA ini;
        if (ini.LoadFile(iniPath.string().c_str()) != SI_OK) {
        	log::info("IDRC - CheckDTryKeyUtilSettings: Failed to parse {}", iniPath.string());
            return false;
        }
        else{
            bIsIDRCSetting = ini.GetBoolValue("General", "bIsIDRCSetting", defaultValue);
            bToggleMovementInputTrace = ini.GetBoolValue("General", "bToggleMovementInputTrace", defaultValue);
            bThumbStickOctodirecitonalTrace = ini.GetBoolValue("MovementInputTrace", "bThumbStickOctodirecitonalTrace", defaultValue);
        	log::info("IDRC - CheckDTryKeyUtilSettings:  bIsIDRCSetting = {}", bIsIDRCSetting);
        	log::info("IDRC - CheckDTryKeyUtilSettings:  bToggleMovementInputTrace = {}", bToggleMovementInputTrace);
        	log::info("IDRC - CheckDTryKeyUtilSettings:  bThumbStickOctodirecitonalTrace = {}", bThumbStickOctodirecitonalTrace);
        }
    } catch (const std::exception& ex) {
    	log::info("IDRC - CheckDTryKeyUtilSettings: Failed to load from .ini: {}", std::string(ex.what()));
        return false;
    } catch (...) {
    	log::info("IDRC - CheckDTryKeyUtilSettings: Failed to load from .ini: Unknown error");
        return false;
    }
    
    return true;
}

/******************************************************************************************/

SKSEPluginLoad(const LoadInterface *skse) {
	InitializeLogging();

	auto* plugin = PluginDeclaration::GetSingleton();
	auto version = plugin->GetVersion();
	log::info("{} {} - Checking for DTryUtils setting...", plugin->GetName(), version);

    Init(skse);

    GetMessagingInterface()->RegisterListener([](MessagingInterface::Message *message) {
           
        if (message->type == MessagingInterface::kDataLoaded){
            bool bCheck = CheckDTryKeyUtilSettings();
        	log::info("IDRC - Check complete. Result: {}", bCheck);
        }
    });

    return true;
}