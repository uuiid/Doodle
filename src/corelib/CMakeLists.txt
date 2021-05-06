
cmrc_add_resource_library(
        CoreResource
        resource/abcExport.py
        resource/mayaExport.py
        resource/Ue4CraeteLevel.py
)

add_library(core SHARED
        core_Cpp.h
        core_global.h)
add_subdirectory(core)
add_subdirectory(FileWarp)
add_subdirectory(FileSys)
add_subdirectory(Metadata)

set_target_properties(core
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#设置导出符号
include(GenerateExportHeader)
set(MY_CUSTOM_CONTENT "")
generate_export_header(core
        EXPORT_MACRO_NAME CORE_API
        CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)
#预编译头文件
target_precompile_headers(core
        PUBLIC
        <filesystem>
        <variant>
        <string>
        <memory>
        <chrono>
        <map>
        <iostream>
        <regex>
        <fstream>
        <mutex>
        <condition_variable>
        <future>
        <vector>
        <unordered_set>
        <queue>
        <functional>
        <stdexcept>
        )

target_link_libraries(core PUBLIC
        SqlppSqlite
        CoreResource
        # RTTR::Core
        cereal
        magic_enum::magic_enum
        ${Boost_LIBRARIES}
        nlohmann_json nlohmann_json::nlohmann_json
        pinyin
        logger
        ${OpenCV_LIBS}
        )


target_compile_definitions(core PRIVATE CORE_LIBRARY core_EXPORT WIN32_LEAN_AND_MEAN)
target_compile_definitions(core PUBLIC DOODLE_QT NOMINMAX _WIN32_WINNT=0x0A00)


target_include_directories(core PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
#        ${OpenCV_INCLUDE_DIRS}
        )