
cmake_minimum_required(VERSION 3.21)

project(TrayApp LANGUAGES CXX RC)

add_executable(TrayApp WIN32
    TrayApp/TrayApp.cpp
    TrayApp/TrayApp.rc
)

target_include_directories(TrayApp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/TrayApp"
)

target_compile_features(TrayApp PRIVATE cxx_std_17)

target_compile_definitions(TrayApp PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(TrayApp PRIVATE
    User32
    Shell32
    Advapi32
)

set_target_properties(TrayApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
