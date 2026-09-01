# Hybrid triplet: v145 (14.51) for main project, v141-era 14.16 for vcpkg ports/dependencies
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# Use the installed v141-era 14.16 toolset for ports/dependencies, but v145 for the main project
if (PORT)
    set(VCPKG_PLATFORM_TOOLSET_VERSION "14.16")
else()
    set(VCPKG_PLATFORM_TOOLSET_VERSION "14.51")
endif()

set(VCPKG_BUILD_TYPE "release")

set(VCPKG_CMAKE_SYSTEM_VERSION "10.0.22621.0")
set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreadedDLL)

set(VCPKG_ENV_PASSTHROUGH_UNTRACKED PATH_TO_VCPKG_ROOT)
set(VCPKG_LOAD_VCVARS_ENV ON)

set(CARBON_BUILD_TYPE "Debug")

if (PORT MATCHES "carbon-.*")
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../vendor/github.com/carbonengine/vcpkg-registry/toolchains/x64-windows-145-triplet.cmake")
    set(VCPKG_HASH_ADDITIONAL_FILES "${CMAKE_CURRENT_LIST_DIR}/../vendor/github.com/carbonengine/vcpkg-registry/toolchains/x64-windows-145-carbon.cmake")
endif ()

if (PORT MATCHES "libyaml")
    set(VCPKG_LIBRARY_LINKAGE static)
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS "${VCPKG_CMAKE_CONFIGURE_OPTIONS};-DCMAKE_POLICY_VERSION_MINIMUM=3.5")
endif ()

if (PORT MATCHES "ccd")
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

if (PORT MATCHES "curl")
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

if (PORT MATCHES "openssl")
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

if (PORT MATCHES "protobuf")
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

if (PORT MATCHES "zlib")
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

if (PORT MATCHES "libuv")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS "-DBUILD_TESTING=OFF")
endif()

if (PORT MATCHES "carbon-pdmprotowrapper")
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

if (PORT MATCHES "meshoptimizer")
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

if (PORT MATCHES "tinyfiledialogs")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_POLICY_VERSION_MINIMUM=3.5")
endif ()

if (PORT MATCHES "civetweb")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_POLICY_VERSION_MINIMUM=3.5")
endif ()

if (PORT MATCHES "libjpeg-turbo")
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

if (PORT MATCHES "libsquish")
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

if (PORT MATCHES "bzip2")
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

if (PORT MATCHES "gameinput")
    # gameinput requires at least MSVC 14.3
    # Skip it for v141 builds
endif ()
