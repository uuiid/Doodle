create table PathParser
(
	id INTEGER not null,
	ClassPath TEXT not null,
	refClass TEXT not null,
	regex TEXT not null,
	prefix TEXT,
	suffix TEXT,
	format TEXT not null,
	primary key (id)
);

create index PathParser_ClassPath_index
	on PathParser (ClassPath);

create unique index PathParser_id_index
	on PathParser (id);

