pgjson - JSON Support for PostgreSQL
====================================
Pgjson provides datatypes and support functions for using JSON-like datatypes directly
in a PostgreSQL database.  For the purpose of this discussion "JSON" is taken to mean
the eco-system of encodings for data-structures comprised of arrays, dictionaries and primitives
such as integers, floats, strings, dates, etc.

This module uses a modified [BSON](http://bsonspec.org/) encoding for its data at rest.  BSON was
created for [MongoDB](http://www.mongodb.org/display/DOCS/BSON) to serve a similar purpose.  The primary
modification is a special encoding for non-Document root values.  For background, the original
BSON spec defines a Document as the abstraction for storing both Lists and Dictionaries and only
allows for a single Document (interpreted as a Dictionary) to be stored as the root element of
a stream.  This implementation creates a superset of BSON capable of storing arbitrary BSON value
types at the root by taking advantage of the high order bit of the first byte of the stream.  If the
high order bit is 1, then the first byte is taken as a negated two's-complement Element Type followed
directly by the Element value exactly as specified for within a Document.  This hack works because
the first four bytes of a stream are supposed to be a little-endian signed 32bit integer representing
the Document length.  Since a length can never be negative, it means that the high-order bit of the stream's
first byte will never by 1 for a standard BSON stream.

This BSON extension creates a workable format for a portable binary representation of any BSON value,
effectively creating a packed binary variant for representing any type of data.  Integrating this into
the Pgjson system allows full correspondence between BSON types and PostgreSQL types.  One side-effect
of this is that the "json" datatype introduced by this package is effectively an "any" type capable
of storing and accessing a wide range of primitives, Lists and Dictionaries.  With appropriate support
functions, this takes PostgreSQL a fair way to being a multi-value database.

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

* autoconf (developed with 2.65 and 2.68)
* automake (developed with 1.11)
* libtool  (developed with 2.2.6 and 2.4)
* flex (developed with 2.5.35)
* bison (developed with 2.3 and 2.4.1)
* postgresql (developed with 8.4 and 9.0)

Compatibility
-------------
I have built it on the following
configurations:

*	Mac OSX Snow Leopard 32bit x86
*	Ubuntu 10.04 32bit x86 (gcc 4.4.3)

As of yet, I have not tried it on a 64bit platform or anything big endian.  64bit support is probably easy as
I've been mindful of it but just haven't pulled it down to a real 64bit system to verify.  In order to work
on big endian systems, several byte swap macros in bsonconst.h will need to be defined properly.  On systems
that only allow aligned memory access, READ_INT32 and READ_INT64 macros in bsonparser.c will need to be
defined properly.

Due to an unfortunate design decision, there is a dependency on funopen/fopencookie to attach a custom write
function to a C stream.  This will work on BSD and Linux based systems, but others (Solaris, Windows, etc) will
not build until this is factored out.  It shouldn't be very hard to do, but I haven't gotten to it yet.  The code
that uses this facility was written early on when I was just prototyping and needs to be revisited.

Building
--------
The project does not yet install, but it can be run out of the dev tree.  Ensure that the following are on your
PATH and that you have the PostgreSQL server headers installed (try postgresql-server-8.4-dev and friends on Ubuntu):

*	autoconf
*	aclocal
*	automake
*	libtoolize
*	flex
*	bison
*	pg_config

Then run:

*	./autobootstrap.sh
*	make

The shared library for the server will be pgplugin/libpgjson-v.v.v.so.  The SQL script to install is generated at
pgplugin/pgjson.sql.  Note that this script is intended for use against a DEV database as it has a hard-coded
reference to the absolute location of the shared library in the dev tree and uses DROP...CASCADE constructs
that can trash any real database.

Example
-------
Here is a stupid example that exercises the json and jsonbinary datatypes (these types differ in that json
uses a JSON text-based external representation whereas jsonbinary exposes the binary representation as its
external form).

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

And the output:

						  json                 |                                                     native                                                     
	--------------------------------------+----------------------------------------------------------------------------------------------------------------
	 nan                                  | \xffffffff000ffff000ffffff
	 null                                 | \xf6ffffff
	 undefined                            | \xfaffffff
	 false                                | \xf8ffffff00
	 true                                 | \xf8ffffff01
	 "HI"                                 | \xfeffffff03000000484900
	 -1095041334                          | \xf0ffffffcafebabe
	 -4703166714098286902                 | \xeeffffffcafebabecafebabe
	 {}                                   | \x0500000000
	 {"first":1,"second":2,"third":[1,2]} | \x360000001066697273740001000000107365636f6e640002000000047468697264001300000010300001000000103100020000000000
	(10 rows)
	
						  json                 |                                                     bytes                                                      
	--------------------------------------+----------------------------------------------------------------------------------------------------------------
	 nan                                  | \xffffffff000ffff000ffffff
	 null                                 | \xf6ffffff
	 undefined                            | \xfaffffff
	 false                                | \xf8ffffff00
	 true                                 | \xf8ffffff01
	 "HI"                                 | \xfeffffff03000000484900
	 -1095041334                          | \xf0ffffffcafebabe
	 -4703166714098286902                 | \xeeffffffcafebabecafebabe
	 {}                                   | \x0500000000
	 {"first":1,"second":2,"third":[1,2]} | \x360000001066697273740001000000107365636f6e640002000000047468697264001300000010300001000000103100020000000000
	 {}                                   | \x0500000000
	 {"first":1,"second":2,"third":[1,2]} | \x360000001066697273740001000000107365636f6e640002000000047468697264001300000010300001000000103100020000000000
	 {"first":[1,2]}                      | \x1f000000046669727374001300000010300001000000103100020000000000
	(13 rows)
	
	
	
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
		

