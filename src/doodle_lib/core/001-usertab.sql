create table if not exists usertab
(
    id               bigint unsigned auto_increment
        primary key,
    user_name        text             not null,
    uuid_path        text             null,
    permission_group bigint default 0 not null
);

create index usertab_id_index
    on usertab (id);

create index usertab_permission_group_index
    on usertab (permission_group);

create index usertab_user_name_index
    on usertab (user_name(512))
