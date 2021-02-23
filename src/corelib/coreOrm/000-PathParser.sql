create table PathParser
(
	id INTEGER not null,
	root TEXT not null,
	refClass TEXT not null,
	regex TEXT not null,
	prefix TEXT,
	suffix TEXT,
	format TEXT not null,
	method TEXT,
	arg TEXT,
	primary key (id)
);

create index PathParser_ClassPath_index
	on PathParser (root);

create unique index PathParser_id_index
	on PathParser (id);

