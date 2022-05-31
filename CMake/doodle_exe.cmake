include(CMakePrintHelpers)
include(GenerateExportHeader)

function(doodle_grpc_generate out_lists)
    set(options IS_EXE)
    set(oneValueArgs NAME)
    set(multiValueArgs LISTS_FILES)
    cmake_parse_arguments(
            DOODLE_GRPC_GENERATE
            "${options}"
            "${oneValueArgs}"
            "${multiValueArgs}"
            ${ARGN}
    )
    foreach (LIST ${DOODLE_GRPC_GENERATE_LISTS_FILES})
        file(REAL_PATH ${LIST} _PATH)
        get_filename_component(_NAME_WE ${_PATH} NAME_WE)
        get_filename_component(_NAME ${_PATH} NAME)
        get_filename_component(_DIRECTORY ${_PATH} DIRECTORY)
        # cmake_print_variables(_PATH _NAME_WE _NAME _DIRECTORY)

        # message("name: " ${LIST})
        list(APPEND
                _OUT
                ${CMAKE_CURRENT_LIST_DIR}/generate/rpc/${_NAME_WE}.pb.h
                ${CMAKE_CURRENT_LIST_DIR}/generate/rpc/${_NAME_WE}.pb.cc
                ${CMAKE_CURRENT_LIST_DIR}/generate/rpc/${_NAME_WE}.grpc.pb.h
                ${CMAKE_CURRENT_LIST_DIR}/generate/rpc/${_NAME_WE}.grpc.pb.cc)
        add_custom_command(
                OUTPUT
                ${CMAKE_CURRENT_LIST_DIR}/generate/rpc/${_NAME_WE}.pb.h
                ${CMAKE_CURRENT_LIST_DIR}/generate/rpc/${_NAME_WE}.pb.cc
                ${CMAKE_CURRENT_LIST_DIR}/generate/rpc/${_NAME_WE}.grpc.pb.h
                ${CMAKE_CURRENT_LIST_DIR}/generate/rpc/${_NAME_WE}.grpc.pb.cc
                COMMAND protobuf::protoc
                ARGS --proto_path=${Z_VCPKG_ROOT_DIR}/installed/${VCPKG_TARGET_TRIPLET}/include
                --proto_path=${_DIRECTORY}
                --cpp_out=dllexport_decl=DOODLELIB_API:${CMAKE_CURRENT_LIST_DIR}/generate/rpc
                --grpc_out=${CMAKE_CURRENT_LIST_DIR}/generate/rpc
                --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
                ${_NAME}
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                MAIN_DEPENDENCY ${_PATH}
        )
    endforeach ()

    set("${out_lists}"
            ${_OUT}
            PARENT_SCOPE)
endfunction()

function(doodle_sqlpp_generate out_lists)
    set(options IS_EXE)
    set(oneValueArgs NAME)
    set(multiValueArgs LISTS_FILES)
    cmake_parse_arguments(
            DOODLE_GRPC_GENERATE
            "${options}"
            "${oneValueArgs}"
            "${multiValueArgs}"
            ${ARGN}
    )
    foreach (LIST ${DOODLE_GRPC_GENERATE_LISTS_FILES})
        file(REAL_PATH ${LIST} _PATH)
        get_filename_component(_NAME_WE ${_PATH} NAME_WE)

        get_filename_component(_NAME ${_PATH} NAME)
        get_filename_component(_DIRECTORY ${_PATH} DIRECTORY)
        # cmake_print_variables(_PATH _NAME_WE _NAME _DIRECTORY)

        string(REGEX MATCH "[a-zA-Z]+"
                CLEAN_NAME ${_NAME_WE})
        # cmake_print_variables(LIST CLEAN_NAME)

        list(APPEND
                _OUT
                ${CMAKE_CURRENT_LIST_DIR}/generate/core/${CLEAN_NAME}_sql.h)
        if (EXISTS ${PROJECT_SOURCE_DIR}/.venv/Scripts/Activate.bat AND WIN32 )
            add_custom_command(
                    OUTPUT "${CMAKE_CURRENT_LIST_DIR}/generate/core/${CLEAN_NAME}_sql.h"
                    COMMAND ${PROJECT_SOURCE_DIR}/.venv/Scripts/Activate.bat
                    COMMAND py
                    ARGS $<TARGET_FILE:sqlpp11::ddl2cpp>
                    ${_PATH}
                    ${CMAKE_CURRENT_LIST_DIR}/generate/core/${CLEAN_NAME}_sql
                    doodle_database
                    MAIN_DEPENDENCY ${_PATH}
            )
        endif ()
    endforeach ()

    set("${out_lists}"
            ${_OUT}
            PARENT_SCOPE)
#     cmake_print_variables(_OUT)


endfunction()


function(add_doodle)
    set(options IS_EXE ADD_WIN32)
    set(oneValueArgs NAME)
    set(multiValueArgs LISTS_FILES)
    cmake_parse_arguments(
            ADD_DOODLE
            "${options}"
            "${oneValueArgs}"
            "${multiValueArgs}"
            ${ARGN}
    )
    if (ADD_DOODLE_IS_EXE)
        message("add exe name : " ${ADD_DOODLE_NAME})
        if (ADD_DOODLE_ADD_WIN32)
            add_executable(${ADD_DOODLE_NAME}
                    WIN32
                    ${ADD_DOODLE_LISTS_FILES})
        else ()
            add_executable(${ADD_DOODLE_NAME}
                    ${ADD_DOODLE_LISTS_FILES})
        endif ()
        target_compile_definitions(
                ${ADD_DOODLE_NAME}
                PUBLIC
                DOODLELIB_STATIC_DEFINE
        )
    else ()
        message("add lib name : " ${ADD_DOODLE_NAME})

        add_library(
                ${ADD_DOODLE_NAME}
                SHARED
                ${ADD_DOODLE_LISTS_FILES}
        )
        string(TOUPPER ${ADD_DOODLE_NAME} _UP_STR)
        set(MY_CUSTOM_CONTENT "")
        generate_export_header(${ADD_DOODLE_NAME}
                EXPORT_MACRO_NAME ${_UP_STR}_API
                CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)
    endif ()
    set_target_properties(${ADD_DOODLE_NAME}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    #设置导出符号
    include(GenerateExportHeader)

#     cmake_print_variables(CMAKE_CURRENT_LIST_DIR)

    #    #预编译头文件
    #    target_precompile_headers(${ADD_DOODLE_NAME}
    #            PRIVATE
    #            <doodle_lib/DoodleLib_pch.h>
    #            )
    target_link_libraries(${ADD_DOODLE_NAME} PUBLIC
            date::date
            date::date-tz
            #        SqlppSqlite
            DoodleLibResource
            # RTTR::Core
            cereal
            magic_enum::magic_enum
            ${Boost_LIBRARIES}
            Boost::locale
            Boost::filesystem
            nlohmann_json nlohmann_json::nlohmann_json
            ${OpenCV_LIBS}
            #        EnTT::EnTT
            #        Boost::locale
            #        Boost::filesystem
            #        Boost::log
            spdlog::spdlog
            unofficial::nana::nana
            #        $<IF:$<CONFIG:Debug>,${WX_based},${WX_base}>
            #        $<IF:$<CONFIG:Debug>,${WX_cored},${WX_core}>
            #        $<IF:$<CONFIG:Debug>,${WX_advd},${WX_adv}>

            csv
            Catch2::Catch2
            SqlppMySql #这个是我们自己寻找的mysql sqlpp连接器
            #        cppzmq
            #        cppzmq-static
            gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts gRPC::grpc++_reflection
            cryptopp-static
            PRIVATE
            ${DOODLE_MAR}
            )


    target_compile_definitions(${ADD_DOODLE_NAME}
            PUBLIC
            #            _WIN32_WINNT=0x0A00
            #            WIN32_LEAN_AND_MEAN
            $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
            #            PRIVATE
            #            _CRT_SECURE_NO_WARNINGS
            #            _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
            #            _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
            )
    target_compile_options(${ADD_DOODLE_NAME}
            PUBLIC
            /EHsc
            )

    target_include_directories(${ADD_DOODLE_NAME}
            PUBLIC
            ${CMAKE_CURRENT_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}/src
            )
    #    add_dependencies(
    #            ${ADD_DOODLE_NAME}
    #            cmrc_prj
    #            doxygen_css
    #            tzdata_prj
    #            windowsZones_prj
    #            csv_parser_prj
    #    )
endfunction()
