cmake_minimum_required(VERSION 3.13)
project(ryfi)

# Build options
option(OPT_BUILD_BLADERF_SUPPORT "Support for BladeRF devices" ON)
option(OPT_BUILD_LIMESDR_SUPPORT "Support for LimeSDR devices" ON)

# Find all source files
file(GLOB_RECURSE SRC "vendor/*.cpp" "src/*.cpp")

# Generate the executable
add_executable(${PROJECT_NAME} ${SRC})

# Include the vendor libraries
target_include_directories(${PROJECT_NAME} PRIVATE "vendor/")

# Select C++20
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Define the hardware support
if (OPT_BUILD_BLADERF_SUPPORT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_BLADERF_SUPPORT)
endif()
if (OPT_BUILD_LIMESDR_SUPPORT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_LIMESDR_SUPPORT)
endif()

# Statically link to libcorrect
add_subdirectory("libcorrect/")
target_include_directories(${PROJECT_NAME} PRIVATE "libcorrect/include")
target_link_libraries(${PROJECT_NAME} PRIVATE correct_static)

# Compiler flags
if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    # Debug Flags
    if (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE -g -Og -std=c++17 -Wno-unused-command-line-argument -undefined dynamic_lookup)
    else ()
        target_compile_options(${PROJECT_NAME} PRIVATE -g -Og -std=c++17)
    endif ()
else()
    # Normal Flags
    if (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2 /Ob2 /EHsc)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE -O3 -std=c++17 -Wno-unused-command-line-argument -undefined dynamic_lookup)
    else ()
        target_compile_options(${PROJECT_NAME} PRIVATE -O3 -std=c++17)
    endif ()
endif()

if (MSVC)
    # Remove warnings meant for retards
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

    # Lib path
    target_link_directories(${PROJECT_NAME} PRIVATE "C:/Program Files/PothosSDR/lib/")

    # Misc headers
    target_include_directories(${PROJECT_NAME} PRIVATE "C:/Program Files/PothosSDR/include/")

    # Volk
    target_link_libraries(${PROJECT_NAME} PRIVATE volk bladeRF LimeSuite)

    # FFTW3
    find_package(FFTW3f CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE FFTW3::fftw3f)
else()
    find_package(PkgConfig)

    pkg_check_modules(FFTW3 REQUIRED fftw3f)
    pkg_check_modules(VOLK REQUIRED volk)
    pkg_check_modules(LIBBLADERF REQUIRED libbladeRF)

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${FFTW3_INCLUDE_DIRS}
        ${VOLK_INCLUDE_DIRS}
        ${LIBBLADERF_INCLUDE_DIRS}
    )
    
    target_link_directories(${PROJECT_NAME} PRIVATE
        ${FFTW3_LIBRARY_DIRS}
        ${VOLK_LIBRARY_DIRS}
        ${LIBBLADERF_LIBRARY_DIRS}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${FFTW3_LIBRARIES}
        ${VOLK_LIBRARIES}
        ${LIBBLADERF_LIBRARIES}
        LimeSuite
    )

    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs pthread)
        target_include_directories(${PROJECT_NAME} PRIVATE "/usr/local/include")
    endif ()
endif ()
