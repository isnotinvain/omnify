# CMakeLists.txt for OmnifyPlugin

cmake_minimum_required(VERSION 3.15)
project("Omnify" VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build for arm64 only
set(CMAKE_OSX_ARCHITECTURES "arm64")

# Add JUCE as a subdirectory. Assumes JUCE is in a 'JUCE' folder at the root.
add_subdirectory(JUCE)

juce_add_plugin(Omnify
    COMPANY_NAME "alexlevenson"
    PRODUCT_NAME "Omnify"
    DESCRIPTION "Play any instrument like an omnichord inspired autoharp"
    COMPANY_WEBSITE "https://github.com/isnotinvain/omnify"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE "ALEV"
    PLUGIN_CODE "Omni"
    FORMATS VST3)

# Collect all source files in the current directory
file(GLOB OMNIFY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

target_sources(Omnify
    PRIVATE
        ${OMNIFY_SOURCES})

target_link_libraries(Omnify
    PRIVATE
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_include_directories(Omnify PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(Omnify PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)