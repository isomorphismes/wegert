cmake_minimum_required(VERSION 3.22.1)
project(wegert C)

set(NATIVE_GLUE_DIR "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")

add_library(native_app_glue STATIC
    "${NATIVE_GLUE_DIR}/android_native_app_glue.c"
)
target_include_directories(native_app_glue PUBLIC "${NATIVE_GLUE_DIR}")

add_library(wegert SHARED wegert.c)
target_compile_features(wegert PRIVATE c_std_11)
target_compile_options(wegert PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(wegert PRIVATE -u ANativeActivity_onCreate)
target_link_libraries(wegert
    native_app_glue
    android
    log
    EGL
    GLESv3
    m
)
