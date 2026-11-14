cmake_minimum_required(VERSION 3.22)
project(Woditor C OBJC)

set(WODITOR_APP_NAME "Woditor" CACHE STRING "App name")
set(WODITOR_BUNDLE_ID "den1580.woditor" CACHE STRING "Bundle identifier")
set(WODITOR_PROJECT_ID "default" CACHE STRING "projectId (as in settings.js)")
set(WODITOR_VERSION "0.1" CACHE STRING "Version")
set(WODITOR_BUILD "1" CACHE STRING "Build number")

get_filename_component(TOP "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(PRE "${CMAKE_CURRENT_SOURCE_DIR}/engine-prebuilt")
if(NOT EXISTS "${PRE}/libwoditor_engine.a")
  message(FATAL_ERROR "Missing ${PRE}/libwoditor_engine.a")
endif()

file(GLOB HOST_SRC ${TOP}/host/*.c)
set(RUNTIME_SRC ${TOP}/runtime/wasm-rt-impl.c ${TOP}/runtime/wasm-rt-mem-impl.c ${TOP}/runtime/wasm-rt-exceptions-impl.c)
set(GLUE_SRC ${PRE}/gen_invoke.c ${PRE}/gen_imports_weak.c ${PRE}/gen_asm_consts.c)
file(GLOB PLATFORM_SRC ${TOP}/platform/ios/*.m ${TOP}/platform/ios/pad/*.m)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/Info.plist @ONLY)

add_executable(Woditor MACOSX_BUNDLE ${HOST_SRC} ${RUNTIME_SRC} ${GLUE_SRC} ${PLATFORM_SRC})
target_compile_definitions(Woditor PRIVATE WASM_RT_USE_MMAP=1 WASM_RT_MEMCHECK_BOUNDS_CHECK=1 WODITOR_MMAP_LIMITED=1
  WASM_RT_SKIP_SIGNAL_RECOVERY=1 WASM_RT_NONCONFORMING_UNCHECKED_STACK_EXHAUSTION=1 WODITOR_FAST_MEM=1
  GLES_SILENCE_DEPRECATION=1)
target_include_directories(Woditor PRIVATE ${TOP}/runtime ${PRE} ${TOP}/host ${TOP}/platform/ios)
set_source_files_properties(${RUNTIME_SRC} ${GLUE_SRC} PROPERTIES COMPILE_OPTIONS "-O2;-w")
set_source_files_properties(${HOST_SRC} PROPERTIES COMPILE_OPTIONS "-O2;-Wall;-Wno-unused-parameter;-Wno-unused-function")
set_source_files_properties(${PLATFORM_SRC} PROPERTIES COMPILE_OPTIONS "-fobjc-arc;-O2")
target_link_libraries(Woditor PRIVATE ${PRE}/libwoditor_engine.a z
  "-framework UIKit" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics"
  "-framework OpenGLES" "-framework AudioToolbox" "-framework AVFoundation" "-framework GameController"
  "-framework PhotosUI" "-framework UniformTypeIdentifiers")

set_target_properties(Woditor PROPERTIES
  OUTPUT_NAME "Woditor"
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${WODITOR_BUNDLE_ID}"
  XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0"
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
  XCODE_ATTRIBUTE_ARCHS "arm64"
  XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO"
  XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
  XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
  XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
  XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
  XCODE_ATTRIBUTE_DEAD_CODE_STRIPPING "YES"
  XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "2")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icon.png")
  set(_ic "${CMAKE_CURRENT_SOURCE_DIR}/icon.png")
  set(_app "$<TARGET_BUNDLE_CONTENT_DIR:Woditor>")
  add_custom_command(TARGET Woditor POST_BUILD
    COMMAND sips -s format png -z 120 120 "${_ic}" --out "${_app}/AppIcon60x60@2x.png" > /dev/null
    COMMAND sips -s format png -z 180 180 "${_ic}" --out "${_app}/AppIcon60x60@3x.png" > /dev/null
    COMMAND sips -s format png -z 152 152 "${_ic}" --out "${_app}/AppIcon76x76@2x~ipad.png" > /dev/null
    COMMAND sips -s format png -z 167 167 "${_ic}" --out "${_app}/AppIcon83.5x83.5@2x~ipad.png" > /dev/null)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/game/Data.wolf" OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/game/Data" OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/game/Game.ini")
  add_custom_command(TARGET Woditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/game" "$<TARGET_BUNDLE_CONTENT_DIR:Woditor>/game")
endif()
