install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/maya
        DESTINATION plug
        COMPONENT maya_plug)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/4.25
        DESTINATION plug/uePlug
        COMPONENT ue4_plug
        PATTERN "Intermediate" EXCLUDE
        )
install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/4.26
        DESTINATION plug/uePlug
        COMPONENT ue4_plug
        PATTERN "Intermediate" EXCLUDE
        )
install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/4.27
        DESTINATION plug/uePlug
        COMPONENT ue4_plug
        PATTERN "Intermediate" EXCLUDE
        )
add_custom_target(doodle_ue4_27)
