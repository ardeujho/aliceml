<?php include("macros.php3"); ?>
<?php heading("The TEXT signature", "The <TT>TEXT</TT> signature") ?>

<?php section("synopsis", "synopsis") ?>

  <PRE>
    signature TEXT
    structure Text : TEXT where Char       = Char
                            and String     = String
			    and Substring  = Substring
			    and CharVector = CharVector
			    and CharArray  = CharArray
  </PRE>

  <P>
    An extended version of the Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/text.html"><TT>TEXT</TT></A> signature.
  </P>

  <P>See also:
    <A href="char.php3"><TT>CHAR</TT></A>,
    <A href="string.php3"><TT>STRING</TT></A>,
    <A href="substring.php3"><TT>SUBSTRING</TT></A>,
    <A href="mono-vector.php3"><TT>MONO_VECTOR</TT></A>,
    <A href="mono-array.php3"><TT>MONO_ARRAY</TT></A>
  </P>

<?php section("import", "import") ?>

  <P>
    Imported implicitly.
  </P>

<?php section("interface", "interface") ?>

  <PRE>
    signature TEXT =
    sig
	structure Char :       CHAR
	structure String :     STRING
			       where type char   = Char.t
				 and type string = Char.string
	structure Substring :  SUBSTRING
			       where type char   = Char.t
				 and type string = String.t
	structure CharVector : MONO_VECTOR
			       where type elem   = Char.t
				 and type vector = String.t
	structure CharArray :  MONO_ARRAY
			       where type elem   = Char.t
				 and type vector = String.t
    end
  </PRE>

<?php section("description", "description") ?>

  <P>
    Like the  Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/text.html"><TT>TEXT</TT></A> signature.
  </P>

<?php footing() ?>
