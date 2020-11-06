create table if not exists asstype
(
	id smallint auto_increment,
	ass_type varchar(64) null,
	assClass_id smallint null,
	constraint id
		unique (id),
	constraint asstype_ibfk_1
		foreign key (assClass_id) references assclass (id)
);

create index assClass_id
	on asstype (assClass_id);

alter table asstype
	add primary key (id);

