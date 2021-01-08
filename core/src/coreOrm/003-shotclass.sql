create table if not exists shotclass
(
	id bigint auto_increment,
	shot_class varchar(64) null,
	project_id bigint null,
	constraint id
		unique (id),
	constraint shotclass_ibfk_1
		foreign key (project_id) references project (id)
);

create index project_id
	on shotclass (project_id);

alter table shotclass
	add primary key (id);

