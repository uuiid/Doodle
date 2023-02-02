

# 修复dll
include(BundleUtilities)
fixup_bundle("${DOODLE_MAIN_EXE}" "" "")
# 添加安装配置
configure_file(@CMAKE_CURRENT_LIST_DIR@/install.wxs ${CMAKE_INSTALL_PREFIX}/wix/doodle_gui_exe.wxs)

# 生成授权文件

execute_process(
        COMMAND ${DOODLE_GENERATE_TOKEN_RUN} ${CMAKE_INSTALL_PREFIX}/bin/
        WORKING_DIRECTORY @PROJECT_BINARY_DIR@/bin
)
