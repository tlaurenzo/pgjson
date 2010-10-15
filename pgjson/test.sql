drop table if exists testbin;
create table testbin(c jsonbinary);

insert into testbin values (E'\\xff000ffff000ffffff');
insert into testbin values (E'\\xf6');
insert into testbin values (E'\\xfa');
insert into testbin values (E'\\xf800');
insert into testbin values (E'\\xf801');
insert into testbin values (E'\\xfe03000000484900');
insert into testbin values (E'\\xf0cafebabe');
insert into testbin values (E'\\xeecafebabecafebabe');

insert into testbin values (E'\\x0500000000');
insert into testbin values ('{first: 1, second: 2, third: [1, 2]}'::varchar);

select c::text as json, c as native from testbin;

drop table if exists testtext;
create table testtext(c json);

insert into testtext select * from testbin;
insert into testtext values ('{}');
insert into testtext values ('{first: 1, second: 2, third: [1, 2]}');
insert into testtext values ('{first: [1, 2]}');

select c::text as json, c::bytea as bytes from testtext;

