cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optional: enable architecture-specific tuning (e.g. -march=native). Off by default for portability.
option(ENABLE_ARCH_TUNING "Enable -march=native or /arch:AVX2 optimizations (less portable)" OFF)

if(WIN32)
    # Only configure platform when using a Visual Studio generator.
    if(NOT DEFINED CMAKE_GENERATOR_PLATFORM AND CMAKE_GENERATOR MATCHES "Visual Studio")
        set(CMAKE_GENERATOR_PLATFORM "x64" CACHE STRING "Windows target platform for Visual Studio generators")
    endif()
endif()

# Allow the user or environment to override Apple-specific settings.
if(APPLE)
    if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Supported OSX architectures")
    endif()
    if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OSX deployment target")
    endif()
endif()

 # Project declaration (include C for pfft C sources)
project(Analytiks VERSION 1.5.0 LANGUAGES C CXX)

add_subdirectory(JUCE)

file(GLOB_RECURSE SOURCE_FILES
    "Source/*.cpp"
    "Source/*.h"
)

file(GLOB PFFT_SOURCES
    "pfft/*.c"
    "pfft/*.h"
)

file(GLOB RESOURCE_FILES
    "resources/*.svg"
    "resources/*.ttf"
)

file(GLOB RWQUEUE_HEADERS
    "rwqueue/*.h"
)

file(GLOB RWQUEUE_RESOURCES
    "rwqueue/*.md"
    "rwqueue/*.cmake.in"
)

 # Choose sane defaults for plugin formats and allow overrides from the cache
if(APPLE)
    set(DEFAULT_PLUGIN_FORMATS "AU;VST3;Standalone")
else()
    set(DEFAULT_PLUGIN_FORMATS "VST3;Standalone")
endif()
set(PLUGIN_FORMATS ${DEFAULT_PLUGIN_FORMATS} CACHE STRING "Plugin formats to build (semicolon-separated)")


juce_add_plugin(
    Analytiks
        COMPANY_NAME "MixedSignals"
        JucePlugin_Name "Analytics"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE Manu
        PLUGIN_CODE xObb
        FORMATS ${PLUGIN_FORMATS}
        PRODUCT_NAME "Analytiks"
)

target_sources(Analytiks
    PRIVATE
        ${SOURCE_FILES}
        ${PFFT_SOURCES}
)

juce_add_binary_data(AnalytiksResources
    SOURCES
        ${RESOURCE_FILES}
        ${RWQUEUE_RESOURCES}
)

target_link_libraries(Analytiks
    PRIVATE
        AnalytiksResources
    PUBLIC
        juce::juce_animation
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_box2d
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_opengl
)

target_include_directories(Analytiks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/pfft
        ${CMAKE_CURRENT_SOURCE_DIR}/rwqueue
)

target_compile_definitions(Analytiks
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
    PRIVATE
        # Add debug/release specific flags if needed
        $<$<CONFIG:Debug>:DEBUG=1 _DEBUG=1>
        $<$<CONFIG:Release>:NDEBUG=1>
)

if(MSVC)
    target_compile_options(Analytiks PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
    )
    # Enable MSVC link-time optimization / whole-program optimization for Release
    set_target_properties(Analytiks PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    if(ENABLE_ARCH_TUNING)
        target_compile_options(Analytiks PRIVATE $<$<CONFIG:Release>:/arch:AVX2>)
    endif()
else()
    target_compile_options(Analytiks PRIVATE
        $<$<CONFIG:Release>:-O3>
    )
    # Add some additional speed-focused flags for GCC/Clang in Release
    target_compile_options(Analytiks PRIVATE
        $<$<CONFIG:Release>:-funroll-loops>
    )
    # Enable link-time optimization where supported
    set_target_properties(Analytiks PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    if(ENABLE_ARCH_TUNING)
        target_compile_options(Analytiks PRIVATE $<$<CONFIG:Release>:-march=native>)
    endif()
endif()

set_source_files_properties(
    ${PFFT_SOURCES}
    PROPERTIES
    LANGUAGE C
)