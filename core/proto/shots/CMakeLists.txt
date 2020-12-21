
# add_custom_command(OUTPUT
#     ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.pb.h
#     ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.grpc.pb.h
#     ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.grpc.pb.cc
#     ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.pb.cc
#     COMMAND protobuf::protoc -I=${CMAKE_CURRENT_LIST_DIR} --grpc_out=. --cpp_out=. --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin> epsidoes.proto
#     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#     DEPENDS protobuf::protoc
#     VERBATIM
# )
# set_source_files_properties(
#     ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.pb.h
#     ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.grpc.pb.h
#     ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.grpc.pb.cc
#     ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.pb.cc
#     DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#     TARGET_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#     PROPERTIES GENERATED TRUE
# )
#gRPC::grpc_cpp_plugin
add_custom_target(generated_shots_server
    COMMAND protobuf::protoc -I=${CMAKE_CURRENT_LIST_DIR} --grpc_out=. --cpp_out=. --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin> epsidoes.proto
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS protobuf::protoc
    VERBATIM)
add_dependencies(core generated_shots_server)
target_include_directories(core PUBLIC  ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB PROTO_HEADER CONFIGURE_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/*.h)
file(GLOB PROTO_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/*.cc)

# target_sources(core PUBLIC
# ${PROTO_HEADER})

# target_sources(core PRIVATE
# ${PROTO_SOURCES})

# target_sources(core PUBLIC
# ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.pb.h
# ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.grpc.pb.h)

# target_sources(core PRIVATE
# ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.grpc.pb.cc
# ${CMAKE_CURRENT_BINARY_DIR}/epsidoes.pb.cc)


# 导出宏的样式
# --cpp_out=dllexport_decl=MY_EXPORT:.
# protobuf_generate(DOODLE_SHOTS_SOURCES DOODLE_SHOTS_HANDLERS episodes.proto)