cmake_minimum_required(VERSION 3.25)

# This tells cmake we have goodies in the /cmake folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (PamplejuceVersion)

# Modern concise way to add dependencies to your project
include (CPM)

# Configures universal binaries and decides which version of macOS to support
include(PamplejuceMacOS)

# Couple tweaks that IMO should be JUCE defaults
include(JUCEDefaults)

# Change me!
# This is the internal name of the project and the name of JUCE's shared code target
# Note: This cannot have spaces (it may be 2024, but you can't have it all!)
# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs display)
set(PROJECT_NAME "ChopShop")

# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs will display)
# You can also just have it be the same thing as PROJECT_NAME
# You may want to append the major version on the end of this (and PROJECT_NAME) ala:
#   set(PROJECT_NAME "MyPlugin_v${MAJOR_VERSION}")
# Doing so enables major versions to show up in IDEs and DAWs as separate plugins
# allowing you to change parameters and behavior without breaking existing user projects
set(PRODUCT_NAME "ChopShop Beta")

# Change me! Used for the MacOS bundle name and Installers
set(COMPANY_NAME "Pounding Systems")

# Change me! Used for the MacOS bundle identifier (and signing)
set(BUNDLE_ID "com.poundingSystems.chopshop")

# Change me! Set the plugin formats you want built
# Valid choices: AAX Unity VST VST3 AU AUv3 Standalone
set(FORMATS Standalone AU VST3 AUv3)

# For simplicity, the name of the CMake project is also the name of the target
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

# JUCE is setup as a submodule in the /JUCE folder
# Locally, you must run `git submodule update --init --recursive` once
# and later `git submodule update --remote --merge` to keep it up to date
# On Github Actions, this is done as a part of actions/checkout
add_subdirectory(JUCE)

# Add any other modules you want modules here, before the juce_add_gui_app call
# juce_add_module(modules/my_module)

# This adds the melatonin inspector module
add_subdirectory (melatonin_inspector)

# Add Tracktion modules
add_subdirectory(tracktion_engine/modules)

# Create a library for the minibpm module
add_library(minibpm STATIC
    source/minibpm.cpp
    source/minibpm.h
)
target_include_directories(minibpm PUBLIC source)

# Add this before juce_add_gui_app
# Remove the JUCE_GENERATE_JUCE_HEADER_ARGS block
# set(JUCE_GENERATE_JUCE_HEADER_ARGS
#     JUCE_HEADER_NAME "JuceHeader.h"
#     JUCE_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/JuceLibraryCode"
# )

# See `docs/CMake API.md` in the JUCE repo for all config options
juce_add_gui_app("${PROJECT_NAME}"
    # Icons for the app
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"

    # Change me!
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"

    # The name of your final executable
    # This can be different from PROJECT_NAME and can have spaces!
    PRODUCT_NAME "${PRODUCT_NAME}"
    
    # Change this from TRUE to FALSE to prevent JUCE from generating the header
    NEEDS_JUCE_HEADER FALSE)

# This lets us use our code in both the JUCE targets and our Test target
# Without running into ODR violations
add_library(SharedCode INTERFACE)

# Enable fast math, C++20 and a few other target defaults
include(SharedCodeDefaults)

# Add JUCE modules include path to fix JuceHeader.h not found error
target_include_directories(SharedCode INTERFACE 
    "${CMAKE_CURRENT_SOURCE_DIR}/Source")

# Manually list all .h and .cpp files for the app
# If you are like me, you'll use globs for your sanity.
# Just ensure you employ CONFIGURE_DEPENDS so the build system picks up changes
# If you want to appease the CMake gods and avoid globs, manually add files like so:
# set(SourceFiles Source/Main.h Source/Main.cpp Source/MainComponent.h Source/MainComponent.cpp)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

# Adds a BinaryData target for embedding assets into the binary
# include(Assets)

# MacOS only: Cleans up folder and target organization on Xcode.
include(XcodePrettify)

# This is where you can set preprocessor definitions for JUCE and your app
target_compile_definitions(SharedCode
    INTERFACE

    # JUCE_WEB_BROWSER and JUCE_USE_CURL off by default
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1

    # lets the app known if we're Debug or Release
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Debug>:_DEBUG=1>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG=1>
    $<$<NOT:$<CONFIG:Debug>>:_NDEBUG=1>
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"

    # App name without version
    PRODUCT_NAME_WITHOUT_VERSION="ChopShop"
)

# Link to any other modules you added (with juce_add_module) here!
# Usually JUCE modules must have PRIVATE visibility
# See https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module
# However, with Pamplejuce, you'll link modules to SharedCode with INTERFACE visibility
# This allows the JUCE app target and the Tests target to link against it
target_link_libraries(SharedCode
    INTERFACE
    Assets
    melatonin_inspector
    juce_audio_utils
    juce_audio_formats
    juce_audio_devices
    juce_audio_processors
    juce_audio_basics
    juce_dsp
    juce_gui_basics
    juce_gui_extra
    juce_graphics
    juce_data_structures
    juce_core
    juce_events
    juce_opengl
    juce_osc
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    tracktion_core
    tracktion_engine
    tracktion_graph
    minibpm)

# Link the JUCE app target to our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

# IPP support, comment out to disable
include(PamplejuceIPP)

# Everything related to the tests target
# include(Tests)

# A separate target for Benchmarks (keeps the Tests target fast)
# include(Benchmarks)

# Output some config for CI (like our PRODUCT_NAME)
include(GitHubENV)

# Add this after the SharedCode target is created
target_compile_features(SharedCode INTERFACE cxx_std_20)

# Add this section to handle SDL3 dependencies
if(APPLE)
    find_package(SDL3 REQUIRED)
    target_link_libraries(SharedCode INTERFACE SDL3::SDL3)
    
    # Copy SDL3 dylib to the app bundle
    add_custom_command(TARGET "${PROJECT_NAME}" POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "/usr/local/lib/libSDL3.0.dylib"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../MacOS/libSDL3.0.dylib"
        COMMENT "Copying SDL3 library to app bundle")
        
    # Fix the install name
    add_custom_command(TARGET "${PROJECT_NAME}" POST_BUILD
        COMMAND install_name_tool -change @rpath/libSDL3.0.dylib @executable_path/libSDL3.0.dylib
        "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Fixing SDL3 install name")
endif()

# Update the Assets section to include your specific files
juce_add_binary_data(Assets SOURCES
    Source/Audiowide-Regular.ttf
    Source/JetBrainsMono-VariableFont_wght.ttf
    Source/screw-icon.png
)

# Add a version manually instead
set(CURRENT_VERSION "1.0.0")
set(MAJOR_VERSION "1")
set(MINOR_VERSION "0")
set(PATCH_VERSION "0")
