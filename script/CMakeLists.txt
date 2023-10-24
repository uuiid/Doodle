install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/5.3/Plugins/Doodle/
        DESTINATION ue53_Plug
        COMPONENT ue4_plug
        PATTERN "Intermediate" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "Source" EXCLUDE
        )
install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/SideFX_Labs/
        DESTINATION SideFX_Labs
        COMPONENT ue4_plug
        PATTERN "Intermediate" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "Source" EXCLUDE
        )


install(CODE "
message(\"CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}\")
include(${PROJECT_SOURCE_DIR}/CMake/doodle_exe.cmake)
set(v_short ${v_short})
set(v_patch ${v_patch})
DOODLE_UE_PLUG(PLUG_JSON ${CMAKE_INSTALL_PREFIX}/ue53_Plug/Doodle.uplugin)
"
        COMPONENT ue4_plug)