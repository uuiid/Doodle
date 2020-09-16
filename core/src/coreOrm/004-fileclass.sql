create table fileclass
(
	id smallint auto_increment,
	file_class varchar(64) null,
	__shot__ smallint null,
	__episodes__ smallint null,
	constraint id
		unique (id),
	constraint fileclass_ibfk_1
		foreign key (__shot__) references shot (id),
	constraint fileclass_ibfk_2
		foreign key (__episodes__) references episodes (id)
);

create index __episodes__
	on fileclass (__episodes__);

create index __shot__
	on fileclass (__shot__);

alter table fileclass
	add primary key (id);

