create table doodleConfig
(
	id INTEGER not null,
	doodleKey TEXT not null,
	doodleValue TEXT not null,
	primary key (id)
);

create index doodleConfig_id_index
	on doodleConfig (id);

