pgjson - JSON Support for PostgreSQL
====================================
Pgjson provides datatypes and support functions for using JSON-like datatypes directly
in a PostgreSQL database.  For the purpose of this discussion "JSON" is taken to mean
the eco-system of encodings for data-structures comprised of arrays, dictionaries and primitives
such as integers, floats, strings, dates, etc.


Author and History
------------------
Pgjson was created by Terry Laurenzo (tj@laurenzo.org) solely because it could be done and hadn't.  I was
recovering from a minor medical procedure and had about a week on my hands.  I have always loved PostgreSQL
and adding a capability like this had been on my todo list for a long time.  I know the current trend is
to use NoSQL databases, and one of the primary draws is the schemaless nature, but I've always preferred
to have *some* structure with the ability to play freely within certain bounds.  In addition, the PostgreSQL
type system, procedure languages, spatial support and everything else that makes it such a great database
system is really a lot to give up in the name of coloring outside of the lines.

The first genesis of thought for this project came when seeing that spatial support in the NoSQL database
world was missing.  I love PostGIS and have written simple spatial extensions for other database-like systems
and had considered adding spatial support to MongoDB, but the feeling of reinventing wheels overwhelmed me.
I knew I could do it, but just didn't have a lot of interest in writing another quadtree, rtree or geohashing
system along with all of the support operations needed to make it anything but a toy.  So, I decided to
attack the problem from the other direction and see how far I could get by taking PostgreSQL closer to the
web norm of simple HTTP interfaces with semi-defined JSON-like data representations.  This project is the first
step - integrating pervasive JSON-like data structures into the PostgreSQL eco-system.  I've got followon
projects in my head for creating a pgjson-aware web gateway for the database server, and a JavaScript procedural
language for the database server (could be very powerful when combined with Postgres9's inline functions).

Anyway, as one of those "because its there" type of efforts, I'm just having fun playing with the nuts and bolts
and coding in C.

Current Status
--------------
This project is currently a toy.  Production use would be insane at this juncture.

Dependencies
------------

* postgresql (developed with 8.4 and 9.0)

Compatibility
-------------
I have built it on the following
configurations:

*	Mac OSX Snow Leopard 32bit x86
*	Ubuntu 10.04 32bit x86 (gcc 4.4.3)
*	Ubuntu 10.04 64bit x86 (gcc 4.4.3)


Building
--------
Make sure that pgxs is on the path and all postgresql dev packages are installed and
run:

*	make

The shared library for the server will be pgjson.so.  The SQL script to install is generated at
pgjson.dev.sql.  Note that this script is intended for use against a DEV database as it has a hard-coded
reference to the absolute location of the shared library in the dev tree and uses DROP...CASCADE constructs
that can trash any real database.

PostgreSQL Types
-------------------------------------
The following types are defined:

* json - The core type. Internally uses a binary representation for storing
the data
  
Casts (not yet re-implemented)
=====
Casting is provided to and from all primitive types.  The way this is done
is being investigated to determine the right set of casts and auxillary
functions to make interactions natual feeling.

Operators (not yet re-implemented)
=========
The "->" operator is an alias for the jsoneval(json,jsonpath) function, allowing
selection of json members with a dereferncing-like syntax.  For exmaple, assume
the following table:

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

In order to access properties of the data, do something like this:

	# select id, (data -> 'first_name')::text as "First Name", (data -> 'last_name')::text as "Last Name" from users;
    id | First Name | Last Name 
   ----+------------+-----------
     1 | Terry      | Laurenzo
     2 | Joe        | Schmoe
   (2 rows)

Note that the values being returned are json objects (representing strings).  It would be
more valuable to cast these to a known internal type.

You can also create a flattened view:

	create view users_flat as
		select
				id,
				(data -> 'first_name')::text as "First Name", 
				(data -> 'last_name')::text as "Last Name",
				(data -> 'email.work')::text as "Work Email",
				(data -> 'email.personal')::text as "Personal Email"
		from users;

And run queries against it:
		
	# select * from users_flat;
    id | First Name | Last Name |       Work Email       |   Personal Email   
   ----+------------+-----------+------------------------+--------------------
     1 | Terry      | Laurenzo  | someone@bigcompany.com | justanyone@aol.com
     2 | Joe        | Schmoe    | joe@shcmoeswidgets.com | joe@aol.com
   (2 rows)
	
   # select * from users_flat where "Last Name" = 'Laurenzo';
    id | First Name | Last Name |       Work Email       |   Personal Email   
   ----+------------+-----------+------------------------+--------------------
     1 | Terry      | Laurenzo  | someone@bigcompany.com | justanyone@aol.com
   (1 row)	
	
   
					
	
License and Copyright
=====================
Unless otherwise noted, all files in this project are Copyright 2010 Terry Laurenzo
and is licensed under the following MIT license:

	Copyright (c) 2010 Terry Laurenzo
	
	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	
	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
   
This project contains code from the PostgreSQL project which is subject to the
PostgreSQL License:

	PostgreSQL Database Management System
	(formerly known as Postgres, then as Postgres95)
	
	Portions Copyright (c) 1996-2010, The PostgreSQL Global Development Group
	
	Portions Copyright (c) 1994, The Regents of the University of California
	
	Permission to use, copy, modify, and distribute this software and its documentation 
	for any purpose, without fee, and without a written agreement is hereby granted, 
	provided that the above copyright notice and this paragraph and the following 
	two paragraphs appear in all copies.
	
	IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR 
	DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING 
	LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
	EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY 
	OF SUCH DAMAGE.
	
	THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, 
	BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
	A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, 
	AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE, 
	SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
		

