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
        if (EXISTS ${PROJECT_SOURCE_DIR}/.venv/Scripts/Activate.bat AND WIN32)
            file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CLEAN_NAME}_sql.cmd
                    CONTENT "call ${PROJECT_SOURCE_DIR}/.venv/Scripts/Activate.bat
python $<TARGET_FILE:sqlpp11::ddl2cpp> ${_PATH} ${CMAKE_CURRENT_LIST_DIR}/generate/core/${CLEAN_NAME}_sql doodle_database")
            add_custom_command(
                    OUTPUT "${CMAKE_CURRENT_LIST_DIR}/generate/core/${CLEAN_NAME}_sql.h"
                    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${CLEAN_NAME}_sql.cmd
                    MAIN_DEPENDENCY ${_PATH}
            )
        endif ()
    endforeach ()

    set("${out_lists}"
            ${_OUT}
            PARENT_SCOPE)
    #     cmake_print_variables(_OUT)


endfunction()


function(doodle_install_code_wix dir)
    install(CODE
            "
            execute_process(
                    COMMAND \${CMAKE_COMMAND} -E make_directory \${CMAKE_INSTALL_PREFIX}/${dir}
                    COMMAND py ${PROJECT_SOURCE_DIR}/src/install_wix/fix_main_wxs.py
                    --input_dir \${CMAKE_INSTALL_PREFIX}/${dir}

                    WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
            )
            execute_process(
                    COMMAND \"$<TARGET_FILE:wix_candle>\"
                    -nologo
                    -arch x64
                    -d${dir}_dir=\${CMAKE_INSTALL_PREFIX}/${dir}
                    -out \"\${CMAKE_INSTALL_PREFIX}/wix/${dir}.wixobj\"
                    \"-I\${CMAKE_INSTALL_PREFIX}/wix\"
                    \"-I\${CMAKE_INSTALL_PREFIX} \"
                    \${CMAKE_INSTALL_PREFIX}/wix/${dir}.wxs

                    WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX}
            )
"
            COMPONENT exe_com
            )
endfunction()