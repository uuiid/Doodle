create table assclass
(
	id smallint auto_increment,
	file_name varchar(256) null,
	__file_class__ smallint null,
	constraint id
		unique (id),
	constraint assclass_ibfk_1
		foreign key (__file_class__) references fileclass (id)
);

create index __file_class__
	on assclass (__file_class__);

alter table assclass
	add primary key (id);

