<?php include("macros.php3"); ?>
<?php heading("The IEEEReal structure", "The <TT>IEEEReal</TT> structure") ?>

<?php section("synopsis", "synopsis") ?>

  <PRE>
    signature IEEE_REAL
    structure IEEEReal : IEEE_REAL
  </PRE>

  <P>
    The Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/ieee-float.html"><TT>IEEEReal</TT></A> structure.
  </P>

  <P>See also:
    <A href="real.php3"><TT>Real</TT></A>
  </P>

<?php section("import", "import") ?>

  <P>
    Imported implicitly.
  </P>

<?php section("interface", "interface") ?>

  <PRE>
    signature IEEE_REAL =
    sig
	 exception Unordered

	 datatype real_order    = LESS | EQUAL | GREATER | UNORDERED
	 datatype float_class   = NAN | INF | ZERO | NORMAL | SUBNORMAL
	 datatype rounding_mode = TO_NEAREST | TO_NEGINF | TO_POSINF | TO_ZERO
	 type decimal_approx    = {kind : float_class, sign : bool, digits : int list,  exp : int}
    end
  </PRE>

<?php section("description", "description") ?>

  <P>
    Like the Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/ieee-float.html"><TT>IEEEReal</TT></A> structure.
  </P>

  <P>
    <I>Limitations:</I> The following standard functions are
    currently missing:
  </P>

  <UL>
    <LI><TT>getRoundingMode</TT>, <TT>setRoundingMode</TT></LI>
    <LI><TT>toString</TT>, <TT>fromString</TT></LI>
  </UL>

<?php footing() ?>
