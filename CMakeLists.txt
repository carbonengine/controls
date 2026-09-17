cmake_minimum_required(VERSION 3.16)
project(_carbon_controls VERSION 1.0.0)

if(APPLE)
    enable_language(OBJCXX)
endif()

include(cmake/CcpBuildConfigurations.cmake)
include(cmake/CcpTargetConfigurations.cmake)
  
find_package(Python3 COMPONENTS Development REQUIRED)
find_package(carbon-blueexposure REQUIRED CONFIG)
find_package(carbon-blue REQUIRED CONFIG)
find_package(carbon-core REQUIRED CONFIG)

set(CARBON_CONTROLS_SOURCE_FILES
    src/carbon_controls.cpp
    src/ControlManager.cpp
    src/ControlManager_Blue.cpp
    src/ControlManager.h
    src/DeviceEnums.h
    src/DeviceEnums.cpp
    src/InputDevice.h
    src/InputDevice.cpp
    src/InputDevice_Blue.cpp
    src/InputElement.h
    src/InputElement.cpp
    src/InputElement_Blue.cpp
    src/StdAfx.h
    src/events/Events.h
    src/events/Events.cpp
    src/events/InputEvent_Blue.cpp
    src/events/InputEvent.cpp
    src/events/InputEvent.h
    src/events/InputEventTrigger_Blue.cpp
    src/events/InputEventTrigger.cpp
    src/events/InputEventTrigger.h
    src/events/controller/ControllerAxisInputEvent_Blue.cpp
    src/events/controller/ControllerAxisInputEvent.cpp
    src/events/controller/ControllerAxisInputEvent.h
    src/events/controller/ControllerButtonInputEvent_Blue.cpp
    src/events/controller/ControllerButtonInputEvent.cpp
    src/events/controller/ControllerButtonInputEvent.h
    src/events/controller/ControllerSwitchInputEvent_Blue.cpp
    src/events/controller/ControllerSwitchInputEvent.cpp
    src/events/controller/ControllerSwitchInputEvent.h
    src/handlers/IInputHandler.h
    src/handlers/InputHandlerApple.h
    src/handlers/InputHandlerApple.mm
    src/handlers/InputHandlerStub.cpp
    src/handlers/InputHandlerStub.h
    src/handlers/InputHandlerWin.cpp
    src/handlers/InputHandlerWin.h
    src/handlers/InputMappingApple.h
    src/handlers/InputMappingApple.mm
    src/handlers/InputMappingWin.cpp
    src/handlers/InputMappingWin.h
)

ccp_add_library(_carbon_controls SHARED ${CARBON_CONTROLS_SOURCE_FILES})
set_build_flavor(_carbon_controls)

add_trinity_dev_debug_flags(_carbon_controls)

target_precompile_headers(_carbon_controls PRIVATE src/StdAfx.h)

if(APPLE)
    # ControlManager.cpp includes InputHandlerApple.h which pulls in Obj-C types; compile it as Obj-C++.
    set_source_files_properties(
        src/ControlManager.cpp
        PROPERTIES
            LANGUAGE OBJCXX
            COMPILE_FLAGS "-fobjc-arc -Wno-unguarded-availability-new"
            SKIP_PRECOMPILE_HEADERS ON
    )
    set_source_files_properties(
        src/handlers/InputHandlerApple.mm
        PROPERTIES
            COMPILE_FLAGS "-fobjc-arc -Wno-unguarded-availability-new"
            SKIP_PRECOMPILE_HEADERS ON
    )
endif()

set_target_properties(_carbon_controls PROPERTIES
    VS_GLOBAL_RunCodeAnalysis false
    VS_GLOBAL_EnableMicrosoftCodeAnalysis false
    VS_GLOBAL_EnableClangTidyCodeAnalysis true
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${CARBON_CONTROLS_SOURCE_FILES}
)

set_target_properties(_carbon_controls PROPERTIES OUTPUT_NAME "_carbon_controls")

target_compile_definitions(_carbon_controls PRIVATE BLUE_WITH_PYTHON=1)
target_link_libraries(_carbon_controls PRIVATE 
        Blue 
        CcpCore
        Python3::Python
)
if( WIN32 )
    find_package(gameinput CONFIG REQUIRED)

    target_link_libraries(_carbon_controls PRIVATE 
        Microsoft::GameInput 
    )

    # The Windows SDK ships a legacy v1 GameInput.h that shadows the vcpkg v3 header
    # on case-insensitive Windows. Copy the vcpkg header to a unique filename so neither the
    # compiler nor IntelliSense can ever resolve to the SDK version.
    get_target_property(_gameinput_inc Microsoft::GameInput INTERFACE_INCLUDE_DIRECTORIES)
    if(_gameinput_inc)
        configure_file(
            "${_gameinput_inc}/gameinput.h"
            "${CMAKE_BINARY_DIR}/gameinput_vcpkg/gameinput_v3.h"
            COPYONLY)
        target_include_directories(_carbon_controls BEFORE PRIVATE
            "${CMAKE_BINARY_DIR}/gameinput_vcpkg")
    endif()
elseif( APPLE )
    target_link_libraries(_carbon_controls PRIVATE
        "-framework GameController"
        "-framework Foundation"
        "-framework CoreHaptics"
    )

    target_link_options(_carbon_controls PRIVATE
        "LINKER:-force_load,$<TARGET_FILE:BlueExposure>"
    )
endif()

# -----------------------------------------------------------------------------
# controller_diagnostic — standalone console app that drives IInputHandler directly.
# -----------------------------------------------------------------------------
set(CONTROLLER_DIAGNOSTIC_SOURCES
    tools/controller_diagnostic.cpp
    src/events/Events.cpp
    src/DeviceEnums.cpp
)
if(WIN32)
    list(APPEND CONTROLLER_DIAGNOSTIC_SOURCES
        src/handlers/InputHandlerWin.cpp
        src/handlers/InputMappingWin.cpp
    )
elseif(APPLE)
    list(APPEND CONTROLLER_DIAGNOSTIC_SOURCES
        src/handlers/InputHandlerApple.mm
        src/handlers/InputMappingApple.mm
    )
else()
    list(APPEND CONTROLLER_DIAGNOSTIC_SOURCES
        src/handlers/InputHandlerStub.cpp
    )
endif()

add_executable(controller_diagnostic ${CONTROLLER_DIAGNOSTIC_SOURCES})
target_include_directories(controller_diagnostic PRIVATE src)
target_link_libraries(controller_diagnostic PRIVATE Blue)

if(APPLE)
    set_source_files_properties(
        tools/controller_diagnostic.cpp
        PROPERTIES
            LANGUAGE OBJCXX
            COMPILE_FLAGS "-fobjc-arc"
    )
    target_link_libraries(controller_diagnostic PRIVATE
        "-framework GameController"
        "-framework Foundation"
        "-framework CoreHaptics"
    )
elseif(WIN32)
    target_link_libraries(controller_diagnostic PRIVATE Microsoft::GameInput)
    # Point at the copied unique-name header (see comment on _carbon_controls above).
    target_include_directories(controller_diagnostic BEFORE PRIVATE
        "${CMAKE_BINARY_DIR}/gameinput_vcpkg")
endif()

# Provide an install target if this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL _carbon_controls)
    # Determine if/when to generate/build documentation. The rules are as follows:
    # - If running locally - Build documentation is OFF by default
    # - If running on a build agent (TeamCity) - Build documentation is ON by default
    # - If -DBUILD_DOCUMENTATION=ON/OFF flag is explicitly set, use it.
    set(BUILD_DOCUMENTATION_DEFAULT_FLAG OFF)
    if(DEFINED ENV{TEAMCITY_VERSION})
        set(BUILD_DOCUMENTATION_DEFAULT_FLAG ON)
    endif()
    message(STATUS "Document generation settings: default=${BUILD_DOCUMENTATION_DEFAULT_FLAG}, option BUILD_DOCUMENTATION=${BUILD_DOCUMENTATION}")
    option(BUILD_DOCUMENTATION "Build Documentation" ${BUILD_DOCUMENTATION_DEFAULT_FLAG})

    if(BUILD_DOCUMENTATION)
        # Set up sphinx build for Python autodoc comments.
        set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/doc/source)
        set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/doc/build)

        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        include(cmake/CcpDocsGenerator.cmake)

        create_carbon_docs_sphinx_target(
                PYTHON_EXE ${Python3_EXECUTABLE}
                VENV_NAME docs_generation_venv
                PYTHONPATH_ENV $ENV{PYTHONPATH}
                SPHINX_SOURCE ${SPHINX_SOURCE}
                SPHINX_BUILD ${SPHINX_BUILD}
                DOXYGEN_SRC_FILES ${SRC_FILES}
                SPHINX_TARGET_NAME Sphinx
                DOXYGEN_TARGET_NAME Doxygen
                INSTALL_DESTINATION documentation
        )

        add_dependencies(Sphinx _carbon_controls)
    endif()


    option(INSTALL_TO_MONOLITH "Install this package to a perforce branch vendor folder - provided for backwards compatability" OFF)
    if (INSTALL_TO_MONOLITH)
        include(cmake/CcpMonolithVendorConfig.cmake)
        include(cmake/CcpPackageConfigHelpers.cmake)


        # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
        install(
                TARGETS _carbon_controls
                EXPORT package_nameTarget
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
                ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
                RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        )

        # Generate target configuration for the monolith
        configure_ccp_vendor_config_file(
                TARGET _carbon_controls
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/carbon_controlsConfig.cmake"
                DEPENDENCIES ""
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/carbon_controlsConfig.cmake DESTINATION .)
        install(DIRECTORY python DESTINATION . FILES_MATCHING PATTERN "*.py")
    else ()
        # Install rule for available public headers
        install(
                TARGETS _carbon_controls
                EXPORT CarbonControlsTarget
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                DESTINATION ${CMAKE_INSTALL_PREFIX}/share/CarbonControls
                LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
                ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
                RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        )

        install(EXPORT CarbonControlsTarget
                DESTINATION share/CarbonControls
                FILE CarbonControls.cmake
        )

        install(
                FILES carbon_controlsConfig.cmake
                DESTINATION ${CMAKE_INSTALL_PREFIX}/share/CarbonControls)
    endif()

    install(DIRECTORY include DESTINATION . FILES_MATCHING PATTERN "*.h")

endif()