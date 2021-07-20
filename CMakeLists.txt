cmake_minimum_required(VERSION 3.14)

project(Doodle
        VERSION 2.7.32.2
        DESCRIPTION "动画制作中的一些辅助工具"
        LANGUAGES CXX
        HOMEPAGE_URL https://github.com/uuiid/Doodle)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
#设置qt自动参数
#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)


add_compile_definitions(QT_NO_KEYWORDS UNICODE _UNICODE NOMINMAX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#添加模块文件
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")


#使用中文
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

#添加选项
option(RUN_QT_TOOL "run qt windeployqt" OFF)
option(RUN_fixup "run fixup " OFF)
option(RUN_COPY "run copy tools" OFF)


#配置版本文件
configure_file(DoodleConfig.h.in ${CMAKE_BINARY_DIR}/configure/DoodleConfig.h)
include_directories(${CMAKE_BINARY_DIR}/configure)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# #添加qt路径
# list(APPEND CMAKE_PREFIX_PATH F:/Qt/5.12.9/msvc2017_64)

#首先搜索我们的自定义函数
list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/CMake)
include(CMakeRC)

#寻找一些外部库
#    find_package(unofficial-nana CONFIG REQUIRED)
#    target_link_libraries(main PRIVATE unofficial::nana::nana)
#
#find_package(Qt5 CONFIG COMPONENTS Core Widgets Gui LinguistTools  REQUIRED)
#GUI 库
find_package(wxWidgets REQUIRED)
find_package(unofficial-nana CONFIG REQUIRED)

find_package(Boost 1.75 REQUIRED COMPONENTS filesystem date_time  log locale iostreams)
find_package(EnTT CONFIG REQUIRED)
# find_package(CURL REQUIRED)
find_package(Sqlpp11 REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)
#find_package(rttr CONFIG REQUIRED)
find_library(DOODLE_MAR mpr.lib REQUIRED)
# find_package(pugixml CONFIG REQUIRED)

find_package(Doxygen REQUIRED)

# 寻找opencv
find_package(OpenCV CONFIG REQUIRED)

#这两个是我自己写的寻找库的方法
#find_package(SqlppSqlite REQUIRED)
find_package(SqlppMySql REQUIRED)

#仅头库
find_package(magic_enum CONFIG REQUIRED)
#find_package(jsoncpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(date CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)
#服务器库
#应该会用其中一个
#find_package(cppzmq CONFIG REQUIRED)

#find_package(cppnetlib CONFIG REQUIRED)
#find_package(civetweb CONFIG REQUIRED)

#google序列化库
# find_package(protobuf CONFIG REQUIRED)
# target_link_libraries(main 
# PRIVATE protobuf::libprotoc 
# protobuf::libprotobuf
# protobuf::libprotobuf-lite)
#Google rpc库
#find_package(gRPC CONFIG REQUIRED)
## Note: 8 target(s) were omitted.
#target_link_libraries(main PRIVATE gRPC::gpr gRPC::upb gRPC::grpc gRPC::grpc++)
#
#find_package(modules CONFIG REQUIRED)
#target_link_libraries(main PRIVATE re2::re2 c-ares::cares)
find_package(gRPC CONFIG REQUIRED)
#target_link_libraries(main PRIVATE gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts)
#target_link_libraries(main PRIVATE re2::re2 c-ares::cares)

find_package(Autodesk_FBX 2020 REQUIRED)
#搜索qt 安装辅助程序
# find_program(WINQT qt5::windeployqt REQUIRED)

#下载自定义嵌入文件
include(ExternalProject)
ExternalProject_Add(cmrc_prj
        GIT_REPOSITORY https://github.com/vector-of-bool/cmrc.git
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/cmrc_prj-prefix/src/cmrc_prj/CMakeRC.cmake ${PROJECT_SOURCE_DIR}/CMake
        TEST_COMMAND ""
        )
#添加doxygen自定义css
ExternalProject_Add(doxygen_css
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_TAG origin/main
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/doxygen_css-prefix/src/doxygen_css/doxygen-awesome.css ${PROJECT_SOURCE_DIR}/doc/css
        #        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/doxygen-awesome-css-prefix/src/doxygen-awesome-css/doxygen-awesome-sidebar-only.css ${PROJECT_SOURCE_DIR}/doc/css
        TEST_COMMAND ""
        )
ExternalProject_Add(tzdata_prj
        URL https://www.iana.org/time-zones/repository/tzdata-latest.tar.gz
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/tzdata_prj-prefix/src/tzdata_prj ${CMAKE_BINARY_DIR}/bin/tzdata
        TEST_COMMAND ""
        )

ExternalProject_Add(windowsZones_prj
        URL https://raw.githubusercontent.com/unicode-org/cldr/master/common/supplemental/windowsZones.xml
#        DOWNLOAD_NAME windowsZones.xml
        DOWNLOAD_NO_EXTRACT TRUE
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/windowsZones_prj-prefix/src/windowsZones.xml ${CMAKE_BINARY_DIR}/bin/tzdata/windowsZones.xml
        TEST_COMMAND ""
        )
#添加文档生成
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
set(DOXYGEN_WARNINGS NO)
set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
set(DOXYGEN_WARN_IF_DOC_ERROR NO)
set(DOXYGEN_OUTPUT_LANGUAGE Chinese)
set(DOXYGEN_IMAGE_PATH ${PROJECT_SOURCE_DIR}/doc)
set(DOXYGEN_EXCLUDE_PATTERNS
        */.idea
        */.vs
        */b_Ninja_VS*
        */cmake-build*
        */venv)
set(DOXYGEN_GENERATE_TREEVIEW YES)
set(DOXYGEN_HTML_EXTRA_STYLESHEET ${PROJECT_SOURCE_DIR}/doc/css/doxygen-awesome.css)
doxygen_add_docs(Doxygen
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/doc
        )

add_subdirectory(src)

enable_testing()
add_subdirectory(test)
add_subdirectory(tools)
#=====================安装打包过程===============================
# list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)
# include(mypack)


#====================一些非qt组件的搜索和安装=============

if (RUN_fixup)
    #第一种搜索方式
    set(CMAKE_INSTALL_UCRT_LIBRARIES "C:/Program Files (x86)/Windows Kits/10/Redist/ucrt/DLLs/x64")
    include(InstallRequiredSystemLibraries)
endif (RUN_fixup)

list(APPEND DIRS ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin)


if (RUN_COPY)
    # #复制资源
    # install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/
    # DESTINATION resource)
    # #复制tool
    # install(DIRECTORY ${CMAKE_SOURCE_DIR}/tools/
    # DESTINATION tools)
    # #复制我自己写的一些工具
    # install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/Cmd_tool
    # DESTINATION tools)
    # #复制插件
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/maya
            DESTINATION plug)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug
            DESTINATION plug)
endif (RUN_COPY)


if (RUN_fixup)
    #最后实现camke修复
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"$<TARGET_FILE:DoodleExe>\"   \"\"   \"${DIRS}\")
        ")
endif (RUN_fixup)


#install(DIRECTORY ${CMAKE_BINARY_DIR}/plug/ DESTINATION plug)
install(DIRECTORY ${CMAKE_BINARY_DIR}/bin/ DESTINATION bin
        #PATTERN "*.manifest" EXCLUDE
        )


set(CPACK_GENERATOR "WIX")
set(CPACK_PACKAGE_EXECUTABLES DoodleExe;Doodle)
set(CPACK_CREATE_DESKTOP_LINKS DoodleExe)
set(CPACK_PACKAGE_VENDOR uuiid)
set(CPACK_PACKAGE_INSTALL_DIRECTORY uuiid)
set(CPACK_WIX_UPGRADE_GUID 96068D94-08C8-412B-9292-6DE8453E61B3)
# set(CPACK_WIX_PRODUCT_ICON ${CMAKE_CURRENT_LIST_DIR}/resource/inco/inco128_128.png)
# set(CPACK_PACKAGE_ICON ${CMAKE_CURRENT_LIST_DIR}/resource/inco/inco128_128.png)
# set(CPACK_WIX_CULTURES zh_CN)
include(CPack REQUIRED)

add_custom_target(cpack_${PROJECT_NAME}
        COMMAND ${CMAKE_CPACK_COMMAND} --build ${PROJECT_BINARY_DIR} --target package)
