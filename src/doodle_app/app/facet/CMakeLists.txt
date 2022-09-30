target_sources(
        doodle_app
        PUBLIC
        json_rpc_facet.h
        gui_facet.h

        PRIVATE
        json_rpc_facet.cpp
        gui_facet.cpp
)
