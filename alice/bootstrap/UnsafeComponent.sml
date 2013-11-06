(*
 * Author:
 *   Leif Kornstaedt <kornstae@ps.uni-sb.de>
 *
 * Copyright:
 *   Leif Kornstaedt, 2000-2003
 *
 * Last change:
 *   $Date$ by $Author$
 *   $Revision$
 *)

(* Dummy replacement for bootstrapping *)

structure UnsafeComponent =
    struct
	fun unavailable f =
	    (TextIO.output (TextIO.stdErr,
			    "UnsafeComponent." ^ f ^
			    "unavailable in bootstrap compiler");
	     raise Fail ("UnsafeComponent." ^ f))

	fun load _ = unavailable "load"
	fun save _ = unavailable "save"
    end
