(*
 * Author:
 *   Leif Kornstaedt <kornstae@ps.uni-sb.de>
 *
 * Copyright:
 *   Leif Kornstaedt, 2000
 *
 * Last change:
 *   $Date$ by $Author$
 *   $Revision$
 *)

(*
 * pickle    ::= int | chunk | block | transform
 * int       ::= POSINT <uint> | NEGINT <uint>
 * chunk     ::= CHUNK size <byte>*size
 * size      ::= <uint>
 * block     ::= BLOCK label size field*size
 *            |  CLOSURE size field*size
 * label     ::= <uint>
 * field     ::= pickle | reference
 * reference ::= REF id
 * id        ::= <uint>
 * transform ::= TRANSFORM (chunk|reference) field
 *)

signature PRIM_PICKLE =
    sig
	type outstream
	type id

	datatype label =
	    TAG of LargeInt.int
	  | CLOSURE
	  | ARRAY
	  | ARRAY_ZERO
	  | CELL
	  | CONSTRUCTOR
	  | CON_VAL
	  | GLOBAL_STAMP
	  | TUPLE
	  | VECTOR
	  | VECTOR_ZERO

	type size = int

	val openOut: string -> outstream
	val outputInt: outstream * LargeInt.int -> unit
	val outputChunk: outstream * Word8.word vector -> id
	val outputBlock: outstream * label * size -> id
	val outputReference: outstream * id -> unit
	val outputString: outstream * string -> id
	val outputTransform: outstream * string -> id
	val closeOut: outstream -> unit
    end
