cmake_minimum_required(VERSION 3.15)

project(Analytiks VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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


juce_add_plugin(
    Analytiks
        COMPANY_NAME "MixedSignals"
        JucePlugin_Name ""
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE Manu
        PLUGIN_CODE xObb
        FORMATS AU VST3 Standalone
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
    )
else()
    target_compile_options(Analytiks PRIVATE
        $<$<CONFIG:Release>:-O3>
    )
endif()

set_source_files_properties(
    ${PFFT_SOURCES}
    PROPERTIES
    LANGUAGE C
)