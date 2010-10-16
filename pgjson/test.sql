drop table if exists testbin;
create table testbin(c jsonbinary);

insert into testbin values (E'\\xcafebabe');
insert into testbin values (E'\\xffffffff000ffff000ffffff');
insert into testbin values (E'\\xf6ffffff');
insert into testbin values (E'\\xfaffffff');
insert into testbin values (E'\\xf8ffffff00');
insert into testbin values (E'\\xf8ffffff01');
insert into testbin values (E'\\xfeffffff03000000484900');
insert into testbin values (E'\\xf0ffffffcafebabe');
insert into testbin values (E'\\xeeffffffcafebabecafebabe');

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

