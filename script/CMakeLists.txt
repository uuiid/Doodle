
if (DEFINED Ue4_ROOT_DIR)
    # 定义安装逻辑
    add_custom_target(doodle_ue4_27
#            COMMAND ${Ue4_ROOT_DIR}/Engine/Build/BatchFiles/RunUAT.bat BuildPlugin -Plugin=${CMAKE_CURRENT_LIST_DIR}/uePlug/Doodle/Doodle.uplugin -TargetPlatforms=Win64 -Package=${CMAKE_CURRENT_LIST_DIR}/uePlug/4.27/Plugins/Doodle -VS2019
            COMMAND ${CMAKE_COMMAND} -E echo "没有构建, 请手动构建"
            BYPRODUCTS ${CMAKE_CURRENT_LIST_DIR}/uePlug/4.27/Plugins/Doodle
            )
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/4.25/Plugins/Doodle/
            DESTINATION ue4.25_Plug
            COMPONENT ue4_plug
            PATTERN "Intermediate" EXCLUDE
            )
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/4.26/Plugins/Doodle/
            DESTINATION ue4.26_Plug
            COMPONENT ue4_plug
            PATTERN "Intermediate" EXCLUDE
            )
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/4.27/Plugins/Doodle/
            DESTINATION ue4.27_Plug
            COMPONENT ue4_plug
            PATTERN "Intermediate" EXCLUDE
            )
endif ()
