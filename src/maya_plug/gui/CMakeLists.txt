target_sources(doodle_maya_lib
        PUBLIC
        maya_layout.h
        maya_plug_app.h

        PRIVATE
        maya_layout.cpp
        maya_plug_app.cpp
        )
add_subdirectory(action)
