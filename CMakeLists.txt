cmake_minimum_required(VERSION 3.16)

project(AdaptiveDirectionalHearingAid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_compile_options(-Wall -Wextra -Wconversion -pedantic)


add_library(fir STATIC
  Fir1.cpp
)

target_include_directories(fir PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(alsa_devices STATIC
  ALSADevices.cpp
)

target_include_directories(alsa_devices PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)

target_include_directories(alsa_devices PUBLIC
  ${ALSA_INCLUDE_DIRS}
)

target_link_libraries(alsa_devices PUBLIC
  ${ALSA_LIBRARIES}
)

add_executable("main"
  play_audio.cpp
)

target_link_libraries("main"
  PRIVATE
    fir
    alsa_devices
)
