create table shot
(
	id smallint auto_increment,
	__episodes__ smallint null,
	shot_ smallint null,
	shotab varchar(64) null,
	constraint id
		unique (id),
	constraint shot_ibfk_1
		foreign key (__episodes__) references episodes (id)
);

create index __episodes__
	on shot (__episodes__);

alter table shot
	add primary key (id);

