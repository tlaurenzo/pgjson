#!/bin/bash
# Read all .json files and generate a loadsample.sql script of inserts
td="$(dirname $0)"

# Generate $outfile
echo "drop table if exists jsontest_source;"
echo "create table jsontest_source (docname varchar not null, doc varchar not null);"
for i in $td/*.json
do
   contents="$(cat $i)"
   echo "insert into jsontest_source (docname, doc) values ('$(basename $i)', \$json\$$contents\$json\$);"
done

# Generate the big table of source documents
echo "drop table if exists jsontest_source_big;"
echo "create table jsontest_source_big (docname varchar not null, doc varchar not null);"
for i in {1..1000}
do
   echo "insert into jsontest_source_big (doc, docname) (select doc, docname from jsontest_source);"
done

# Generate the json big table
echo "drop table if exists jsontest_json_big;"
echo "create table jsontest_json_big (docname varchar not null, doc json not null);"
echo "insert into jsontest_json_big (docname, doc) (select docname, doc::json from jsontest_source_big);"
