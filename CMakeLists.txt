# It's recommended to set a minimum CMake version.
# If you use CMake features from higher versions, update this to match.
cmake_minimum_required(VERSION 3.21)

# Set your project name. This will be the name of your SKSE .dll file.
set(NAME "IntuitiveDragonRideControl" CACHE STRING "")
set(VERSION 5.0.2 CACHE STRING "")

project(
	${NAME}
	VERSION ${VERSION}
	LANGUAGES CXX
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/Plugin.h.in
	${CMAKE_CURRENT_BINARY_DIR}/include/Plugin.h
	@ONLY
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/plugin.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/plugin.rc
	@ONLY
)

list(APPEND SOURCES ${CMAKE_CURRENT_BINARY_DIR}/plugin.rc)

set(OUTPUT_FOLDER "C:/Users/tom/AppData/Local/ModOrganizer/MO_SkyrimSE/mods/_DEV_IntuitiveDragonRideControl")

option(ENABLE_SKYRIM_SE "Enable support for Skyrim SE in the dynamic runtime feature." ON)
option(ENABLE_SKYRIM_AE "Enable support for Skyrim AE in the dynamic runtime feature." ON)
option(ENABLE_SKYRIM_VR "Enable support for Skyrim VR in the dynamic runtime feature." ON)
set(BUILD_TESTS OFF)

# Get all source files from src/ and include/
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h include/*.h)

# Add the include directory for TS_SKSEFunctions
set(TSSKSEFUNCTIONS_INCLUDE_DIR "../TS_SKSEFunctions-main/include")
file(GLOB_RECURSE TSSKSEFUNCTIONS_SOURCES "../TS_SKSEFunctions-main/src/*.cpp" "../TS_SKSEFunctions-main/include/*.h")

# Combine the source files
list(APPEND SOURCES ${TSSKSEFUNCTIONS_SOURCES})

# --- CommonLibSSE: Use local path from environment variable ---
set(COMMONLIBSSE_PATH $ENV{CommonLibSSEPath_NG})

if(NOT COMMONLIBSSE_PATH)
    message(FATAL_ERROR "CommonLibSSEPath_NG environment variable is not set!")
endif()

add_subdirectory("$ENV{CommonLibSSEPath_NG}" CommonLibSSE EXCLUDE_FROM_ALL)

# Add your SKSE plugin target
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) # <--- use C++23 standard
target_precompile_headers(${PROJECT_NAME} PRIVATE include/PCH.h) # <--- PCH.h is required!

target_include_directories(${PROJECT_NAME} PRIVATE include ${TSSKSEFUNCTIONS_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/include)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

# Link to CommonLibSSE (adjust the library name if needed)
target_link_libraries(${PROJECT_NAME} PRIVATE CommonLibSSE::CommonLibSSE)

# Include inih for .ini files
find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")
target_include_directories(${PROJECT_NAME} PUBLIC ${SIMPLEINI_INCLUDE_DIRS})

# When your SKSE .dll is compiled, this will automatically copy the .dll into your mods folder.
# Only works if you configure DEPLOY_ROOT above (or set the SKYRIM_MODS_FOLDER environment variable)
if(DEFINED OUTPUT_FOLDER)
    # If you specify an <OUTPUT_FOLDER> (including via environment variables)
    # then we'll copy your mod files into Skyrim or a mod manager for you!

    # Copy the SKSE plugin .dll files into the SKSE/Plugins/ folder
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )

    # If you perform a "Debug" build, also copy .pdb file (for debug symbols)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()
endif()
