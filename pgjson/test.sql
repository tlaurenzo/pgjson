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

--select c::text as json, c as native from testbin;

drop table if exists testtext;
create table testtext(c json);

insert into testtext select * from testbin;
insert into testtext values ('{}');
insert into testtext values ('{first: 1, second: 2, third: [1, 2]}');
insert into testtext values ('{first: [1, 2]}');

select c as json from testtext;

select ('{a:1,b:2}'::json -> 'a');
select ('{a:{embed:"document"},b:2}'::json -> 'a');

select c -> 'third[1]' from testtext;
select c -> 'third[1]' from testbin;

drop table if exists users;
create table users (id bigserial, data json);
insert into users (data) values ('{
	first_name: "Terry",
	last_name: "Laurenzo",
	email: {
		"work": "someone@bigcompany.com",
		"personal": "justanyone@aol.com" 
	}
}');
insert into users (data) values ('{
	first_name: "Joe",
	last_name: "Schmoe",
	email: {
		"work": "joe@shcmoeswidgets.com",
		"personal": "joe@aol.com" 
	}
}');

select id, (data -> 'first_name')::text as "First Name", (data -> 'last_name')::text as "Last Name" from users;

drop view if exists users_flat;
create view users_flat as
   select
         id,
         (data -> 'first_name')::text as "First Name", 
         (data -> 'last_name')::text as "Last Name",
         (data -> 'email.work')::text as "Work Email",
         (data -> 'email.personal')::text as "Personal Email"
   from users;
   
select * from users_flat;

