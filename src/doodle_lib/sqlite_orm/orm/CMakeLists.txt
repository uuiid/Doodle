target_sources(
        doodle_lib
        PUBLIC
        storage.h
        fwd.h
        select.h
        update.h
        orm.h   
        sqlite_statement.h
        
        storage_impl.h
        select_impl.h

        PRIVATE
        storage.cpp
        column_operations.cpp
)