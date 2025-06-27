#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "ModAPI.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;
using namespace std::literals;

#define DLLEXPORT __declspec(dllexport)

#include "Plugin.h"