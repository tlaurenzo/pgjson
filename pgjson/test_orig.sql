drop table if exists testbin;
create table testbin(c jsonbinary);

insert into testbin values (E'\\x0500000000');
insert into testbin values ('{first: 1, second: 2, third: [1, 2]}'::varchar);

select c::text as json, c as native, c::bytea as bytea from testbin;

drop table if exists testtext;
create table testtext(c json);

insert into testtext values ('{}');
insert into testtext values ('{first: 1, second: 2, third: [1, 2]}');
insert into testtext values ('{first: [1, 2]}');

select c::text as json, c as native, c::bytea as bytea from testtext;

