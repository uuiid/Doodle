
# find_package(Boost COMPONENTS filesystem regex REQUIRED)

add_executable(doodleServer 
                DoodleServer_global.h
                main.cpp)

add_subdirectory(src)

target_include_directories(doodleServer PRIVATE ${Boost_INCLUDE_DIR})
target_include_directories(doodleServer
PUBLIC
${CMAKE_CURRENT_LIST_DIR})

#两个网络服务器库, 用一个
target_link_libraries(doodleServer PRIVATE cppnetlib-uri 
                        cppnetlib-server-parsers 
                        cppnetlib-client-connections)# boost网络基础库
target_link_libraries(doodleServer PRIVATE cppzmq cppzmq-static) # zmq消息库,主要

target_link_libraries(doodleServer PRIVATE date::date date::date-tz) #时间库
target_link_libraries(doodleServer PRIVATE nlohmann_json nlohmann_json::nlohmann_json) #json库

target_link_libraries(doodleServer PRIVATE Boost::filesystem Boost::regex Boost::iostreams)

target_compile_definitions(doodleServer PRIVATE _WIN32_WINNT=0x0A00)
#安装基本组件
install(TARGETS doodleServer
        DESTINATION bin)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:doodleServer>
        DESTINATION bin
        CONFIGURATIONS Debug)

