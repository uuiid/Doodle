create table if not exists metadata
(
	id bigint unsigned not null
		primary key,
	parent bigint unsigned null,
	path text null,
	update_time datetime default CURRENT_TIMESTAMP not null
);

create index Metadata_id_index
	on metadata (id);

create index Metadata_parent_index
	on metadata (parent);

