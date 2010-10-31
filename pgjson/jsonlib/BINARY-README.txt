Goals of the binary representation:
  1. Fast sibling traversal for arrays and objects
  2. Direct representation of all scalar and compound json datatypes in a stream
  3. Space efficiency nominally as good or better than JSON text for a wide cross section of
     payloads
  4. Direct representation of Unicode strings
  5. Direct representation of arbitrary precision numbers
  6. Ability to recognize and ignore unknown values
  7. Direct translation to and from other formats such as JSON-text, BSON, etc
  8. Biased towards small documents and values but supports arbitrary sizes
 
The format was designed to be able to represent a proper superset of JSON as
defined at http://www.json.org.  Additions have been added to support JSON as
used in common practice:
  a. Binary values
  b. Date values
  c. Undefined value
  d. Separate storage for int, float and arbitrary precision numeric

Not all JSON representations will be able to handle these extensions portably
but different transcoder options can bridge the gap.

Definition
----------
A jsonb stream is a sequence of bytes representing either a JSON scalar or compound
value of one of the following types:
  Compound:
  	1. Object: Collection of name/value pairs
  	2. Array: Ordered list of values
  Scalar:
  	1. String
  	2. Number
  	3. Boolean
  	4. Null
  	5. Undefined

Each VALUE has the following form:
		<TYPESPEC:BYTE> <CONTINUED_LENGTH:ANY>? <DATA:BYTE*>

The TYPESPEC is designed to communicate the internal structure of the data
that follows plus the low order 4 bits of the data length and a bit to
indicate that the length spills over to additional bytes.  This takes
advantage of the assumption that the most common lengths can be represented
in 4 bits (length of 16) or less.  For larger lengths, the LENGTHCONT bit
will be 1, indicating that the byte following the TYPESPEC continues the
length.

The lower 7 bits of CONTINUED_LENGTH bytes add 7bits of length to the total,
reserving the high order bit to indicate that an additional CONTINUED_LENGTH
byte follows.

The TYPESPEC byte consists consists of the fields:
	TYPE (bit 7-5):
		000 = object
		001 = array
		010 = string
		011 = number
		100 = simple scalar
		101 = typed string
		110 = typed binary
		111 = reserved
	LENGTHCONT (bit 4): 1 if the length continues to the next byte
	LENGTH (bit 3-0): Low 4 bits of length of following data

Type Specific Data Formats
--------------------------

Objects (000)
=============
The DATA for an object consists of a sequence (potentially empty) of PAIRS.  Each PAIR is a
modified UTF-8 encoded stringz representing the pair name immediately followed by the VALUE.

Arrays (001)
============
The DATA for an array consists of a sequence (potentially empty) of VALUES.

String (010)
============
The DATA for a String is the UTF-8 encoded data for the string.  It is not null terminated.

Number (011)
============
Numbers are stored as the ascii characters describing the number as specified by the json spec.

Simple Scalar (100)
===================
The DATA consists of a single byte holding the encoded simple scalar value:
	0 : false
	1 : true
	2 : null
	3 : undefined
	
Typed String (101)
==================
This is an extension mechanism for storing strings which can/should be interpreted as discreet
types if possible.  The DATA consists of a single STRING_TYPE byte followed by the UTF-8 string
value.  The LENGTH includes the STRING_TYPE byte, making the string length to be LENGTH - 1.

Presently defined STRING_TYPE codes:
	0 : Reserved
	1 : Date as number of ms since the epoch (UTC)
	
Typed Binary (110)
==================
This is an extension mechanism for storing binary values that are interpreted as a particular type.
The DATA consists of a single BINARY_TYPE byte followed by the bytes that make up the value.  The
LENGTH includes the BINARY_TYPE byte.

The only defined BINARY_TYPE is 0, which indicates an untyped binary value.  Unless if other formatting
options are given during conversion to a textual form, this should be output encoded as Base64.

Byte Order
==========
No multi-byte binary values are present in the format.  Therefore, it is completely agnostic with respect
to byte order.
