(*
 * Author:
 *   Leif Kornstaedt <kornstae@ps.uni-sb.de>
 *
 * Copyright:
 *   Leif Kornstaedt, 1999
 *
 * Last change:
 *   $Date$ by $Author$
 *   $Revision$
 *)

SMLofNJ.Internals.GC.messages false;
CM.make' "top/main-com+.cm";

local
    fun hdl f x =
	(f x; OS.Process.success)
	handle e =>
	    (TextIO.output (TextIO.stdErr,
			    "uncaught exception " ^ exnName e ^ "\n");
	     OS.Process.failure)

    fun defaults () =
	(SMLToComPlusMain.Switches.printComponentSig := true;
	 SMLToComPlusMain.Switches.defaultImport := true)

    fun stoc nil =   (* for testing bootstrapping *)
	(defaults ();
	 hdl SMLToComPlusMain.flattenString (TextIO.inputAll TextIO.stdIn))
      | stoc ([infile, "-o", outfile] | ["-c", infile, "-o", outfile]) =
	(defaults ();
	 hdl SMLToComPlusMain.compile (infile, outfile, ""))
      | stoc [infile, outfile] =
	(defaults ();
	 hdl SMLToComPlusMain.compile (infile, outfile, ""))
      | stoc _ = OS.Process.failure

    fun main _ = stoc (tl(CommandLine.arguments ()))
in
    val _ = SMLofNJ.exportFn ("stoc-com+", main)
end;
