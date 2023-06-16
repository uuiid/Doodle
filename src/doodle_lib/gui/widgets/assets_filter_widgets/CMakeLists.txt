target_sources(
        doodle_lib
        PUBLIC
        name_filter_factory.h
        filter_base.h
        filter_factory_base.h
        filter_factory_template.h
        name_filter.h
        assets_tree.h

        PRIVATE
        name_filter_factory.cpp
        filter_factory_base.cpp
        filter_base.cpp
        name_filter.cpp
        assets_tree.cpp
)
