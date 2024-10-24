install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/5.4/Plugins/Doodle/
        DESTINATION ue54_Plug
        PATTERN "Intermediate" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "Source" EXCLUDE
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/5.4/Plugins/UnrealEngine5VLC/
        DESTINATION UnrealEngine5VLC
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
DOODLE_UE_PLUG(PLUG_JSON \${CMAKE_INSTALL_PREFIX}/ue54_Plug/Doodle.uplugin)
"
        COMPONENT ue4_plug)