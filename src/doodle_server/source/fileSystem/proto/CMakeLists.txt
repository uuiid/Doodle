

add_custom_target(generated_file_server
COMMAND protobuf::protoc -I=${CMAKE_CURRENT_LIST_DIR} -I=$ENV{VCPKG_MYROOT}/installed/x64-windows/include --grpc_out=. --cpp_out=. --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin> fileSystem.proto
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
DEPENDS protobuf::protoc
VERBATIM
)

# add_custom_target(generated_file_server
# COMMAND protobuf::protoc -I=${CMAKE_CURRENT_LIST_DIR} -I=$ENV{VCPKG_MYROOT}/installed/x64-windows/include --cpp_out=.  doodle_test.proto
# WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
# DEPENDS protobuf::protoc
# VERBATIM
# )

add_dependencies(doodleServer generated_file_server)

file(GLOB PROTO_HEADER CONFIGURE_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/*.h)
file(GLOB PROTO_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/*.cc)

target_include_directories(doodleServer 
PRIVATE 
${CMAKE_CURRENT_BINARY_DIR})

target_sources(doodleServer
  PUBLIC
  # ${PROTO_HEADER}
  # ${CMAKE_CURRENT_BINARY_DIR}/fileSystem.grpc.pb.h
  # ${CMAKE_CURRENT_BINARY_DIR}/doodle_test.pb.h
)
target_sources(doodleServer
  PRIVATE
  # ${PROTO_SOURCES}
  # ${CMAKE_CURRENT_BINARY_DIR}/fileSystem.grpc.pb.cc
  # ${CMAKE_CURRENT_BINARY_DIR}/doodle_test.pb.cc
)