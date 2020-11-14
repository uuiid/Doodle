create table configure
(
	id smallint auto_increment,
	name varchar(128) null,
	value varchar(128) null,
	project_id smallint null,
	constraint id
		unique (id),
	constraint configure_ibfk_1
		foreign key (project_id) references project (id)
);

create index project_id
	on configure (project_id);

alter table configure
	add primary key (id);

