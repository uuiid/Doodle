//
// Created by TD on 2022/5/31.
//

#pragma once

#include <string_view>

namespace doodle::database_n {
constexpr auto create_ctx_table            = R"(
create table if not exists context
(
    id          integer
            primary key,
    com_hash integer,
    json_data text
);
)";

constexpr auto create_ctx_table_index      = R"(
create index if not exists context_index_id
    on context (id);
)";
constexpr auto create_ctx_table_unique     = R"(
create unique index if not exists ctx_hash_index
    on context (com_hash);
)";

constexpr auto create_entity_table         = R"(
create table if not exists entity
(
    id          integer
            primary key,
    uuid_data   text,
    update_time datetime default CURRENT_TIMESTAMP not null

);
)";
constexpr auto create_entity_table_index   = R"(
create index if not exists entity_index
    on entity (id);
)";

constexpr auto create_com_table            = R"(
create table if not exists com_entity
(
    id        integer
            primary key,
    entity_id integer,
    com_hash  integer,
    json_data text,
    foreign key (entity_id)
        references entity (id)
        on delete cascade
        on update cascade
);
)";
constexpr auto create_com_table_index_id   = R"(
create index if not exists com_entity_index
    on com_entity (id);
)";
constexpr auto create_com_table_index_hash = R"(
create index if not exists com_entity_index_hash
    on com_entity (com_hash);
)";
constexpr auto create_com_table_trigger    = R"(
create trigger if not exists UpdataLastTime_ AFTER UPDATE OF json_data
    ON com_entity
begin
    update entity set update_time =CURRENT_TIMESTAMP where id = old.entity_id;
end;
)";
constexpr auto create_version_table        = R"(
create table if not exists doodle_info
(
    version_major integer not null,
    version_minor integer not null
);

)";
}  // namespace doodle::database_n
