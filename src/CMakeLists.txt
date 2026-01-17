# CMakeLists.txt for OmnifyPlugin

cmake_minimum_required(VERSION 3.15)
project("Omnify" VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to arm64 for local development, but allow CI to override for universal builds
if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

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
    FORMATS AU VST3 Standalone)

# Collect all source files
file(GLOB OMNIFY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/datamodel/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/voicing_styles/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/components/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/panels/*.cpp"
)

target_sources(Omnify
    PRIVATE
        ${OMNIFY_SOURCES})

# Add binary resources (fonts)
juce_add_binary_data(OmnifyBinaryData
    SOURCES
        Resources/Orbitron-Regular.ttf)

target_link_libraries(Omnify
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_osc
        OmnifyBinaryData
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

target_include_directories(Omnify PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann"
)

target_compile_definitions(Omnify PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)

if(MSVC)
    target_compile_options(Omnify PRIVATE /W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Omnify PRIVATE
        -Wall
        -Wstrict-aliasing
        -Wuninitialized
        -Wconversion
        -Wsign-compare
        -Wint-conversion
        -Wconditional-uninitialized
        -Wconstant-conversion
        -Wbool-conversion
        -Wextra-semi
        -Wunreachable-code
        -Wcast-align
        -Wshift-sign-overflow
        -Wmissing-prototypes
        -Wnullable-to-nonnull-conversion
        -Wno-unused-parameter
        -Wno-shadow-field-in-constructor)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(Omnify PRIVATE
        -Wall
        -Wextra
        -Wstrict-aliasing
        -Wuninitialized
        -Wconversion
        -Wsign-compare
        -Wunreachable-code
        -Wcast-align
        -Wno-unused-parameter)
endif()
