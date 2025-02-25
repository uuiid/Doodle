target_sources(doodle_core
        PUBLIC
        boost_uuid_warp.h
        json_warp.h
        enum_template_tool.h
        boost_icl_warp.h
        boost_fmt_icl.h
        boost_fmt_rational.h
        boost_fmt_asio.h
        boost_fmt_url.h
        boost_fmt_string.h
        boost_fmt_beast.h
        boost_locale_warp.h

)
add_subdirectory(detail)
