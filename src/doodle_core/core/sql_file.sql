create table if not exists context
(
    id        integer primary key,
    com_hash  integer,
    json_data text
);
create index if not exists context_index_id on context (id);
create unique index if not exists ctx_hash_index on context (com_hash);
create table if not exists entity
(
    id          integer primary key,
    uuid_data   text,
    update_time datetime default CURRENT_TIMESTAMP not null
);
create index if not exists entity_index on entity (id);
create table if not exists com_entity
(
    id        integer
        primary key,
    entity_id integer,
    com_hash  integer,
    json_data text,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists com_entity_index on com_entity (id);
create index if not exists com_entity_index_hash on com_entity (com_hash);
create trigger if not exists UpdataLastTime_
    AFTER
        UPDATE OF json_data
    ON com_entity
begin
    update entity
    set update_time = CURRENT_TIMESTAMP
    where id = old.entity_id;
end;

create table if not exists doodle_info
(
    version_major integer not null,
    version_minor integer not null
);
create table if not exists usertab
(
    id               integer
        primary key,
    entity_id        integer,

    user_name        text             not null,
    permission_group bigint default 0 not null
);

create index if not exists usertab_id_index on usertab (id);

CREATE TABLE IF NOT EXISTS work_task_info
(
    id         integer
        primary key,
    entity_id  integer,

    user_id    text,
    task_name  text,
    region     text,
    abstract   text,
    time_point datetime,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists work_task_info_index on work_task_info (id);
create index if not exists work_task_info_index2 on work_task_info (entity_id);

CREATE TABLE IF NOT EXISTS episodes
(
    id        integer
        primary key,
    entity_id integer,

    eps       integer,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists episodes_index on episodes (id);
create index if not exists episodes_index2 on episodes (entity_id);

CREATE TABLE IF NOT EXISTS shot
(
    id        integer
        primary key,
    entity_id integer,

    shot_int  integer,
    shot_ab   text,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists shot_index on shot (id);
create index if not exists shot_index2 on shot (entity_id);

CREATE TABLE IF NOT EXISTS redirection_path_info
(
    id                    integer
        primary key,
    entity_id             integer,

    redirection_file_name text,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists redirection_path_info_index on redirection_path_info (id);
create index if not exists redirection_path_info_index2 on redirection_path_info (entity_id);

CREATE TABLE IF NOT EXISTS rpi_search_path
(
    id               integer
        primary key,
    entity_id        integer,

    redirection_path text,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists rpi_search_path_index on rpi_search_path (id);
create index if not exists rpi_search_path_index2 on rpi_search_path (entity_id);

CREATE TABLE IF NOT EXISTS assets
(
    id          integer
        primary key,
    entity_id   integer,

    assets_path text,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists assets_index on assets (id);
create index if not exists assets_index2 on assets (entity_id);

CREATE TABLE IF NOT EXISTS comment
(
    id             integer
        primary key,
    entity_id      integer,

    comment_string text,
    comment_time   text,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists comment_index on comment (id);
create index if not exists comment_index2 on comment (entity_id);

CREATE TABLE IF NOT EXISTS export_file_info
(
    id           integer
        primary key,
    entity_id    integer,

    file_path    text,
    start_frame  integer,
    end_frame    integer,
    ref_file     text,
    export_type_ text,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists export_file_info_index on export_file_info (id);
create index if not exists export_file_info_index2 on export_file_info (entity_id);

CREATE TABLE IF NOT EXISTS image_icon
(
    id        integer
        primary key,
    entity_id integer,

    path      text,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists image_icon_index on image_icon (id);
create index if not exists image_icon_index2 on image_icon (entity_id);

CREATE TABLE IF NOT EXISTS assets_file
(
    id        integer
        primary key,
    entity_id integer,

    name      text,
    path      text,
    version   integer,
    user_ref  integer,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists assets_file_index on assets_file (id);
create index if not exists assets_file_index2 on assets_file (entity_id);

CREATE TABLE IF NOT EXISTS time_point_info
(
    id            integer
        primary key,
    entity_id     integer,

    first_time    datetime,
    second_time   datetime,
    info          text,
    is_extra_work boolean,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists time_point_info_index on time_point_info (id);
create index if not exists time_point_info_index2 on time_point_info (entity_id);

CREATE TABLE IF NOT EXISTS project_config
(
    id                                integer
        primary key,
    entity_id                         integer,

    sim_path                          text,
    export_group                      text,
    cloth_proxy                       text,
    simple_module_proxy               text,
    find_icon_regex                   text,
    assets_list                       text,
    icon_extensions                   text,
    upload_path                       text,
    season_count                      integer,
    use_only_sim_cloth                boolean,
    use_divide_group_export           boolean,
    use_rename_material               boolean,
    use_merge_mesh                    boolean,
    t_post                            integer,
    export_anim_time                  integer,
    maya_camera_select                boolean,
    use_write_metadata                boolean,
    abc_export_extract_reference_name text,
    abc_export_format_reference_name  text,
    abc_export_extract_scene_name     text,
    abc_export_format_scene_name      text,
    abc_export_add_frame_range        boolean,
    maya_camera_suffix                text,
    maya_out_put_abc_suffix           text,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists project_config_index on project_config (id);
create index if not exists project_config_index2 on project_config (entity_id);
CREATE TABLE IF NOT EXISTS season
(
    id        integer
        primary key,
    entity_id integer,

    p_int     integer,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists season_index on season (id);
create index if not exists season_index2 on season (entity_id);

CREATE TABLE IF NOT EXISTS importance
(
    id        integer
        primary key,
    entity_id integer,

    cutoff_p  text,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists importance_index on importance (id);
create index if not exists importance_index2 on importance (entity_id);

CREATE TABLE IF NOT EXISTS project
(
    id         integer
        primary key,
    entity_id  integer,

    p_name     text,
    p_path     text,
    p_en_str   text,
    p_shor_str text,
    foreign key (entity_id) references entity (id) on delete cascade on update cascade
);
create index if not exists project_index on project (id);
create index if not exists project_index2 on project (entity_id);
     