create table shotclass
(
	id smallint auto_increment,
	shot_class varchar(64) null,
	shots_id smallint null,
	constraint id
		unique (id),
	constraint shotclass_ibfk_1
		foreign key (shots_id) references shots (id)
);

create index shots_id
	on shotclass (shots_id);

alter table shotclass
	add primary key (id);

