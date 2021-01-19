add_subdirectory(model)
add_subdirectory(view)

target_sources(
        doodle
        PUBLIC
        AssScreenShotWidght.h
        assWidget.h
)
target_sources(
        doodle
        PRIVATE
        AssScreenShotWidght.cpp
        assWidget.cpp
)