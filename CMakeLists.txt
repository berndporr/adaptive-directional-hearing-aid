cmake_minimum_required(VERSION 3.20)

project(AdaptiveDirectionalHearingAid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wconversion -pedantic)

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
add_executable(main_fir
    play_audio_fir.cpp
)

target_link_libraries(main_fir PRIVATE
    fir
    alsa_devices
)

include(GNUInstallDirs)
