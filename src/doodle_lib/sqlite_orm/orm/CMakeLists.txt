target_sources(
        doodle_lib
        PUBLIC
        storage.h
        fwd.h
        select.h
        storage_impl.h

        PRIVATE
        storage.cpp
        column_operations.cpp
)