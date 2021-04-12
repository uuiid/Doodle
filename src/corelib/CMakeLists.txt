
cmrc_add_resource_library(
  CoreResource
  resource/abcExport.py
  resource/mayaExport.py
  resource/NewLevelSequence.uasset
  resource/NewWorld.umap
)

add_library(core SHARED
)
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


target_compile_definitions(core PRIVATE CORE_LIBRARY core_EXPORT)
target_compile_definitions(core PUBLIC DOODLE_QT NOMINMAX)


target_include_directories(core PUBLIC 
${CMAKE_CURRENT_LIST_DIR}
${CMAKE_CURRENT_BINARY_DIR}
PRIVATE
${OpenCV_INCLUDE_DIRS}
)

