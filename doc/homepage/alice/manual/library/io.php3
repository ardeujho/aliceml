<?php include("macros.php3"); ?>
<?php heading("The IO structure", "The <TT>IO</TT> structure") ?>

<?php section("synopsis", "synopsis") ?>

  <PRE>
    signature IO
    structure IO : IO
  </PRE>

  <P>
    The Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/io.html"><TT>IO</TT></A> structure.
  </P>

  <P>See also:
    <A href="bin-io.php3"><TT>BinIO</TT></A>,
    <A href="text-io.php3"><TT>TextIO</TT></A>,
    <A href="imperative-io.php3"><TT>IMPERATIVE_IO</TT></A>,
    <A href="stream-io.php3"><TT>STREAM_IO</TT></A>,
    <A href="prim-io.php3"><TT>PRIM_IO</TT></A>,
  </P>

<?php section("import", "import") ?>

  <P>
    Imported implicitly.
  </P>

<?php section("interface", "interface") ?>

  <PRE>
    signature IO =
    sig
	exception Io of {name : string, function : string, cause : exn}
	exception BlockingNotSupported
	exception NonblockingNotSupported
	exception RandomAccessNotSupported
	exception ClosedStream

	datatype buffer_mode = NO_BUF | LINE_BUF | BLOCK_BUF
    end
  </PRE>

<?php section("description", "description") ?>

  <P>
    Like the Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/io.html"><TT>IO</TT></A> structure.
  </P>

<?php footing() ?>
