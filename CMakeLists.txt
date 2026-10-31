cmake_minimum_required (VERSION 3.22)

# macOS: run on 10.13 and newer
set (CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
# Windows: link the C++ runtime statically, so nobody needs to install a Visual C++ redistributable
set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project (NovaBandsalat VERSION 0.1.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

#===============================================================================
# JUCE: downloaded automatically, or taken from a local checkout (-DNOVA_JUCE_DIR=...)
set (NOVA_JUCE_DIR "" CACHE PATH "Optional local JUCE checkout")
if (NOVA_JUCE_DIR)
    add_subdirectory (${NOVA_JUCE_DIR} JUCE)
else()
    include (FetchContent)
    FetchContent_Declare (JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.12
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE)
    FetchContent_MakeAvailable (JUCE)
endif()

set (NOVA_FORMATS VST3 AU Standalone CACHE STRING "Plugin formats to build")
option (NOVA_LINUX_MINIMAL "Linux test build without optional X11 extensions and audio devices" OFF)

#===============================================================================
juce_add_plugin (NovaBandsalat
    COMPANY_NAME                  "NOVA"
    BUNDLE_ID                     "com.nova.bandsalat"
    PLUGIN_MANUFACTURER_CODE      Nova
    PLUGIN_CODE                   Bs77
    FORMATS                       ${NOVA_FORMATS}
    PRODUCT_NAME                  "NOVA Bandsalat"
    DESCRIPTION                   "Cassette deck effect"
    IS_SYNTH                      FALSE
    NEEDS_MIDI_INPUT              FALSE
    NEEDS_MIDI_OUTPUT             FALSE
    IS_MIDI_EFFECT                FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS   FALSE
    COPY_PLUGIN_AFTER_BUILD       FALSE
    VST3_CATEGORIES               Fx Distortion
    AU_MAIN_TYPE                  kAudioUnitType_Effect
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT    "NOVA Bandsalat verarbeitet das Audio deines Eingangs.")

juce_add_binary_data (NovaBandsalatData
    SOURCES
        Resources/Fonts/Michroma.ttf
        Resources/Fonts/Archivo.ttf
        Resources/Fonts/ReenieBeanie.ttf
        Resources/Fonts/PlexMono.ttf)

target_sources (NovaBandsalat
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/TapeEngine.cpp
        Source/NovaLook.cpp)

target_compile_definitions (NovaBandsalat
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0)

if (NOVA_LINUX_MINIMAL)
    target_compile_definitions (NovaBandsalat PUBLIC
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_ALSA=0 JUCE_JACK=0)
endif()

if (MSVC)
    target_compile_options (NovaBandsalat PRIVATE /utf-8)
endif()

target_link_libraries (NovaBandsalat
    PRIVATE
        NovaBandsalatData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
