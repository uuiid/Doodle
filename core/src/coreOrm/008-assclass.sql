create table if not exists assclass
(
	id bigint auto_increment,
	ass_name varchar(256) null,
	assdep_id bigint null,
	constraint id
		unique (id),
	constraint assclass_ibfk_1
		foreign key (assdep_id) references assdepartment (id)
);

create index assdep_id
	on assclass (assdep_id);

alter table assclass
	add primary key (id);

