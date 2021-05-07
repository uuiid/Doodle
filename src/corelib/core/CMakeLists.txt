target_sources(
        core
        PUBLIC
        coreset.h
        coresql.h
        ToolsSetting.h
        Ue4Setting.h
        Name.h
        MetadataSet.h

        PRIVATE
        coreset.cpp
        coresql.cpp
        Ue4Setting.cpp
        Name.cpp
        MetadataSet.cpp
)
