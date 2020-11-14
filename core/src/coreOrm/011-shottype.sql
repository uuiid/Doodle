create table shottype
(
	id smallint auto_increment,
	shot_type varchar(64) null,
	shotClass_id smallint null,
	shots_id smallint null,
	constraint id
		unique (id),
	constraint shottype_ibfk_1
		foreign key (shotClass_id) references shotclass (id),
	constraint shottype_ibfk_2
		foreign key (shots_id) references shots (id)
);

create index shotClass_id
	on shottype (shotClass_id);

create index shots_id
	on shottype (shots_id);

alter table shottype
	add primary key (id);

