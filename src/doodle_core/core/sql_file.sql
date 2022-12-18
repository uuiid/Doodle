create table if not exists context
(
    id        integer auto_increment
        primary key,
    com_hash  integer,
    json_data text
);
create index if not exists context_index_id
    on context (id);
create unique index if not exists ctx_hash_index
    on context (com_hash);

create table if not exists entity
(
    id          integer auto_increment
        constraint entity_pk
            primary key,
    uuid_data   text,
    update_time datetime default CURRENT_TIMESTAMP not null

);
create index if not exists entity_index
    on entity (id);

create table if not exists com_entity
(
    id        integer auto_increment
        primary key,
    entity_id integer,
    com_hash  integer,
    json_data text,
    foreign key (entity_id)
        references entity (id)
        on delete cascade
        on update cascade
);
create index if not exists com_entity_index
    on com_entity (id);
create index if not exists com_entity_index_hash
    on com_entity (com_hash);

create table if not exists usertab
(
    id               bigint unsigned auto_increment
        primary key,
    user_name        text             not null,
    uuid_path        text             null,
    permission_group bigint default 0 not null
);

create index if not exists usertab_id_index
    on usertab (id);

create index if not exists usertab_permission_group_index
    on usertab (permission_group);

create index if not exists usertab_user_name_index
    on usertab (user_name);

create trigger if not exists UpdataLastTime_
    AFTER UPDATE OF json_data
    ON com_entity
begin
    update entity set update_time =CURRENT_TIMESTAMP where id = old.entity_id;
end;

create table if not exists doodle_info
(
    version_major integer not null,
    version_minor integer not null
);
