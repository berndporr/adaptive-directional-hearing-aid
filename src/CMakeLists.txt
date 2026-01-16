cmake_minimum_required(VERSION 3.16)

project(AdaptiveDirectionalHearingAid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wconversion -pedantic)

# ---- FIR library ----
add_library(fir STATIC
    Fir1.cpp
)

target_include_directories(fir PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}../include 
)

# ---- ALSA library ----
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)

add_library(alsa_devices STATIC
    ALSADevices.cpp
)

target_include_directories(alsa_devices PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}../include 
    ${ALSA_INCLUDE_DIRS}
)

target_link_libraries(alsa_devices PUBLIC
    ${ALSA_LIBRARIES}
)

# ---- Torch / LibTorch ----
find_package(Torch REQUIRED torch)

# ---- DNF Torch library ----
add_library(dnf_torch STATIC
    dnf_torch.cpp
)

target_include_directories(dnf_torch PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}../include 
    ${TORCH_INCLUDE_DIRS}
)

target_link_libraries(dnf_torch ${TORCH_LIBRARIES})

# ---- Executables ----
add_executable(main_fir
    play_audio_fir.cpp
)

add_executable(main_neural
    play_audio_neural.cpp
)

add_executable(main_training
    play_audio_training.cpp
)

add_executable(main_test
    play_audio_test_trained.cpp
)


target_link_libraries(main_fir PRIVATE
    fir
    alsa_devices
)

target_link_libraries(main_neural PRIVATE
    dnf_torch
    alsa_devices
    torch
)

target_link_libraries(main_training PRIVATE
    dnf_torch
    alsa_devices
    torch
)

target_link_libraries(main_test PRIVATE
    dnf_torch
    alsa_devices
    torch
)

include(GNUInstallDirs)
