cmake_minimum_required(VERSION 3.22)

project(DopplerFX VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures")
endif()

# ---------------------------------------------------------------------------
# JUCE
#
# By default JUCE is downloaded at configure time. To build against a local
# copy instead (much faster for repeat builds):
#     cmake -B build -DJUCE_PATH=/path/to/JUCE
# ---------------------------------------------------------------------------
if (DEFINED JUCE_PATH AND NOT JUCE_PATH STREQUAL "")
    message(STATUS "DopplerFX: using local JUCE at ${JUCE_PATH}")
    add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    message(STATUS "DopplerFX: fetching JUCE 8.0.4")
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
set(DOPPLERFX_FORMATS VST3 Standalone)
if (APPLE)
    list(APPEND DOPPLERFX_FORMATS AU)
endif()

juce_add_plugin(DopplerFX
    COMPANY_NAME                "Entropic Labs"
    BUNDLE_ID                   "com.entropiclabs.dopplerfx"
    # Manufacturer/plugin codes form the VST3 class ID. Renaming the company
    # is cosmetic, but changing these would orphan projects that already
    # load the plugin, so they stay put.
    PLUGIN_MANUFACTURER_CODE    Dplr
    PLUGIN_CODE                 Dpfx
    FORMATS                     ${DOPPLERFX_FORMATS}
    PRODUCT_NAME                "Doppler FX"
    DESCRIPTION                 "Entropic Labs Doppler FX - physically modelled motion effect"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx Modulation Delay
    AU_MAIN_TYPE                kAudioUnitType_Effect)

target_sources(DopplerFX PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/Presets.cpp
    Source/dsp/DopplerEngine.cpp
    Source/dsp/PitchShifter.cpp
    Source/dsp/ResonantFilter.cpp
    Source/dsp/Gate.cpp
    Source/dsp/Limiter.cpp
    Source/gui/LookAndFeel.cpp
    Source/gui/RadarDisplay.cpp
    Source/gui/Controls.cpp)

target_compile_definitions(DopplerFX PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(DopplerFX PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

target_include_directories(DopplerFX PRIVATE Source)

# ---------------------------------------------------------------------------
# Headless DSP checks (optional; -DDOPPLERFX_BUILD_TESTS=OFF to skip)
# ---------------------------------------------------------------------------
option(DOPPLERFX_BUILD_TESTS "Build the headless DSP checks" ON)

if (DOPPLERFX_BUILD_TESTS)
    juce_add_console_app(DopplerFXTests PRODUCT_NAME "DopplerFXTests")

    target_sources(DopplerFXTests PRIVATE
        Tests/DspTests.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Parameters.cpp
    Source/Presets.cpp
        Source/dsp/DopplerEngine.cpp
        Source/dsp/PitchShifter.cpp
        Source/dsp/ResonantFilter.cpp
        Source/dsp/Gate.cpp
        Source/dsp/Limiter.cpp
        Source/gui/LookAndFeel.cpp
        Source/gui/RadarDisplay.cpp
        Source/gui/Controls.cpp)

    target_compile_definitions(DopplerFXTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JucePlugin_Name="Doppler FX"
        JucePlugin_Desc="Physically modelled doppler motion effect"
        JucePlugin_Manufacturer="Doppler Audio"
        JucePlugin_VersionString="1.0.0"
        JucePlugin_IsSynth=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0)

    target_include_directories(DopplerFXTests PRIVATE Source)

    target_link_libraries(DopplerFXTests PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    enable_testing()
    add_test(NAME dsp-checks COMMAND DopplerFXTests)
endif()
