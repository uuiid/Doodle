create table shottype
(
	id bigint auto_increment,
	shot_type varchar(64) null,
	shotClass_id bigint null,
	project_id bigint null,
	constraint id
		unique (id),
	constraint shottype_ibfk_1
		foreign key (shotClass_id) references shotclass (id),
	constraint shottype_ibfk_2
		foreign key (project_id) references project (id)
);

create index project_id
	on shottype (project_id);

create index shotClass_id
	on shottype (shotClass_id);

alter table shottype
	add primary key (id);

