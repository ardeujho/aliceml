<?php include("macros.php3"); ?>
<?php heading("The OS.FileSys structure", "The <TT>OS.FileSys</TT> structure") ?>

<?php section("synopsis", "synopsis") ?>

  <PRE>
    signature OS_FILE_SYS
    structure FileSys : OS_FILE_SYS
  </PRE>

  <P>
    The Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/os-file-sys.html"><TT>OS.FileSys</TT></A> structure.
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
    signature OS_FILE_SYS =
    sig
	type dirstream
	eqtype file_id
	datatype access_mode = A_READ | A_WRITE | A_EXEC

	val chDir :    string -> unit
	val getDir :   unit -> string
	val mkDir :    string -> unit
	val rmDir :    string -> unit
	val isDir :    string -> bool

	val isLink :   string -> bool
	val readLink : string -> string

	val fileSize : string -> Position.int
	val modTime :  string -> Time.time

	val remove :   string -> unit

	val tmpName :  unit -> string
    end
  </PRE>

<?php section("description", "description") ?>

  <P>
    Like the Standard ML Basis'
    <A href="http://SML.sourceforge.net/Basis/os-file-sys.html"><TT>OS.FileSys</TT></A> structure.
  </P>

  <P>
    <I>Limitations:</I> The following functions are currently missing:
  </P>

  <UL>
    <LI><TT>openDir</TT>, <TT>readDir</TT>, <TT>rewindDir</TT>, <TT>closeDir</TT></LI>
    <LI><TT>fullPath</TT>, <TT>realPath</TT></LI>
    <LI><TT>access</TT></LI>
    <LI><TT>setTime</TT></LI>
    <LI><TT>fileId</TT></LI>
    <LI><TT>hash</TT>, <TT>compare</TT></LI>
  </UL>

<?php footing() ?>
