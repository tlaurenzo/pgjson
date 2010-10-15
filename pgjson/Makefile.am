ACLOCAL_AMFLAGS = -I m4
SUBDIRS = shared pgplugin


psql: all
	psql < pgplugin/pgjson.sql
	
psqltest: psql
	psql < test.sql
	
