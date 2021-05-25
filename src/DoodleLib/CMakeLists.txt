
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
SET(ENV{VIRTUAL_ENV} "${PROJECT_SOURCE_DIR}/venv")
#add_custom_command(
#        OUTPUT core/MetadataTabSql.h
##        COMMAND venv/Scripts/Activate.bat
#        COMMAND py $<TARGET_FILE:sqlpp11::ddl2cpp>  core/000-metadata.sql core/MetadataTabSql.h
#)

add_library(DoodleLib
        SHARED
        winDoodleLib.rc
        ${DOODLELIB_HEADER}
        ${DOODLELIB_SOURCE}
        core/MetadataTabSql.h
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
        <DoodleLib/DoodleLib_fwd.h>
        )
#get_target_property(DOODLE_PCH_S DoodleLib PRECOMPILE_HEADERS )
#message(${DOODLE_PCH_S})

target_link_libraries(DoodleLib PUBLIC
#        SqlppSqlite
        DoodleLibResource
        # RTTR::Core
        cereal
        magic_enum::magic_enum
        ${Boost_LIBRARIES}
        nlohmann_json nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
#        EnTT::EnTT
#        Boost::locale
#        Boost::filesystem
#        Boost::log
        $<IF:$<CONFIG:Debug>,${WX_based},${WX_base}>
        $<IF:$<CONFIG:Debug>,${WX_cored},${WX_core}>
        $<IF:$<CONFIG:Debug>,${WX_advd},${WX_adv}>
        SqlppMySql #这个是我们自己寻找的mysql sqlpp连接器

        PRIVATE
        ${DOODLE_MAR}

        )


target_compile_definitions(DoodleLib
        PUBLIC
        _WIN32_WINNT=0x0A00
        WIN32_LEAN_AND_MEAN
        PRIVATE
        _CRT_SECURE_NO_WARNINGS
#        $<$<CONFIG:Debug>:_CRTDBG_MAP_ALLOC>
        )


target_include_directories(DoodleLib
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        )
