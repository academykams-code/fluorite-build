cmake_minimum_required(VERSION 3.20)
project(libloader LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
find_library(UIKIT_FRAMEWORK UIKit REQUIRED)
find_library(METAL_FRAMEWORK Metal REQUIRED)
find_library(METALKIT_FRAMEWORK MetalKit REQUIRED)
find_library(SECURITY_FRAMEWORK Security REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.mm")

add_library(libloader SHARED ${SOURCES})

set_target_properties(libloader PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION A
    MACOSX_FRAMEWORK_IDENTIFIER com.appdome.libloader
    MACOSX_FRAMEWORK_BUNDLE_VERSION 1
    MACOSX_FRAMEWORK_SHORT_VERSION_STRING 2.6.2
    OUTPUT_NAME libloader
    XCODE_ATTRIBUTE_INSTALL_PATH "@rpath"
)

target_include_directories(libloader PUBLIC include)

target_link_libraries(libloader PRIVATE
    ${FOUNDATION_FRAMEWORK}
    ${UIKIT_FRAMEWORK}
    ${METAL_FRAMEWORK}
    ${METALKIT_FRAMEWORK}
    ${SECURITY_FRAMEWORK}
)
