
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

add_custom_target(
        create_list_doodle_cmake
        COMMAND ${CMAKE_COMMAND} -P ListFile.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

include(DoodleLib.cmake)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/core/MetadataTabSql.h
        COMMAND ${PROJECT_SOURCE_DIR}/venv/Scripts/Activate.bat
        COMMAND py
        ARGS $<TARGET_FILE:sqlpp11::ddl2cpp>
             ${CMAKE_CURRENT_LIST_DIR}/core/000-metadatatab.sql
             ${CMAKE_CURRENT_BINARY_DIR}/core/MetadataTabSql
             doodle
        MAIN_DEPENDENCY core/000-metadatatab.sql
)
add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/MetadataServer.pb.h
            ${CMAKE_CURRENT_BINARY_DIR}/MetadataServer.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/MetadataServer.grpc.pb.h
            ${CMAKE_CURRENT_BINARY_DIR}/MetadataServer.grpc.pb.cc
        COMMAND protobuf::protoc
        ARGS --proto_path=${Z_VCPKG_ROOT_DIR}/installed/${VCPKG_TARGET_TRIPLET}/include
        --proto_path=${CMAKE_CURRENT_LIST_DIR}/rpc
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        MetadataServer.proto
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        MAIN_DEPENDENCY rpc/MetadataServer.proto
)


add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/FileSystemServer.pb.h
            ${CMAKE_CURRENT_BINARY_DIR}/FileSystemServer.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/FileSystemServer.grpc.pb.h
            ${CMAKE_CURRENT_BINARY_DIR}/FileSystemServer.grpc.pb.cc
        COMMAND protobuf::protoc
        ARGS --proto_path=${Z_VCPKG_ROOT_DIR}/installed/${VCPKG_TARGET_TRIPLET}/include
        --proto_path=${CMAKE_CURRENT_LIST_DIR}/rpc
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        FileSystemServer.proto
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        MAIN_DEPENDENCY rpc/FileSystemServer.proto
)
add_library(DoodleLib
        SHARED
        winDoodleLib.rc
        ${DOODLELIB_HEADER}
        ${DOODLELIB_SOURCE}
        ${CMAKE_CURRENT_BINARY_DIR}/core/MetadataTabSql.h
        ${CMAKE_CURRENT_BINARY_DIR}/MetadataServer.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/MetadataServer.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/MetadataServer.grpc.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/MetadataServer.grpc.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/FileSystemServer.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/FileSystemServer.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/FileSystemServer.grpc.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/FileSystemServer.grpc.pb.cc
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
        date::date
        date::date-tz
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
#        cppzmq
#        cppzmq-static
        gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts gRPC::grpc++_reflection
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
        _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
        )


target_include_directories(DoodleLib
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        )
