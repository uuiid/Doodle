

add_executable(doodle
        WIN32
        ${RESOURE_FILE}
        ${RESOURCE_SOURCE}
        win_exe.rc
        #doodle.manifest
        main.cpp

        )

set_target_properties(doodle
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#设置导出符号
# include(GenerateExportHeader)
# set(MY_CUSTOM_CONTENT "")
# generate_export_header(doodle
# EXPORT_MACRO_NAME DOODLE_API
# CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)
#预编译头文件
target_precompile_headers(doodle
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
        <wx/wx.h>
        )

add_subdirectory(source)
target_compile_definitions(doodle PRIVATE wxUSE_STD_STRING wxUSE_UNICODE_WCHAR)
#target_include_directories(doodle PRIVATE ${BOOST_INCLUDE_DIRS} ${wxWidgets_INCLUDE_DIRS})
# target_link_directories(doodle PRIVATE f:/Source/vcpkg/installed/x64-windows/debug/lib/)
#add_custom_target(tset_gen
#COMMAND ${CMAKE_COMMAND} -E echo $<IF:$<CONFIG:Debug>,${WX_based},${WX_base}>
#)

target_link_libraries(doodle
        PRIVATE
        $<IF:$<CONFIG:Debug>,${WX_based},${WX_base}>
        $<IF:$<CONFIG:Debug>,${WX_cored},${WX_core}>
        $<IF:$<CONFIG:Debug>,${WX_advd},${WX_adv}>
        )


target_link_libraries(doodle PUBLIC ${Boost_LIBRARIES})
target_link_libraries(doodle PRIVATE core)
target_link_libraries(doodle PRIVATE logger)