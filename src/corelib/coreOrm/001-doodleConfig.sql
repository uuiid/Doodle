create table doodleConfig
(
	id INTEGER not null,
	localRoot TEXT not null,
	projectName TEXT not null,
	primary key (id)
);

create index doodleConfig_id_index
	on doodleConfig (id);

