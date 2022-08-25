create table if not exists metadatatab
(
    id          integer primary key,
    parent      integer                            null,
    uuidPath    text                               null,
    user_data   text                               null,
    update_time datetime default CURRENT_TIMESTAMP not null,
    meta_type   bigint   default 0                 null,
    episode     int                                null,
    shot        int                                null,
    season      int                                null,
    assets_p    text                               null,
    constraint fk_test_id
        foreign key (parent) references metadatatab (id)
            on delete cascade
);

create index IF NOT EXISTS Metadata_parent_index
    on metadatatab (parent);
create index IF NOT EXISTS Metadata_episode_index
    on metadatatab (episode);
create index IF NOT EXISTS Metadata_shot_index
    on metadatatab (shot);
create index IF NOT EXISTS Metadata_uuidPath_index
    on metadatatab (uuidPath);

create table if not exists usertab
(
    id               integer primary key,
    user_name        text             not null,
    uuid_path        text             null,
    user_data        text             null,
    permission_group bigint default 0 not null
);

create index IF NOT EXISTS usertab_uuid_path_index
    on usertab (uuid_path);
create index IF NOT EXISTS usertab_user_name_index
    on usertab (user_name);


create table if not exists doodle_info
(
    version_major integer not null,
    version_minor integer not null
);

insert into doodle_info (version_major, version_minor)
values (0, 0);
