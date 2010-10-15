drop table if exists testtext;
create table testtext(c json);

insert into testtext values ('{first: [1, 2]}');

select c::text as json, c as native, c::bytea as bytea from testtext;

