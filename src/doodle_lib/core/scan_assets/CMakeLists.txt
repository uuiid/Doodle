target_sources(doodle_lib
        PUBLIC
        base.h
        scene_scan_category.h
        prop_scan_category.h
        character_scan_category.h
        scan_category_service.h

        PRIVATE
        base.cpp
        scene_scan_category.cpp
        prop_scan_category.cpp
        character_scan_category.cpp
        scan_category_service.cpp
)
