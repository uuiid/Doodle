create table zh_ch
(
	id INTEGER,
	znch TEXT not null,
	enus TEXT not null,
	primary key (id)
);

create index zh_ch_znch_index
	on zh_ch (znch);

create index zn_ch_enus_index
	on zh_ch (enus);

