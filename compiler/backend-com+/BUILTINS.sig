(*
 * Author:
 *   Leif Kornstaedt <kornstae@ps.uni-sb.de>
 *
 * Copyright:
 *   Leif Kornstaedt, 1999-2000
 *
 * Last change:
 *   $Date$ by $Author$
 *   $Revision$
 *)

signature BUILTINS =
    sig
	val lookupClass: string -> IL.dottedname
	val lookupField: string -> IL.dottedname * IL.id
    end
