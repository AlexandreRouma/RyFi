cmake_minimum_required(VERSION 3.13)
project(ryfi)

file(GLOB_RECURSE SRC "vendor/*.cpp" "src/*.cpp")

add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE "vendor/")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

add_subdirectory("libcorrect/")
target_include_directories(${PROJECT_NAME} PRIVATE "libcorrect/include")
target_link_libraries(${PROJECT_NAME} PRIVATE correct_static)

# Compiler flags
if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    # Debug Flags
    if (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /std:c++17 /EHsc)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE -g -Og -std=c++17 -Wno-unused-command-line-argument -undefined dynamic_lookup)
    else ()
        target_compile_options(${PROJECT_NAME} PRIVATE -g -Og -std=c++17)
    endif ()
else()
    # Normal Flags
    if (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2 /Ob2 /std:c++17 /EHsc)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE -O3 -std=c++17 -Wno-unused-command-line-argument -undefined dynamic_lookup)
    else ()
        target_compile_options(${PROJECT_NAME} PRIVATE -O3 -std=c++17)
    endif ()
endif()

if (MSVC)
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
