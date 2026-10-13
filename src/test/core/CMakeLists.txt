target_sources(test_main
        PRIVATE
        # 测试基建自测（<test/test_coro.h>）：改过那个头文件就先跑它
        coro_test_support.cpp
        data.cpp
        axis_angle.cpp
        depth_estimation.cpp
        multipart_body_test.cpp
        multipart_body_split_boundary_test.cpp
        http_listener_port_shm.cpp
        sqlite_fk_fix.cpp
        sqlite_rebuild.cpp
        sqlite_pragma.cpp
        sqlite_real_db.cpp
        sqlite_schema.cpp
        sqlite_conn_pragma.cpp
        sqlite_fk_semantics.cpp
        sqlite_not_found.cpp
        sqlite_upgrade.cpp
        ai_light_schema.cpp
        ai_light_state.cpp
        ai_light_stats.cpp
        ai_light_paths.cpp
        ai_light_json_validation.cpp
        ai_light_dispatch.cpp
        shot_render_light_paths.cpp
        url_sign.cpp
        seedance2_inference.cpp
        seedance2_task_model.cpp
        seedance2_task_pipeline.cpp
)

 
