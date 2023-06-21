
message("ue4 root ${Ue4_ROOT_DIR}")
install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/4.27/Plugins/Doodle/
        DESTINATION ue427_Plug
        COMPONENT ue4_plug
        )
install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/5.2/Plugins/Doodle/
        DESTINATION ue52_Plug
        COMPONENT ue4_plug
        )
install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/SideFX_Labs/
        DESTINATION SideFX_Labs
        COMPONENT ue4_plug
        PATTERN "Intermediate" EXCLUDE
        )


install(CODE "
message(${CMAKE_MODULE_PATH})
include(${PROJECT_SOURCE_DIR}/CMake/doodle_exe)
DOODLE_UE_PLUG(PLUG_JSON ${CMAKE_INSTALL_PREFIX}/ue427_Plug/Doodle.uplugin)
DOODLE_UE_PLUG(PLUG_JSON ${CMAKE_INSTALL_PREFIX}/ue52_Plug/Doodle.uplugin)
"
        COMPONENT ue4_plug)