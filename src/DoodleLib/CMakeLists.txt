
cmrc_add_resource_library(
        DoodleLibResource
        resource/abcExport.py
        resource/icon.ico
        resource/mayaExport.py
        resource/mayaIcon.png
        resource/Ue4CraeteLevel.py
        resource/ue4Icon.png
        resource/zhtopy.txt

)

include(DoodleLib.cmake)
message("DOODLELIB_HEADER=${DOODLELIB_HEADER}
DOODLELIB_SOURCE=${DOODLELIB_SOURCE}")

add_library(DoodleLib
        SHARED
        winDoodleLib.rc
        ${DOODLELIB_HEADER}
        ${DOODLELIB_SOURCE}
        DoodleLib.manifest
        )
set_target_properties(DoodleLib
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#设置导出符号
include(GenerateExportHeader)
set(MY_CUSTOM_CONTENT "")

generate_export_header(DoodleLib
        EXPORT_MACRO_NAME DOODLELIB_API
        CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)
#预编译头文件
target_precompile_headers(DoodleLib
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
        <Windows.h>
        <boost/process.hpp> # 这个头要在wx的前面才不会出错
        <wx/wx.h>           # 因为wx中的setup.h 定义了一个pid_t宏 会影响boost中的处理
#        [["#ifdef pid_t"]]
#        [["#undef pid_t"]]
#        [["#endif"]]
        )

target_link_libraries(DoodleLib PUBLIC
        SqlppSqlite
        DoodleLibResource
        # RTTR::Core
        cereal
        magic_enum::magic_enum
        ${Boost_LIBRARIES}
        nlohmann_json nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
        EnTT::EnTT
#        Boost::locale
#        Boost::filesystem
#        Boost::log
        $<IF:$<CONFIG:Debug>,${WX_based},${WX_base}>
        $<IF:$<CONFIG:Debug>,${WX_cored},${WX_core}>
        $<IF:$<CONFIG:Debug>,${WX_advd},${WX_adv}>
        
        PRIVATE 
        ${DOODLE_MAR}

        )


target_compile_definitions(DoodleLib PRIVATE
        DoodleLib_LIBRARY
        DoodleLib_EXPORT
        wxUSE_STD_STRING
        wxUSE_UNICODE_WCHAR
        ) # WIN32_LEAN_AND_MEAN
target_compile_definitions(DoodleLib PUBLIC _WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN)


target_include_directories(DoodleLib
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        )
