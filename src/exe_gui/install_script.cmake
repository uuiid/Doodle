

# 修复dll
# include(BundleUtilities)
# fixup_bundle("${DOODLE_MAIN_EXE}" "" "")

# 生成授权文件
execute_process(
        COMMAND ${DOODLE_GENERATE_TOKEN_RUN} ${CMAKE_INSTALL_PREFIX}/bin/
        WORKING_DIRECTORY @PROJECT_BINARY_DIR@/bin
)
