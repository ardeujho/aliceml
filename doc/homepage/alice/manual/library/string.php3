<?php include("macros.php3"); ?>
<?php heading("The STRING signature", "The <TT>STRING</TT> signature") ?>

<?php section("synopsis", "synopsis") ?>

  <PRE>
    signature STRING
    structure String : STRING where type string = string
                                and type char   = char
    structure WideString : STRING where type char = WideChar.t
  </PRE>

  <P>
    An extended version of the Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/string.html"><TT>STRING</TT></A> signature.
  </P>

  <P>See also:
    <A href="mono-vector.php3"><TT>MONO_VECTOR</TT></A>,
    <A href="substring.php3"><TT>SUBSTRING</TT></A>,
    <A href="unique-string.php3"><TT>UNIQUE_STRING</TT></A>,
    <A href="string-cvt.php3"><TT>StringCvt</TT></A>
  </P>

<?php section("import", "import") ?>

  <P>
    Imported implicitly.
  </P>

<?php section("interface", "interface") ?>

  <PRE>
    signature STRING =
    sig
	eqtype char
	eqtype string
	type t = string

	val maxSize :     int

	val size :        string -> int
	val str :         char -> string
	val sub :         string * int -> char
	val substring :   string * int * int -> string
	val extract :     string * int * int option -> string

	val op ^ :        string * string -> string
	val concat :      string list -> string
	val concatWith :  string -> string list -> string
	val implode :     char list -> string
	val explode :     string -> char list
	val tabulate :    int * (int -> char) -> string

	val map :         (char -> char) -> string -> string
	val translate :   (char -> string) -> string -> string
	val fields :      (char -> bool) -> string -> string list
	val tokens :      (char -> bool) -> string -> string list

	val op < :        string * string -> bool
	val op > :        string * string -> bool
	val op <= :       string * string -> bool
	val op >= :       string * string -> bool
	val equal :       string * string -> bool
	val compare :     string * string -> order
	val collate :     (char * char -> order) -> string * string -> order
	val hash :        string -> int

	val isPrefix :    string -> string -> bool
	val isSuffix :    string -> string -> bool
	val isSubstring : string -> string -> bool

	val toWide :      string -> WideString.string
	val fromWide :    WideString.string -> string

	val toString :    string -> string
	val toCString :   string -> string
	val fromString :  string -> string option
	val fromCString : string -> string option
	val scan :        (char,'a) StringCvt.reader -> (string,'a) StringCvt.reader
	val scanC :       (char,'a) StringCvt.reader -> (string,'a) StringCvt.reader
    end
  </PRE>

<?php section("description", "description") ?>

  <P>
    Items not described here are as in the  Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/string.html"><TT>STRING</TT></A> signature.
  </P>

  <DL>
    <DT>
      <TT>type t = string</TT>
    </DT>
    <DD>
      <P>A local synonym for type <TT>string</TT>.</P>
    </DD>

    <DT>
      <TT>tabulate (<I>n</I>, <I>f</I>)</TT>
    </DT>
    <DD>
      <P>Creates a string of size <TT><I>n</I></TT>, where the characters are
      defined in order of increasing index by applying <TT><I>f</I></TT> to the
      character's index. This is equivalent to the expression:</P>
      <PRE>
        implode (List.tabulate (<I>n</I>, <I>f</I>))</PRE>
      <P>If <TT><I>n</I></TT> &lt; 0 or <TT>maxSize</TT> &lt; <TT><I>n</I></TT>,
      then the <TT>Size</TT> exception is raised. </P>
    </DD>

    <DT>
      <TT>equal (<I>s1</I>, <I>s2</I>)</TT>
    </DT>
    <DD>
      <P>An explicit equality function on strings. Equivalent to <TT>op=</TT>.</P>
    </DD>

    <DT>
      <TT>hash <I>s</I></TT>
    </DT>
    <DD>
      <P>A hash function for strings.</P>
    </DD>

    <DT>
      <TT>scan <I>getc</I> <I>strm</I></TT> <BR>
      <TT>scanC <I>getc</I> <I>strm</I></TT>
    </DT>
    <DD>
      <P>Scans a string as an SML (C) source program string, converting escape
      sequences into the appropriate characters. These functions are similar to
      <TT>fromString</TT> and <TT>fromCString</TT>, but can convert from
      arbitrary streams.</P>
    </DD>

    <DT>
      <TT>toWide <I>s</I></TT> <BR>
      <TT>fromWide <I>s</I></TT>
    </DT>
    <DD>
      <P>Convert between the standard and the wide character set. Raise
      <TT>Chr</TT> if any character of the given string is not representable
      in the target character set.</P>
    </DD>
  </DL>

<?php footing() ?>
