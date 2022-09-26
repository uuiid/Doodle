#target_sources(doodle_lib
#        PRIVATE
#        im-neo-sequencer/imgui_neo_internal.cpp
#        im-neo-sequencer/imgui_neo_internal.h
#        im-neo-sequencer/imgui_neo_sequencer.cpp
#        im-neo-sequencer/imgui_neo_sequencer.h
#        )

include(ExternalProject)
include(FetchContent)

FetchContent_Declare(
        IconFontCppHeaders
        GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
        GIT_TAG main
)
FetchContent_MakeAvailable(IconFontCppHeaders)

target_include_directories(
        doodle_lib
        PRIVATE ${iconfontcppheaders_SOURCE_DIR}
)

message("IconFontCppHeaders_SOURCE_DIR = ${iconfontcppheaders_SOURCE_DIR}")
