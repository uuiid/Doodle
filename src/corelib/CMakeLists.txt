

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportFbx.h 
                          ${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportFbx.cpp
COMMAND ${BIN2CPP} --file=${CMAKE_CURRENT_LIST_DIR}/resource/mayaExport.py --output=. --headerfile=CoreResourceMayaExportFbx.h --identifier=CoreResourceMayaExportFbx --override 
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
DEPENDS resource/mayaExport.py
COMMENT "bin2cpp create pinyin lib file"
)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportAbc.h 
                          ${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportAbc.cpp
COMMAND ${BIN2CPP} --file=${CMAKE_CURRENT_LIST_DIR}/resource/abcExport.py --output=. --headerfile=CoreResourceMayaExportAbc.h --identifier=CoreResourceMayaExportAbc --override 
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
DEPENDS resource/abcExport.py
COMMENT "bin2cpp create pinyin lib file"
)

add_library(core SHARED
${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportFbx.h 
${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportFbx.cpp
${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportAbc.h 
${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportAbc.cpp

)
add_subdirectory(core)
add_subdirectory(FileWarp)
add_subdirectory(FileSys)

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

# find_package(Freetype REQUIRED)
# set(HARFBUZZ_ROOT "F:/vcpkg/installed/x64-windows" CACHE PATH "harfbuzz install root dir")
# find_path(HARFBUZZ_INCLUDE_DIRS
# NAMES hb-ft.h
# PATH_SUFFIXES harfbuzz
# HINTS ${HARFBUZZ_ROOT}/include)
# if(CMAKE_BUILD_TYPE MATCHES "Debug" )
# message("is debug")
# find_library(HARFBUZZ_LIBRARIES
#     NAMES harfbuzz
#     HINTS ${HARFBUZZ_ROOT}/debug/lib)
# else()
# message("not is debug")
# find_library(HARFBUZZ_LIBRARIES
#     NAMES harfbuzz
#     HINTS ${HARFBUZZ_ROOT}/lib)
# endif()
# find_package_handle_standard_args(HARFBUZZ
#     DEFAULT_MSG HARFBUZZ_LIBRARIES HARFBUZZ_INCLUDE_DIRS)

target_link_libraries(core PUBLIC 
SqlppSqlite
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

