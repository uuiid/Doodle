install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/5.5/Plugins/Doodle/
        DESTINATION ue55_Plug
        PATTERN "Intermediate" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "Source" EXCLUDE
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/SideFX_Labs/
        DESTINATION SideFX_Labs
        PATTERN "Intermediate" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "Source" EXCLUDE
)


install(CODE "
include(${PROJECT_SOURCE_DIR}/CMake/doodle_exe.cmake)
set(v_short ${v_short})
set(v_patch ${v_patch})
DOODLE_UE_PLUG(PLUG_JSON \${CMAKE_INSTALL_PREFIX}/ue55_Plug/Doodle.uplugin)
"
        COMPONENT ue4_plug)