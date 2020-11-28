create table configure
(
	id bigint auto_increment,
	name varchar(128) null,
	value varchar(128) null,
	project_id bigint null,
	constraint id
		unique (id),
	constraint configure_ibfk_1
		foreign key (project_id) references project (id)
);

create index project_id
	on configure (project_id);

alter table configure
	add primary key (id);

