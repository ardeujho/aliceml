<?php include("macros.php3"); ?>
<?php heading("The OS.IO structure", "The <TT>OS.IO</TT> structure") ?>

<?php section("synopsis", "synopsis") ?>

  <PRE>
    signature OS_IO
    structure IO : OS_IO
  </PRE>

  <P>
    The Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/os-io.html"><TT>OS.IO</TT></A> structure.
  </P>

  <P>See also:
    <A href="os.php3"><TT>OS</TT></A>
  </P>

<?php section("import", "import") ?>

  <P>
    Imported implicitly.
  </P>

<?php section("interface", "interface") ?>

  <PRE>
    signature OS_IO =
    sig
	eqtype iodesc
	eqtype iodesc_kind

	val hash :    iodesc -> word
	val compare : iodesc * iodesc -> order
	val kind :    iodesc -> iodesc_kind

	structure Kind :
	sig
	    val file :    iodesc_kind
	    val dir :     iodesc_kind
	    val symlink : iodesc_kind
	    val tty :     iodesc_kind
	    val pipe :    iodesc_kind
	    val socket :  iodesc_kind
	    val device :  iodesc_kind
	end
    end
  </PRE>

<?php section("description", "description") ?>

  <P>
    Like the Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/os-io.html"><TT>OS.IO</TT></A> structure.
  </P>

  <P>
    <I>Limitations:</I> The polling functionality is currently missing:
  </P>

  <UL>
    <LI><TT>type</TT> <TT>poll_desc</TT> <TT>and</TT> <TT>poll_info</TT></LI>
    <LI><TT>exception</TT> <TT>Poll</TT></LI>
    <LI><TT>pollDesc</TT>, <TT>pollToIODesc</TT></LI>
    <LI><TT>pollIn</TT>, <TT>pollOut</TT>, <TT>pollPri</TT></LI>
    <LI><TT>poll</TT></LI>
    <LI><TT>isIn</TT>, <TT>isOut</TT>, <TT>isPri</TT></LI>
    <LI><TT>infoToPollDesc</TT></LI>
  </UL>

<?php footing() ?>
