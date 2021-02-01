
# find_package(Boost COMPONENTS filesystem regex REQUIRED)

# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
add_executable(doodleServer  
                DoodleServer_global.h
                main.cpp)

add_subdirectory(source)
add_subdirectory(share)


target_include_directories(doodleServer PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(doodleServer PRIVATE logger)
target_link_libraries(doodleServer PRIVATE cppzmq cppzmq-static) # zmq消息库,主要

target_link_libraries(doodleServer PRIVATE date::date date::date-tz) #时间库
target_link_libraries(doodleServer PRIVATE nlohmann_json nlohmann_json::nlohmann_json) #json库
target_link_libraries(doodleServer PRIVATE magic_enum::magic_enum)
target_link_libraries(doodleServer PRIVATE Boost::filesystem Boost::regex Boost::iostreams)


# target_link_libraries(doodleServer PRIVATE gRPC::grpc++_alts)
# target_link_libraries(doodleServer PRIVATE gRPC::grpc++)
target_compile_definitions(doodleServer PRIVATE _WIN32_WINNT=0x0A00)
# target_link_options(doodleServer PRIVATE "/ENTRY:mainCRTStartup")
# target_compile_definitions(doodleServer PRIVATE _WINDOWS)
# set_target_properties(doodleServer PROPERTIES WIN32_EXECUTABLE FALSE)
#安装基本组件
install(TARGETS doodleServer
        DESTINATION bin)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:doodleServer>
        DESTINATION bin
        CONFIGURATIONS Debug)

