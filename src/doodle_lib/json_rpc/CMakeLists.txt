target_sources(doodle_lib
        PUBLIC
        parser_rpc.h
        server.h
        session_manager.h
        PRIVATE
        parser_rpc.cpp
        server.cpp
        session_manager.cpp

        )
