target_sources(doodle_core
        PUBLIC
        parser_rpc.h
        rpc_client.h
        rpc_reply.h
        rpc_server.h
        server.h

        session.h
        rpc_request.h

        PRIVATE
        parser_rpc.cpp
        rpc_client.cpp
        rpc_reply.cpp
        rpc_server.cpp
        server.cpp
        session.cpp

        rpc_request.cpp
        )
