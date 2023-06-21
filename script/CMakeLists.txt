
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

DOODLE_UE_PLUG(PLUG_JSON ${PROJECT_SOURCE_DIR}/script/uePlug/4.27/Plugins/Doodle/Doodle.uplugin)
DOODLE_UE_PLUG(PLUG_JSON ${PROJECT_SOURCE_DIR}/script/uePlug/5.2/Plugins/Doodle/Doodle.uplugin)