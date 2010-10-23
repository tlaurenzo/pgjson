/**
 * jsonbinary.h
 * Defines an internal binary representation for extended json
 * structures.
 *
 * Goals of the binary representation:
 *   1. Fast sibling traversal for arrays and objects
 *   2. Direct representation of all scalar and compound json datatypes in a stream
 *   3. Space efficiency nominally as good or better than JSON text for a wide cross section of
 *      payloads
 *   4. Direct representation of Unicode strings
 *   5. Direct representation of arbitrary precision numbers
 *   6. Ability to recognize and ignore unknown values
 *   7. Direct translation to and from other formats such as JSON-text, BSON, etc
 *   8. Biased towards small documents and values but supports arbitrary sizes
 *
 * The format was designed to be able to represent a proper superset of JSON as
 * defined at http://www.json.org.  Additions have been added to support JSON as
 * used in common practice:
 *   a. Binary values
 *   b. Date values
 *   c. Undefined value
 *   d. Separate storage for int, float and arbitrary precision numeric
 *
 * Not all JSON representations will be able to handle these extensions portably
 * but different transcoder options can bridge the gap.
 *
 * Definition
 * ----------
 * A jsonb stream is a sequence of bytes representing either a JSON scalar or compound
 * value of one of the following types:
 *   Compound:
 *   	1. Object: Collection of name/value pairs
 *   	2. Array: Ordered list of values
 *   Scalar:
 *   	1. String
 *   	2. Number
 *   		a. Integer
 *   		b. Big Integer
 *   		c. Floating point
 *   		d. Arbitrary precision numeric
 *   	3. Boolean
 *   	4. Null
 *   	5. Undefined
 *   	6. Binary string
 *   	7. Date
 *
 * Each VALUE has the following form:
 * 		<TYPESPEC:BYTE> <LENGTH:ANY>? <DATA:BYTE*>
 *
 * The TYPESPEC byte consists consists of two fields:
 * 		TYPECODE (bits 0-4)
 * 		LENGTHSPEC (bits 5-7)
 * Both fields within TYPESPEC are treated as an unsigned integer.
 *
 * The TYPECODE (5 bits) specifies the type of the data to follow:
 * 		0x00: Object
 * 		0x01: Array
 * 		0x02: String
 * 		0x03: Integer
 * 		0x04: Float
 * 		0x05: Decimal
 * 		0x06: Boolean true
 * 		0x07: Boolean false
 * 		0x08: Null
 * 		0x09: Undefined
 * 		0x0a: Binary String
 * 		0x0b: Date
 *
 * LENGTHSPEC (3 bits) is interpreted as follows:
 * 		0: Length==0.  No Length field follows.
 * 		1: One byte fixed length.  No length field follows.
 * 		2: Two byte fixed length.  No length field follows.
 * 		3: Four byte fixed length.  No length field follows.
 * 		4: Eight byte fixed length.  No length field follows.
 * 		5: Tiny variable length.  One byte LENGTH field follows.
 * 		6: Small variable length.  Two byte LENGTH field follows.
 * 		7: Large variable length.  Four byte LENGTH field follows.
 *
 * As can be seen, there are multiple legal ways to store lengths and readers should
 * be able to deal with all of them in any context.  Writers should make an effort to
 * use the smallest length representation, but efficiency trades can be made.  For
 * example, the length of objects and arrays may not be known in advance and in a write
 * once stream, the proper LENGTH representation will need to be chosen in advance in
 * order to eliminate intermediate buffering.
 *
 * Also, some types are only valid with certain actual lengths:
 * 		- Boolean true/false (0x06, 0x07), null (0x08), undefined (0x09): Length must be zero
 * 		- Integer (0x03): Length must be 4 or 8 bytes signifying a 4 or 8 byte little endian integer
 * 		- Float (0x04): Length must be 4 or 8 bytes signifying a IEEE 754 single or double precision float
 * 		- Date (0x0b): Length must be 8 bytes and DATA is the number of ms since the epoch
 *
 * If the LENGTH field exists, it is always an unsigned native byte order integer of size
 * specified by LENGTHSPEC.
 *
 * Type Specific Data Formats
 * --------------------------
 *
 * Objects
 * =======
 * The DATA for an object consists of a sequence (potentially empty) of PAIRS.  Each PAIR is a
 * modified UTF-8 encoded stringz representing the pair name immediately followed by the VALUE.
 *
 * Arrays
 * ======
 * The DATA for an array consists of a sequence (potentially empty) of VALUES.
 *
 * String
 * ======
 * The DATA for a String is the UTF-8 encoded data for the string.  It is not null terminated.
 *
 * Integer
 * =======
 * The DATA for an Integer is the 4 or 8 byte native byte order 2's complement signed integer value.
 *
 * Float
 * =====
 * The DATA for a Float is the 4 or 8 byte native byte order single or double precision IEEE 754 floating point value.
 *
 * Binary String
 * =============
 * The DATA is the binary value
 *
 * Date
 * ====
 * The DATA is the UTC number of milliseconds since the epoch
 *
 * Decimal
 * =======
 * Unlike "calculation" oriented representation such as the Postgres NumericData or
 * Java BigDecimal, this data type is purely a mechanism for easily storing a lossless
 * representation of a JSON arbitrary precision number.  It is a variable length format
 * based on BID decimal numbers from the IEEE 754 (2008) standard and the Java BigDecimal /
 * BigInteger classes.  It consists of the following:
 *    <EXPONENT:signed int32> <SIGNIFICAND:byte>+
 * The SIGNIFICAND is a variable length binary 2's complement integer stored with its
 * most significant byte first.
 *
 * Note that the SIGNIFICAND is always represented in big-endian, regardless of the
 * byte order.
 *
 * The LENGTH of a Decimal value must be >= 5.
 *
 * Endian-ness
 * -----------
 * The JSON binary stream can either be stored in big-endian or native byte order.  In general,
 * internal representations will likely be native and external, big-endian.  As such, byte swap
 * functions are defined for taking an entire stream from native to external (big endian)
 * and vica-versa.
 */
