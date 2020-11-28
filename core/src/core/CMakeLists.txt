target_sources(
        core
        PUBLIC
        coreDataManager.h
        coreset.h
        coresql.h
        coresqldata.h

)
target_sources(
        core
        PRIVATE
        coreDataManager.cpp
        coreset.cpp
        coresql.cpp
        coresqldata.cpp
)