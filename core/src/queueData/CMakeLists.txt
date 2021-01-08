target_sources(
        core
        PUBLIC
        queueData.h
        queueManager.h
)
target_sources(
        core
        PRIVATE
        queueData.cpp
        queueManager.cpp

)