
if(DEFINED Ue4_ROOT_DIR)
        # 定义安装逻辑
        add_custom_target(doodle_ue4_27

                # COMMAND ${Ue4_ROOT_DIR}/Engine/Build/BatchFiles/RunUAT.bat BuildPlugin -Plugin=${CMAKE_CURRENT_LIST_DIR}/uePlug/Doodle/Doodle.uplugin -TargetPlatforms=Win64 -Package=${CMAKE_CURRENT_LIST_DIR}/uePlug/4.27/Plugins/Doodle -VS2019
                COMMAND ${CMAKE_COMMAND} -E echo "没有构建, 请手动构建"

                # BYPRODUCTS ${CMAKE_CURRENT_LIST_DIR}/uePlug/4.27/Plugins/Doodle
                )
        message("ue4 root ${Ue4_ROOT_DIR}")
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/4.27/Plugins/Doodle/
                DESTINATION ue427_Plug
                COMPONENT ue4_plug
                PATTERN "Intermediate" EXCLUDE
                PATTERN "*.pdb" EXCLUDE
                PATTERN "Source" EXCLUDE
                )
        doodle_install_code_wix(ue427_Plug ue4_plug)

        install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/5.0/Plugins/Doodle/
                DESTINATION ue50_Plug
                COMPONENT ue4_plug
                PATTERN "Intermediate" EXCLUDE
                PATTERN "*.pdb" EXCLUDE
                PATTERN "Source" EXCLUDE
                )
        doodle_install_code_wix(ue50_Plug ue4_plug)

        install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/5.1/Plugins/Doodle/
                DESTINATION ue51_Plug
                COMPONENT ue4_plug
                PATTERN "Intermediate" EXCLUDE
                PATTERN "*.pdb" EXCLUDE
                PATTERN "Source" EXCLUDE
                )
        doodle_install_code_wix(ue51_Plug ue4_plug)

        install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug/SideFX_Labs/
                DESTINATION SideFX_Labs
                COMPONENT ue4_plug
                PATTERN "Intermediate" EXCLUDE
                )
        doodle_install_code_wix(SideFX_Labs ue4_plug)
        install(FILES ue_install.wxs
                DESTINATION wix
                COMPONENT ue4_plug)
endif()
