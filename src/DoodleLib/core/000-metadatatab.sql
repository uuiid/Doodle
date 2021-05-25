create table if not exists metadataTab
(
	id bigint unsigned not null
		primary key,
	parent bigint unsigned null,
	uuidPath text null,
	update_time datetime default CURRENT_TIMESTAMP not null
);

create index Metadata_id_index
	on metadatatab (id);

create index Metadata_parent_index
	on metadatatab (parent);

