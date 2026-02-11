add_executable(
        doodle_kitsu_supplement
        kitsu_supplement_main.cpp
        ../DoodleExe.manifest
        ../icon.ico
        ../win_exe.rc
)

set_target_properties(doodle_kitsu_supplement
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(doodle_kitsu_supplement
        PUBLIC
        mimalloc
        doodle_lib
)



# 调整动态库导入模块顺序
function(dow_minject)
    include(ExternalProject)
    ExternalProject_Add(
            minject
            GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
            GIT_TAG dev
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
    )
    set(MIMALLOC_EXE ${CMAKE_BINARY_DIR}/_deps/minject-src/bin/minject.exe PARENT_SCOPE)
endfunction()
dow_minject()

# add_custom_command(
#         TARGET doodle_kitsu_supplement
#         POST_BUILD
#         COMMAND ${MIMALLOC_EXE} --force --inplace $<TARGET_FILE:doodle_kitsu_supplement>
# )
# add_custom_target(genexdebug2
#         COMMAND ${CMAKE_COMMAND} -E echo "${MIMALLOC_EXE} --force --inplace $<TARGET_FILE:doodle_kitsu_supplement>")

# 寻找 crashpad_handler.exe 本身
# debug  build\Ninja_debug\vcpkg_installed\x64-windows\debug\tools\
# release  build\Ninja_debug\vcpkg_installed\x64-windows\tools\
find_program(
        crashpad_handler_exe
        NAMES crashpad_handler.exe
        HINTS
        $<$<CONFIG:Debug>:${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/debug/tools>
        $<$<CONFIG:Release>:${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/tools>
)

# 复制动态加载库 E:\Doodle\vcpkg\buildtrees\libtorch\x64-windows-dbg\bin\
add_custom_command(
        TARGET doodle_kitsu_supplement
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/vcpkg/buildtrees/libtorch/x64-windows-$<IF:$<CONFIG:Debug,RelWithDebInfo>,dbg,rel>/bin/nvfuser_codegen.dll $<TARGET_FILE_DIR:doodle_kitsu_supplement>
        # COMMAND ${CMAKE_COMMAND} -E echo "${PROJECT_SOURCE_DIR}/vcpkg/buildtrees/libtorch/x64-windows-$<IF:$<CONFIG:Debug,RelWithDebInfo>,dbg,rel>/bin/nvfuser_codegen.dll" 
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${crashpad_handler_exe} $<TARGET_FILE_DIR:doodle_kitsu_supplement>

)
# 如果是debug, 需要将 zlibd1.dll 复制一份, 命名为 zlib1.dll
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
                TARGET doodle_kitsu_supplement
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different  $<TARGET_FILE_DIR:doodle_kitsu_supplement>/zlibd1.dll $<TARGET_FILE_DIR:doodle_kitsu_supplement>/zlib1.dll
        )
endif()

# 添加生成 PDB 文件
target_compile_options(
        doodle_kitsu_supplement
        PRIVATE
        "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RELEASE>>:/Zi>"
)
target_link_options(
        doodle_kitsu_supplement 
        PRIVATE 
        "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RELEASE>>:/DEBUG>"
        "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RELEASE>>:/OPT:REF>"
        "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RELEASE>>:/OPT:ICF>"
)
 

# 定义安装逻辑
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
include(InstallRequiredSystemLibraries)

install(TARGETS doodle_kitsu_supplement
        RUNTIME_DEPENDENCIES
        DIRECTORIES
        $<TARGET_FILE_DIR:Fbx::Fbx>
        $ENV{CUDA_PATH}/bin
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)



find_file(libomp140_x86_64_dll
        NAMES
        libomp140.x86_64.dll
        HINTS
        ${MSVC_REDIST_DIR}/debug_nonredist/x64/Microsoft.VC143.OpenMP.LLVM
        DOC
        "libomp140.x86_64.dll"
        REQUIRED
        NO_DEFAULT_PATH
)
find_file(vcomp140_dll
        NAMES
        vcomp140.dll
        HINTS
        ${MSVC_REDIST_DIR}/x64/Microsoft.VC143.OpenMP
        DOC
        "vcomp140.dll"
        REQUIRED
        NO_DEFAULT_PATH
)

install(FILES ${PROJECT_SOURCE_DIR}/vcpkg/buildtrees/libtorch/x64-windows-$<IF:$<CONFIG:Debug,RelWithDebInfo>,dbg,rel>/bin/nvfuser_codegen.dll 
        ${libomp140_x86_64_dll}
        ${vcomp140_dll}
        ${PROJECT_BINARY_DIR}/bin/libmp3lame.dll
        $<TARGET_FILE:7zip::7zip>
        DESTINATION bin
)
install(FILES ${crashpad_handler_exe}
        DESTINATION bin
)
set(DOODLE_INSTALL_IGNORE_ITEM
        doodle_maya_exe_2018.exe
        doodle_maya_exe_2019.exe
        doodle_maya_exe_2020.exe
)

list(JOIN DOODLE_INSTALL_IGNORE_ITEM " " DOODLE_INSTALL_IGNORE_ITEM)
# 最后实现camke修复
install(CODE
        "
# 修复dll
include(BundleUtilities)
fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/bin/$<TARGET_NAME:doodle_kitsu_supplement>.exe\" \"\" \"\${CMAKE_INSTALL_PREFIX}/bin\"
        IGNORE_ITEM
        ${DOODLE_INSTALL_IGNORE_ITEM}
)

# 生成授权文件
execute_process(
        COMMAND $<TARGET_FILE:generate_token> \${CMAKE_INSTALL_PREFIX}/bin/
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)
        "
)
