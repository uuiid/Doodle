add_subdirectory(model)
add_subdirectory(view)

target_sources(
        doodle
        PUBLIC
        assWidget.h
)
target_sources(
        doodle
        PRIVATE
        assWidget.cpp
)