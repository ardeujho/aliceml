<?php include("macros.php3"); ?>
<?php heading("The PRIM_IO signature", "The <TT>PRIM_IO</TT> signature") ?>

<?php section("synopsis", "synopsis") ?>

  <PRE>
    signature PRIM_IO
    structure BinPrimIO : PRIM_IO where type array  = Word8Array.t
    				    and type vector = Word8Vector.t
				    and type elem   = Word8.t
				    and type pos    = Position.t
    structure TextPrimIO : PRIM_IO where type array = CharArray.t
    				    and type vector = CharVector.t
				    and type elem   = Char.t
    functor PrimIO (structure V :  MONO_VECTOR
                    structure A :  MONO_ARRAY where Vector = V
		    val someElem : V.elem
		    type pos
		    val compare :  pos * pos -> order) :
	    PRIM_IO where type elem   = V.elem
	              and type vector = V.t
		      and type array  = A.t
		      and type pos    = pos
  </PRE>

  <P>
    The Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/prim-io.html"><TT>PRIM_IO</TT></A> signature and
    <A href="http://SML.sourceforge.net/Basis/prim-io-fn.html"><TT>PrimIO</TT></A> functor.
  </P>

  <P>See also:
    <A href="stream-io.php3"><TT>STREAM_IO</TT></A>,
    <A href="imperative-io.php3"><TT>IMPERATIVE_IO</TT></A>,
    <A href="bin-io.php3"><TT>BinIO</TT></A>,
    <A href="text-io.php3"><TT>TextIO</TT></A>
  </P>

<?php section("import", "import") ?>

  <P>
    Imported implicitly.
  </P>

<?php section("interface", "interface") ?>

  <PRE>
    signature PRIM_IO =
    sig
	type elem
	type vector
	type array

	eqtype pos
	val compare : pos * pos -> order

	datatype reader = RD of
	    {name :       string,
	     chunkSize :  int,
	     readVec :    (int -> vector) option,
	     readArr :    ({buf: array, i: int, sz: int option} -> int) option,
	     readVecNB :  (int -> vector option) option,
	     readArrNB :  ({buf: array, i: int, sz: int option} -> int option) option,
	     block :      (unit -> unit) option,
	     canInput :   (unit -> bool) option,
	     avail :      unit -> int option,
	     getPos :     (unit -> pos) option,
	     setPos :     (pos -> unit) option,
	     endPos :     (unit -> pos) option,
	     verifyPos :  (unit -> pos) option,
	     close :      unit -> unit,
	     ioDesc :     OS.IO.iodesc option}

	datatype writer = WR of
	    {name :       string,
	     chunkSize :  int,
	     writeVec :   ({buf: vector, i: int, sz: int option} -> int) option,
	     writeArr :   ({buf: array,  i: int, sz: int option} -> int) option,
	     writeVecNB : ({buf: vector, i: int, sz: int option} -> int option) option,
	     writeArrNB : ({buf: array,  i: int, sz: int option} -> int option) option,
	     block :      (unit -> unit) option,
	     canOutput :  (unit -> bool) option,
	     getPos :     (unit -> pos) option,
	     setPos :     (pos -> unit) option,
	     endPos :     (unit -> pos) option,
	     verifyPos :  (unit -> pos) option,
	     close :      unit -> unit,
	     ioDesc :     OS.IO.iodesc option}

	val openVector :    vector -> reader
	val nullRd :        unit -> reader
	val nullWr :        unit -> writer
	val augmentReader : reader -> reader
	val augmentWriter : writer -> writer
    end
  </PRE>

<?php section("description", "description") ?>

  <P>
    Like the Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/prim-io.html"><TT>PRIM_IO</TT></A> signature and
    <A href="http://SML.sourceforge.net/Basis/prim-io-fn.html"><TT>PrimIO</TT></A> functor.
  </P>

<?php footing() ?>
