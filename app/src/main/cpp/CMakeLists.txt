cmake_minimum_required(VERSION 3.22.1)
project(wegert C)

set(NATIVE_GLUE_DIR "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")

add_library(native_app_glue STATIC
    "${NATIVE_GLUE_DIR}/android_native_app_glue.c"
)
target_include_directories(native_app_glue PUBLIC "${NATIVE_GLUE_DIR}")

option(
    WEGERT_USE_ICK_PREBUILT
    "Use the separately compiled ICK complex-math object on arm64"
    ON
)

set(WEGERT_SOURCES wegert.c)
if(ANDROID_ABI STREQUAL "arm64-v8a" AND WEGERT_USE_ICK_PREBUILT)
    set(ICK_COMPLEX_MATH_OBJECT
        "${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/arm64-v8a/complex_math_ick.o"
    )
    if(NOT EXISTS "${ICK_COMPLEX_MATH_OBJECT}")
        message(FATAL_ERROR "missing ICK arm64 complex-math object")
    endif()
    set_source_files_properties(
        "${ICK_COMPLEX_MATH_OBJECT}"
        PROPERTIES EXTERNAL_OBJECT TRUE
    )
    list(APPEND WEGERT_SOURCES "${ICK_COMPLEX_MATH_OBJECT}")
else()
    list(APPEND WEGERT_SOURCES complex_math_fallback.c)
endif()

add_library(wegert SHARED ${WEGERT_SOURCES})
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
