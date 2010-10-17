AM_CPPFLAGS = -I../include -I../include $(POSTGRESQL_SERVER_CFLAGS)

noinst_LTLIBRARIES = libjsoncalloc.la libjsonpgalloc.la
bin_PROGRAMS = jsontool jsonquery


LIB_SOURCE_FILES = \
	util/stringutil.c \
	util/stringutil_platform.c \
	util/hexdump.c \
	jsonparser.tab.c \
	jsonparser.yy.c \
	jsonpath.tab.c \
	jsonpath.yy.c \
	json/jsonserializer.c \
	json/jsonparser.c \
	json/bsonserializer.c \
	json/jsonoutputter.c \
	json/bsonparser.c \
	json/jsonpath.c \
	json/bsonliteral.c


libjsoncalloc_la_CFLAGS = -include alloc/setupcalloc.h
libjsoncalloc_la_LDFLAGS = -static
libjsoncalloc_la_SOURCES = $(LIB_SOURCE_FILES)

libjsonpgalloc_la_CFLAGS = -include alloc/setuppgalloc.h
libjsonpgalloc_la_LDFLAGS = -shared
libjsonpgalloc_la_SOURCES = $(LIB_SOURCE_FILES)
	

jsontool_SOURCES = json/jsontool.c
jsontool_LDADD = libjsoncalloc.la

jsonquery_SOURCES = json/jsonquery.c
jsonquery_LDADD = libjsoncalloc.la


#### Local rules ####
# Lex and yacc rules
jsonparser.tab.c: json/jsonparser.tab.g
	$(YACC) -d -o jsonparser.tab.c json/jsonparser.tab.g
	
jsonparser.yy.c: json/jsonparser.yy.l jsonparser.tab.c jsonparser.tab.h
	$(LEX) -o jsonparser.yy.c json/jsonparser.yy.l

jsonpath.tab.c: json/jsonpath.tab.g
	$(YACC) -d -o jsonpath.tab.c json/jsonpath.tab.g
	
jsonpath.yy.c: json/jsonpath.yy.l jsonpath.tab.c jsonpath.tab.h
	$(LEX) -o jsonpath.yy.c json/jsonpath.yy.l


clean-local:
	test -z jsonparser.yy.c || rm -f jsonparser.yy.c
	test -z jsonparser.tab.c || rm -f jsonparser.tab.c 
	test -z jsonparser.tab.h || rm -f jsonparser.tab.h 
	test -z jsonpath.tab.c || rm -f jsonpath.tab.c 
	test -z jsonpath.tab.h || rm -f jsonpath.tab.h 
	
