cmake_minimum_required(VERSION 3.13)
project(ryfi)

file(GLOB_RECURSE SRC "vendor/*.cpp" "src/*.cpp")

add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE "vendor/")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

add_subdirectory("libcorrect/")
target_include_directories(${PROJECT_NAME} PUBLIC "libcorrect/include")
target_link_libraries(${PROJECT_NAME} PUBLIC correct_static)

if (MSVC)
    # Lib path
    target_link_directories(${PROJECT_NAME} PRIVATE "C:/Program Files/PothosSDR/lib/")

    # Misc headers
    target_include_directories(${PROJECT_NAME} PRIVATE "C:/Program Files/PothosSDR/include/")

    # Volk
    target_link_libraries(${PROJECT_NAME} PRIVATE volk bladeRF)

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
    )

    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs pthread)
        target_include_directories(bladerf_source PRIVATE "/usr/local/include")
    endif ()
endif ()