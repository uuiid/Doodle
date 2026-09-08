cmake_minimum_required(VERSION 3.14)
project(depths-cpp LANGUAGES CXX)

find_package(onnxruntime CONFIG REQUIRED)
find_package(OpenCV REQUIRED)

# Depths-CPP 是 header-only 库，仅需要 include 目录
add_library(depths-cpp INTERFACE)
target_include_directories(depths-cpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(depths-cpp INTERFACE
    onnxruntime::onnxruntime
    ${OpenCV_LIBS}
)

# C++17 要求（与 Depths-CPP 一致）
target_compile_features(depths-cpp INTERFACE cxx_std_17)

# 安装
install(DIRECTORY include/ DESTINATION include)
install(TARGETS depths-cpp EXPORT depths-cpp-config)
install(EXPORT depths-cpp-config DESTINATION lib/cmake/depths-cpp)