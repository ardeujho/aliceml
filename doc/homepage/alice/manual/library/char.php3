<?php include("macros.php3"); ?>
<?php heading("The CHAR signature", "The <TT>CHAR</TT> signature") ?>

<?php section("synopsis", "synopsis") ?>

  <PRE>
    signature CHAR
    structure Char : CHAR where type char = char and type string = string
    structure WideChar : CHAR
  </PRE>

  <P>
    An extended version of the Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/char.html"><TT>CHAR</TT></A> signature.
  </P>

  <P>
    <I>Limitation:</I> <TT>WideChar</TT> currently is the same structure as
    <TT>Char</TT>.
  </P>

  <P>See also:
    <A href="string.php3"><TT>String</TT></A>
  </P>

<?php section("import", "import") ?>

  <P>
    Imported implicitly.
  </P>

<?php section("interface", "interface") ?>

  <PRE>
    signature CHAR =
    sig
	eqtype char
	eqtype string
	type t = char

	val minChar :     char
	val maxChar :     char
	val maxOrd :      int

	val chr :         int -> char
	val ord :         char -> int
	val pred :        char -> char
	val succ :        char -> char

	val op < :        char * char -> bool
	val op <= :       char * char -> bool
	val op > :        char * char -> bool
	val op >= :       char * char -> bool
	val equal :       char * char -> bool
	val compare :     char * char -> order
	val hash :        char -> int

	val contains :    string -> char -> bool
	val notContains : string -> char -> bool

	val toLower :     char -> char
	val toUpper :     char -> char

	val isLower :     char -> bool
	val isUpper :     char -> bool
	val isAlpha :     char -> bool
	val isAlphaNum :  char -> bool
	val isDigit :     char -> bool
	val isBinDigit :  char -> bool
	val isOctDigit :  char -> bool
	val isHexDigit :  char -> bool
	val isPunct :     char -> bool
	val isPrint :     char -> bool
	val isGraph :     char -> bool
	val isSpace :     char -> bool
	val isCntrl :     char -> bool
	val isAscii :     char -> bool

	val toWide :      char -> WideChar.char
	val fromWide :    WideChar.char -> char

	val toString :    char -> string
	val toCString :   char -> string
	val fromString :  string -> char option
	val fromCString : string -> char option
	val scan :        (char,'a) StringCvt.reader -> (char,'a) StringCvt.reader
	val scanC :       (char,'a) StringCvt.reader -> (char,'a) StringCvt.reader
    end
  </PRE>

<?php section("description", "description") ?>

  <P>
    Items not described here are as in the  Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/char.html"><TT>CHAR</TT></A> signature.
  </P>

  <DL>
    <DT>
      <TT>type t = char</TT>
    </DT>
    <DD>
      <P>A local synonym for type <TT>char</TT>.</P>
    </DD>

    <DT>
      <TT>equal (<I>c1</I>, <I>c2</I>)</TT>
    </DT>
    <DD>
      <P>An explicit equality function on chars. Equivalent to <TT>op=</TT>.</P>
    </DD>

    <DT>
      <TT>hash <I>c</I></TT>
    </DT>
    <DD>
      <P>A hash function for characters. Returns <TT>Int.hash (ord
      <I>c</I>)</TT>.</P>
    </DD>

    <DT>
      <TT>isBinDigit <I>c</I></TT>
    </DT>
    <DD>
      <P>Returns <TT>true</TT> iff <TT><I>c</I></TT> is a binary digit (0 or
      1).</P>
    </DD>

    <DT>
      <TT>isOctDigit <I>c</I></TT>
    </DT>
    <DD>
      <P>Returns <TT>true</TT> iff <TT><I>c</I></TT> is an octal digit (0-7).</P>
    </DD>

    <DT>
      <TT>scanC <I>getc</I> <I>strm</I></TT>
    </DT>
    <DD>
      <P>Scans a character (including space) or a C escape sequence representing
      a character from the prefix of a string. Similar to <TT>scan</TT>, except
      that it uses C escape conventions, like the function
      <TT>fromCString</TT>.</P>
    </DD>

    <DT>
      <TT>toWide <I>c</I></TT> <BR>
      <TT>fromWide <I>c</I></TT>
    </DT>
    <DD>
      <P>Convert between the standard and the wide character set. Raise
      <TT>Chr</TT> if the given character is not representable in the target
      character set.</P>
    </DD>
  </DL>

<?php footing() ?>
