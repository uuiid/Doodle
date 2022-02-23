create table if not exists metadatatab
(
    id          bigint unsigned auto_increment
        primary key,
    parent      bigint unsigned    null,
    uuidPath    text               null,
    user_data   text               null,
    update_time datetime default CURRENT_TIMESTAMP not null,
    meta_type   bigint   default 0 null,
    episode     int                null,
    shot        int                null,
    season      int                null,
    assets_p    text               null,
    uuid_data   text               null,
    constraint fk_test_id
        foreign key (parent) references metadatatab (id)
            on delete cascade
);

create index Metadata_id_index
    on metadatatab (id);

create index Metadata_parent_index
    on metadatatab (parent);

create index metadatatab_episode_index
    on metadatatab (episode);

create index metadatatab_shot_index
    on metadatatab (shot);
