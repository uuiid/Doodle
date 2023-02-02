target_sources(doodle_lib
        PUBLIC
        main_facet.h
        rpc_server_facet.h
        create_move_facet.h

        PRIVATE
        main_facet.cpp
        rpc_server_facet.cpp
        create_move_facet.cpp
        )
