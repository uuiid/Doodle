create table context
(
    id          integer auto_increment
        constraint entity_pk
        primary key,
    com_hash integer,
    json_data text
);
create index if not exists context_index_id
    on context (id);
create table entity
(
    id          integer auto_increment
        constraint entity_pk
        primary key,
    uuid_data   text,
    update_time datetime default CURRENT_TIMESTAMP not null

);
create index if not exists entity_index
    on entity (id);
create table com_entity
(
    id        integer auto_increment
        constraint entity_pk
        primary key,
    entity_id integer
        constraint entity_id_ref references entity (id),
    com_hash integer,
    json_data text
);
create index if not exists com_entity_index
    on com_entity (id);
create index if not exists com_entity_index_hash
    on com_entity (com_hash);

create table if not exists usertab
(
    id               bigint unsigned auto_increment
        primary key,
    user_name        text not null,
    uuid_path        text null,
    permission_group bigint default 0 not null
);

create index usertab_id_index
    on usertab (id);

create index usertab_permission_group_index
    on usertab (permission_group);

create index usertab_user_name_index
    on usertab (user_name);

create trigger UpdataLastTime_ AFTER UPDATE OF json_data
    ON com_entity
begin
update entity set update_time =CURRENT_TIMESTAMP where id = old.entity_id;
end;

