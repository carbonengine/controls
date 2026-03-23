cmake_minimum_required(VERSION 3.16)
project(carbon_controls VERSION 1.0.0)

include(cmake/CcpBuildConfigurations.cmake)
include(cmake/CcpTargetConfigurations.cmake)

find_package(carbon-blueexposure CONFIG REQUIRED)
  
set(PUBLIC_HEADER_FILES
    include/IControllerInput.h
)
set(PRIVATE_HEADER_FILES
    include/stub/ControllerInputStub.h   
    include/windows/ControllerInputWin.h
)

set(CARBON_CONTROLS_SOURCE_FILES
    ${PUBLIC_HEADER_FILES}
    ${PRIVATE_HEADER_FILES}
    src/carbon_controls.cpp
    src/stub/ControllerInputStub.cpp
    src/windows/ControllerInputWin.cpp
)

ccp_add_library(carbon-controls SHARED ${CARBON_CONTROLS_SOURCE_FILES})

set_target_properties(carbon-controls PROPERTIES
    VS_GLOBAL_RunCodeAnalysis false
    VS_GLOBAL_EnableMicrosoftCodeAnalysis false
    VS_GLOBAL_EnableClangTidyCodeAnalysis true
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${CARBON_CONTROLS_SOURCE_FILES}
)

set_target_properties(carbon-controls PROPERTIES OUTPUT_NAME "carbon-controls")

target_include_directories(carbon-controls
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(carbon-controls PRIVATE BlueExposure)
  
if( WIN32 )
        find_package(ms-gdk CONFIG REQUIRED)
        target_link_libraries(carbon-controls PRIVATE Xbox::GameRuntime)
endif() 
# Provide an install target if this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
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

        add_dependencies(Sphinx carbon-controls)
    endif()


    option(INSTALL_TO_MONOLITH "Install this package to a perforce branch vendor folder - provided for backwards compatability" OFF)
    if (INSTALL_TO_MONOLITH)
        include(cmake/CcpMonolithVendorConfig.cmake)
        include(cmake/CcpPackageConfigHelpers.cmake)

        # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
        install(
                TARGETS carbon-controls
                EXPORT package_nameTarget
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                LIBRARY DESTINATION ${CCP_VENDOR_LIB_PATH}
                ARCHIVE DESTINATION ${CCP_VENDOR_LIB_PATH}
                RUNTIME DESTINATION ${CCP_VENDOR_BIN_PATH}
        )

        # Generate target configuration for the monolith
        configure_ccp_vendor_config_file(
                TARGET carbon-controls
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/CarbonControlsConfig.cmake"
                DEPENDENCIES "BLUEEXPOSURE"
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CarbonControlsConfig.cmake DESTINATION .)
        install(DIRECTORY python DESTINATION . FILES_MATCHING PATTERN "*.py")
    else ()
        # Install rule for available public headers
        install(
                TARGETS carbon-controls
                EXPORT CarbonControlsTarget
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                DESTINATION ${CMAKE_INSTALL_PREFIX}/share/CarbonControls
                ARCHIVE DESTINATION lib
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION bin
        )

        install(EXPORT CarbonControlsTarget
                DESTINATION share/CarbonControls
                FILE CarbonControls.cmake
        )

        install(
                FILES CarbonControlsConfig.cmake
                DESTINATION ${CMAKE_INSTALL_PREFIX}/share/CarbonControls)
    endif()

    install(DIRECTORY include DESTINATION . FILES_MATCHING PATTERN "*.h")

endif()