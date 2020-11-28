create table shots
(
	id bigint auto_increment,
	shot bigint null,
	shotab varchar(64) null,
	episodes_id bigint null,
	constraint id
		unique (id),
	constraint shots_ibfk_1
		foreign key (episodes_id) references episodes (id)
);

create index episodes_id
	on shots (episodes_id);

alter table shots
	add primary key (id);

