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
option(
    WEGERT_ENABLE_DIRECT_JNI
    "Include the direct DEX/JNI boundary in libwegert"
    OFF
)
option(WEGERT_USE_ICK_COMPLEX_SOURCE "Compile the owned complex implementation directly with ICK" OFF)
option(
    WEGERT_ICON_CAPTURE
    "Build the clean Jones-trefoil launcher-icon capture preset"
    OFF
)

set(WEGERT_SOURCES
    wegert.c
    wegert_function.c
    wegert_view.c
    wegert_scene.c
    wegert_gles.c
    wegert_portrait_renderer_gles.c
    wegert_placement_controls.c
    wegert_overlay_renderer_gles.c
)
if(WEGERT_ENABLE_DIRECT_JNI)
    list(APPEND WEGERT_SOURCES wegert_jni.c)
endif()
if(WEGERT_USE_ICK_COMPLEX_SOURCE)
    list(APPEND WEGERT_SOURCES complex_math_ick.c)
elseif(ANDROID_ABI STREQUAL "arm64-v8a" AND WEGERT_USE_ICK_PREBUILT)
    set(ICK_COMPLEX_MATH_OBJECT
        "${CMAKE_CURRENT_SOURCE_DIR}/complex_math_ick.o"
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

set(WEGERT_ICK_SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../ai-ci-ick" CACHE PATH "Pinned shared ICK producer checkout")
get_filename_component(WEGERT_ICK_SHARED "${WEGERT_ICK_SHARED}" ABSOLUTE)
include("${WEGERT_ICK_SHARED}/ick-android/OwnedC.cmake")
set(WEGERT_ICK_STAGE "${CMAKE_CURRENT_SOURCE_DIR}/ick-stages/${ANDROID_ABI}" CACHE PATH "Qualified installed ABI compiler stage")
if(NOT ICK_COMPILER)
    ick_android_stage(ROOT "${WEGERT_ICK_STAGE}" FORTIFY_SOURCE 2)
endif()
set(WEGERT_ICK_OPTIONS -Wall -Wextra -Wpedantic "-ffile-prefix-map=${WEGERT_ICK_SHARED}=/opt/ai-ci-ick")
if(WEGERT_ICON_CAPTURE)
    list(APPEND WEGERT_ICK_OPTIONS -DWEGERT_ICON_CAPTURE=1)
endif()
set(WEGERT_OWNED_C ${WEGERT_SOURCES})
set(WEGERT_EXTERNAL_OBJECTS)
if(ANDROID_ABI STREQUAL "arm64-v8a" AND WEGERT_USE_ICK_PREBUILT AND NOT WEGERT_USE_ICK_COMPLEX_SOURCE)
    list(REMOVE_ITEM WEGERT_OWNED_C "${ICK_COMPLEX_MATH_OBJECT}")
    list(APPEND WEGERT_EXTERNAL_OBJECTS "${ICK_COMPLEX_MATH_OBJECT}")
endif()
ick_android_objects(WEGERT_ICK_OBJECTS NAME wegert
    SOURCES ${WEGERT_OWNED_C}
    INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}" "${NATIVE_GLUE_DIR}"
    OPTIONS ${WEGERT_ICK_OPTIONS}
    STANDARD 11)
add_library(wegert SHARED ${WEGERT_ICK_OBJECTS} ${WEGERT_EXTERNAL_OBJECTS})
set_target_properties(wegert PROPERTIES LINKER_LANGUAGE C)
target_include_directories(wegert PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(wegert PRIVATE c_std_11)
target_compile_options(wegert PRIVATE -Wall -Wextra -Wpedantic)
if(WEGERT_ICON_CAPTURE)
    target_compile_definitions(wegert PRIVATE WEGERT_ICON_CAPTURE=1)
endif()
target_link_options(wegert PRIVATE -u ANativeActivity_onCreate)
target_link_libraries(wegert
    native_app_glue
    android
    log
    EGL
    GLESv3
    m
)
