SET search_path = public;

CREATE TYPE json;

CREATE OR REPLACE FUNCTION json_in(cstring) RETURNS json
	AS 'pgjson.so'
	LANGUAGE C IMMUTABLE;
