#!/bin/bash
ITERATIONS=100
PSQL=psql
td="$(dirname $0)"

echo "Running benchmark with 100 iterations per step" >&2


range() {
   local i=1
   local end=$1
   while [ "$i" -le "$end" ]
   do
      echo $i
      i=$((i+1))
   done
}


# The next three generators produce test scripts that report lengths of
#  - binary parsed data
#  - original data (no parse)
#  - normalized (parsed to text)
generate_parse_to_binary() {
   local i
   for i in $(range $ITERATIONS); do
      echo "select Length(JsonAsBinary(doc::json)), docname from jsontest_source_big;"
   done
}
generate_parse_null() {
   local i
   for i in $(range $ITERATIONS); do
      echo "select Length(doc), docname from jsontest_source_big;"
   done
}
generate_parse_normalize() {
   local i
   for i in $(range $ITERATIONS); do
      echo "select Length(JsonNormalize(doc)), docname from jsontest_source_big;"
   done
}
generate_serialize() {
   local i
   for i in $(range $ITERATIONS); do
      echo "select Length(doc::text), docname from jsontest_json_big;"
   done
}

generate_roundtrip() {
   local i
   for i in $(range $ITERATIONS); do
      echo "select Length((doc::json)::text), docname from jsontest_source_big;"
   done
}

# Load data
echo "Loading data..."
$td/gensamples.sh | $PSQL > /dev/null
echo "Data loaded."

# Report size statistics
echo "=== DOCUMENT SIZE STATISTICS ==="
echo '
   select docname "Document Name", Length(doc) "Original Size", 
      Length(JsonAsBinary(doc::json)) "Binary Size",
      Length(JsonNormalize(doc)) "Normalized Size",
      ((Length(JsonNormalize(doc)) - Length(JsonAsBinary(doc::json)))::double precision / Length(JsonNormalize(doc))::double precision)*100 "Percentage Savings"
   from jsontest_source;
' | $PSQL

### Run tests
echo "=== TEST PARSE AND SERIALIZATION ===" >&2
echo "Null Parse:" >&2
generate_parse_null | time $PSQL > $td/results-null.txt

echo "Parse to Binary:" >&2
generate_parse_to_binary | time $PSQL > $td/results-parsebinary.txt

echo "Serialize from Binary:" >&2
generate_serialize | time $PSQL > $td/results-serialize.txt

echo "Normalize Text:" >&2
generate_parse_normalize | time $PSQL > $td/results-parsenormalize.txt

echo "Roundtrip (parse to binary and serialize):" >&2
generate_parse_normalize | time $PSQL > $td/results-parsenormalize.txt

