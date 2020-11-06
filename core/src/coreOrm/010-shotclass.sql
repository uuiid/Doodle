create table if not exists shotclass
(
	id smallint auto_increment,
	shot_class varchar(64) null,
	shots_id smallint null,
	episodes_id smallint null,
	constraint id
		unique (id),
	constraint shotclass_ibfk_1
		foreign key (shots_id) references shots (id),
	constraint shotclass_ibfk_2
		foreign key (episodes_id) references episodes (id)
);

create index episodes_id
	on shotclass (episodes_id);

create index shots_id
	on shotclass (shots_id);

alter table shotclass
	add primary key (id);

