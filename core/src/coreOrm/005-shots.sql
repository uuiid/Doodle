create table shots
(
	id smallint auto_increment,
	shot smallint null,
	shotab varchar(64) null,
	episodes_id smallint null,
	constraint id
		unique (id),
	constraint shots_ibfk_1
		foreign key (episodes_id) references episodes (id)
);

create index episodes_id
	on shots (episodes_id);

alter table shots
	add primary key (id);

