message("CPACK_TOPLEVEL_DIRECTORY=${CPACK_TOPLEVEL_DIRECTORY}")
message("DOODLE_7z_NAME=${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}.7z")
message("CPACK_PACKAGE_DIRECTORY=${CPACK_PACKAGE_DIRECTORY}")
message("CPACK_PACKAGE_INSTALL_DIRECTORY=${CPACK_PACKAGE_INSTALL_DIRECTORY}")

set(DOODLE_7z_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME})
execute_process(
        COMMAND 7z a "${CPACK_TOPLEVEL_DIRECTORY}/${DOODLE_7z_NAME}.7z" "${CPACK_TOPLEVEL_DIRECTORY}/${DOODLE_7z_NAME}" -mx2 -mmt8
        COMMAND_ECHO STDOUT
)
execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CPACK_TOPLEVEL_DIRECTORY}/${DOODLE_7z_NAME}.7z ${CPACK_PACKAGE_DIRECTORY}/${DOODLE_7z_NAME}.7z
)
