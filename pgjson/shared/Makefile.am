AM_CPPFLAGS = -I../include

noinst_LTLIBRARIES = libjson.la

libjson_la_SOURCES = \
   util/stringutil.c \
   util/stringutil_platform.c \
   util/hexdump.c \
   jsonparser.tab.c \
   jsonparser.yy.c \
   jsonpath.tab.c \
   jsonpath.yy.c \
   json/jsonserializer.c \
   json/jsonaccess.c \
   json/jsonparser.c \
   json/bsonserializer.c \
   json/jsonoutputter.c \
   json/bsonparser.c \
   json/jsonpath.c
   
# Lex and yacc rules
jsonparser.tab.c: json/jsonparser.tab.g
	$(YACC) -d -o jsonparser.tab.c json/jsonparser.tab.g
	
jsonparser.yy.c: json/jsonparser.yy.l jsonparser.tab.c jsonparser.tab.h
	$(LEX) -o jsonparser.yy.c json/jsonparser.yy.l

jsonpath.tab.c: json/jsonpath.tab.g
	$(YACC) -d -o jsonpath.tab.c json/jsonpath.tab.g
	
jsonpath.yy.c: json/jsonpath.yy.l jsonpath.tab.c jsonpath.tab.h
	$(LEX) -o jsonpath.yy.c json/jsonpath.yy.l
	
bin_PROGRAMS = jsontool jsonquery

jsontool_SOURCES = json/jsontool.c
jsontool_LDADD = libjson.la

jsonquery_SOURCES = json/jsonquery.c
jsonquery_LDADD = libjson.la

clean-local:
	test -z jsonparser.yy.c || rm -f jsonparser.yy.c
	test -z jsonparser.tab.c || rm -f jsonparser.tab.c 
	test -z jsonparser.tab.h || rm -f jsonparser.tab.h 
	
