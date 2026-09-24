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
)

set(CARBON_CONTROLS_PLATFORM_SOURCE_FILES
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

# Sources only compiled into the mock flavor of the module used by the tests.
set(CARBON_CONTROLS_MOCK_SOURCE_FILES
    src/handlers/MockInputHandler.h
    src/handlers/MockInputHandler.cpp
)

# Applies the configuration shared by the real and the mock flavor of the module.
function(configure_carbon_controls_target target)
    set_build_flavor(${target})
    add_trinity_dev_debug_flags(${target})
    target_precompile_headers(${target} PRIVATE src/StdAfx.h)
    set_target_properties(${target} PROPERTIES
        VS_GLOBAL_RunCodeAnalysis false
        VS_GLOBAL_EnableMicrosoftCodeAnalysis false
        VS_GLOBAL_EnableClangTidyCodeAnalysis true
        OUTPUT_NAME "_carbon_controls"
    )
    target_compile_definitions(${target} PRIVATE BLUE_WITH_PYTHON=1)
    target_link_libraries(${target} PRIVATE
            Blue
            CcpCore
            Python3::Python
    )
    if(APPLE)
        target_link_options(${target} PRIVATE
            "LINKER:-force_load,$<TARGET_FILE:BlueExposure>"
        )
    endif()
endfunction()

set(CARBON_CONTROLS_COMMON_SOURCE_FILES ${CARBON_CONTROLS_SOURCE_FILES})
list(APPEND CARBON_CONTROLS_SOURCE_FILES ${CARBON_CONTROLS_PLATFORM_SOURCE_FILES})
ccp_add_library(_carbon_controls SHARED ${CARBON_CONTROLS_SOURCE_FILES})
configure_carbon_controls_target(_carbon_controls)

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

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${CARBON_CONTROLS_SOURCE_FILES} ${CARBON_CONTROLS_MOCK_SOURCE_FILES}
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
endif()

# -----------------------------------------------------------------------------
# controller_diagnostic
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

# -----------------------------------------------------------------------------
# Tests — a mock flavor of the module driven by MockInputHandler, exercised by Python unittest
# tests that are run through exefile and reported to CTest in GoogleTest format.
# -----------------------------------------------------------------------------
if(CMAKE_PROJECT_NAME STREQUAL _carbon_controls)
    option(BUILD_TESTING "Build tests. Enabled by default." ON)
endif()

if(BUILD_TESTING)
    enable_testing()
    include(GoogleTest)
    find_package(GTest CONFIG REQUIRED)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    ccp_add_library(_carbon_controls_mock SHARED ${CARBON_CONTROLS_COMMON_SOURCE_FILES} ${CARBON_CONTROLS_MOCK_SOURCE_FILES})
    configure_carbon_controls_target(_carbon_controls_mock)
    target_compile_definitions(_carbon_controls_mock PRIVATE CARBON_CONTROLS_MOCK_INPUT=1)
    # Same module name as the real flavor, so it lives in its own folder that the tests put on the path.
    set_target_properties(_carbon_controls_mock PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/mock"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/mock"
        FOLDER "Tests"
    )

    # Find exefile tool, which is used to run the tests
    if (DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE MATCHES "Release")
        string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
        set(EXEFILE_TOOL_NAME exefile_${CMAKE_BUILD_TYPE_LOWER})
    else()
        set(EXEFILE_TOOL_NAME exefile)
    endif()
    find_program(exefile_tool ${EXEFILE_TOOL_NAME} NO_CACHE REQUIRED)

    set(PYTHON_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/Python)
    # Python test sources are only used to trigger a rebuild of the test target when they change,
    # which in turn triggers test discovery.
    file(GLOB_RECURSE PYTHON_TEST_SOURCES "${PYTHON_TEST_DIR}/*.py")
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TouchedPyFiles.h
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/TouchedPyFiles.h
        DEPENDS ${PYTHON_TEST_SOURCES}
    )

    set(TEST_TARGET CarbonControlsPythonTest)
    add_executable(${TEST_TARGET} ${PYTHON_TEST_DIR}/GTestAdapter/GTestAdapter.cpp ${CMAKE_CURRENT_BINARY_DIR}/TouchedPyFiles.h)
    target_sources(${TEST_TARGET} PRIVATE ${PYTHON_TEST_SOURCES})
    source_group(TREE "${PYTHON_TEST_DIR}" FILES ${PYTHON_TEST_SOURCES})

    if(WIN32)
        add_custom_command(TARGET ${TEST_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${exefile_tool} $<TARGET_FILE_DIR:${TEST_TARGET}>
        )
        set(exefile_tool $<TARGET_FILE_DIR:${TEST_TARGET}>/${EXEFILE_TOOL_NAME}.exe)
        add_custom_command(TARGET ${TEST_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:_carbon_controls_mock> $<TARGET_FILE_DIR:${TEST_TARGET}>
                COMMAND_EXPAND_LISTS
        )
        set(PATH_SEPARATOR "\\;")
    else()
        set(PATH_SEPARATOR ":")
    endif()

    set(PYTHON_LIB_PATH
        "$<TARGET_FILE_DIR:_carbon_controls_mock>"
        "$<TARGET_FILE_DIR:${TEST_TARGET}>"
        "${PYTHON_TEST_DIR}/tests"
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/python"
    )
    if(APPLE)
        list(APPEND PYTHON_LIB_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/python3.12/lib-dynload")
    endif()
    list(APPEND PYTHON_LIB_PATH "${Python3_STDLIB}")
    list(JOIN PYTHON_LIB_PATH "${PATH_SEPARATOR}" PYTHON_LIB_PATH)

    target_compile_definitions(${TEST_TARGET} PRIVATE
        "EXECUTABLE_PATH=\"${exefile_tool}\""
        "REPORTER_PATH=\"${PYTHON_TEST_DIR}/GTestAdapter/gtest_reporter.py\""
        "TEST_PATH=\"${PYTHON_TEST_DIR}/tests\""
        "BUILDFLAVOR=\"$<LOWER_CASE:$<CONFIG>>\""
        "PYTHON_LIB_PATH=\"${PYTHON_LIB_PATH}\""
    )
    target_include_directories(${TEST_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${TEST_TARGET} PRIVATE GTest::gtest)
    add_dependencies(${TEST_TARGET} _carbon_controls_mock)
    set_target_properties(${TEST_TARGET} PROPERTIES FOLDER "Tests")
    gtest_discover_tests(${TEST_TARGET} DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 30 TEST_PREFIX ${TEST_TARGET}.)
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