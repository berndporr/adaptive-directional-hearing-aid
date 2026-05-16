cmake_minimum_required(VERSION 3.20)

project(AdaptiveDirectionalHearingAid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wconversion -pedantic)
include(GNUInstallDirs)

# ---- ALSA library ----
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)

add_library(alsa_devices STATIC
    ALSADevices.cpp
)

target_include_directories(alsa_devices PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ALSA_INCLUDE_DIRS}
)

target_link_libraries(alsa_devices PUBLIC
    ${ALSA_LIBRARIES}
)

# ---- Executables ----
add_executable(fir_filtering
    fir_filtering.cpp
    adaptive_filter.cpp
)

target_link_libraries(fir_filtering PRIVATE
    fir
    alsa_devices
)

# ---- Tests ----
add_executable(test_playback
    test_playback.cpp
)

target_link_libraries(test_playback PRIVATE
    alsa_devices
)

add_executable(test_recording
    test_recording.cpp
)

target_link_libraries(test_recording PRIVATE
    alsa_devices
)

add_executable(test_passthrough
    test_passthrough.cpp
)

target_link_libraries(test_passthrough PRIVATE
    alsa_devices
)
