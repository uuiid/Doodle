create table pinyin
(
	id integer
		primary key autoincrement,
	znch text,
	en text
);

create index _znch_
	on pinyin (znch);

